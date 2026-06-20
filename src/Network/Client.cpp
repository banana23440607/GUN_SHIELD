#include "Client.h"

bool Client::Connect(const char* host, int port)
{
    if (!InitWinsock()) return false;

    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(m_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_recvThread = std::thread(&Client::RecvLoop, this);
    return true;
}

void Client::Stop()
{
    m_running = false;
    if (m_sock != INVALID_SOCKET)
    {
        PacketHeader disc{ PacketType::Disconnect, sizeof(PacketHeader), m_localId };
        NetworkBase::SendTo(m_sock, &disc, sizeof(disc));
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
    if (m_recvThread.joinable()) m_recvThread.join();
}

void Client::Update()
{
    // パケット処理はゲームシーン側が RecvQueue() を使って行う
}

void Client::SendJoin(const char* name, Team preferred)
{
    PktJoin pkt = {};
    pkt.header        = { PacketType::Join, sizeof(PktJoin), m_localId };
    pkt.preferredTeam = preferred;
    strncpy_s(pkt.name, name, 15);
    NetworkBase::SendTo(m_sock, &pkt, sizeof(pkt));
}

void Client::SendInput(const PlayerInput& input, uint32_t sequence)
{
    PktPlayerInput pkt = {};
    pkt.header   = { PacketType::PlayerInput, sizeof(PktPlayerInput), m_localId };
    pkt.input    = input;
    pkt.sequence = sequence;
    NetworkBase::SendTo(m_sock, &pkt, sizeof(pkt));
}

void Client::SendPing()
{
    PacketHeader ping{ PacketType::Ping, sizeof(PacketHeader), m_localId };
    NetworkBase::SendTo(m_sock, &ping, sizeof(ping));
}

void Client::RecvLoop()
{
    while (m_running)
    {
        PacketHeader hdr;
        if (!RecvExact(m_sock, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)))
            break;

        uint16_t bodySize = hdr.size - sizeof(PacketHeader);
        std::vector<uint8_t> data(hdr.size);
        memcpy(data.data(), &hdr, sizeof(hdr));

        if (bodySize > 0)
        {
            if (!RecvExact(m_sock, data.data() + sizeof(hdr), bodySize))
                break;
        }

        // JoinAck は自分のIDとチームを確定
        if (hdr.type == PacketType::JoinAck)
        {
            const PktJoinAck& ack = *reinterpret_cast<const PktJoinAck*>(data.data());
            if (ack.accepted)
            {
                m_localId   = ack.assignedId;
                m_localTeam = ack.assignedTeam;
            }
        }

        m_recvQueue.Push(std::move(data));
    }

    m_running = false;
}
