#include "Server.h"
#include <cstring>

bool Server::Start(int port)
{
    if (!InitWinsock()) return false;

    m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenSock == INVALID_SOCKET) return false;

    int opt = 1;
    setsockopt(m_listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<u_short>(port));

    if (bind(m_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
    if (listen(m_listenSock, MAX_PLAYERS) == SOCKET_ERROR)                  return false;

    m_running = true;
    m_acceptThread = std::thread(&Server::AcceptLoop, this);
    return true;
}

void Server::Stop()
{
    m_running = false;
    closesocket(m_listenSock);
    m_listenSock = INVALID_SOCKET;

    for (auto& c : m_clients)
    {
        if (c.active)
            closesocket(c.sock);
    }

    if (m_acceptThread.joinable()) m_acceptThread.join();

    for (auto& c : m_clients)
        if (c.recvThread.joinable()) c.recvThread.join();
}

void Server::Update()
{
    std::vector<uint8_t> data;
    while (m_recvQueue.Pop(data))
        ProcessPacket(data);
}

void Server::AcceptLoop()
{
    while (m_running)
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET sock = accept(m_listenSock, (sockaddr*)&clientAddr, &addrLen);

        if (sock == INVALID_SOCKET) break;

        std::lock_guard<std::mutex> lk(m_clientMtx);

        int connectedCount = 0;
        for (auto& c : m_clients) if (c.active) ++connectedCount;

        if (connectedCount >= MAX_PLAYERS)
        {
            closesocket(sock);
            continue;
        }

        uint8_t id = AssignId();
        if (id == 0xFF) { closesocket(sock); continue; }

        m_clients[id].sock   = sock;
        m_clients[id].id     = id;
        m_clients[id].active = true;
        m_clients[id].recvThread = std::thread(&Server::RecvLoop, this, id);
    }
}

void Server::RecvLoop(uint8_t clientId)
{
    ClientSession& cs = m_clients[clientId];

    while (m_running && cs.active)
    {
        PacketHeader hdr;
        if (!RecvExact(cs.sock, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)))
            break;

        uint16_t bodySize = hdr.size - sizeof(PacketHeader);
        std::vector<uint8_t> data(hdr.size);
        memcpy(data.data(), &hdr, sizeof(hdr));

        if (bodySize > 0)
        {
            if (!RecvExact(cs.sock, data.data() + sizeof(hdr), bodySize))
                break;
        }

        m_recvQueue.Push(std::move(data));
    }

    std::lock_guard<std::mutex> lk(m_clientMtx);
    closesocket(cs.sock);
    cs.active = false;
    cs.sock   = INVALID_SOCKET;
    SendPlayerList();
    BroadcastTeamStats();
}

void Server::ProcessPacket(const std::vector<uint8_t>& data)
{
    if (data.size() < sizeof(PacketHeader)) return;

    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(data.data());

    switch (hdr->type)
    {
    case PacketType::Join:
        HandleJoin(hdr->senderId, *reinterpret_cast<const PktJoin*>(data.data()));
        break;

    case PacketType::PlayerInput:
        // ゲームロジック（BattleScene）が RecvQueue から直接取得
        break;

    case PacketType::Ping:
    {
        PacketHeader pongHdr{ PacketType::Pong, sizeof(PacketHeader), 0 };
        SendTo(hdr->senderId, &pongHdr, sizeof(pongHdr));
        break;
    }
    default:
        break;
    }
}

void Server::HandleJoin(uint8_t clientId, const PktJoin& pkt)
{
    if (clientId >= MAX_PLAYERS || !m_clients[clientId].active) return;

    ClientSession& cs = m_clients[clientId];
    Team team = AssignTeam(pkt.preferredTeam);
    cs.team = team;
    strncpy_s(cs.name, pkt.name, 15);

    PktJoinAck ack = {};
    ack.header   = { PacketType::JoinAck, sizeof(PktJoinAck), 0 };
    ack.assignedId   = clientId;
    ack.assignedTeam = team;
    ack.accepted     = true;
    SendTo(clientId, &ack, sizeof(ack));

    SendPlayerList();
    BroadcastTeamStats();
}

void Server::SendPlayerList()
{
    PktPlayerList pkt = {};
    pkt.header = { PacketType::PlayerList, sizeof(PktPlayerList), 0 };

    uint8_t count = 0;
    for (auto& c : m_clients)
    {
        if (!c.active) continue;
        PlayerInfo& pi = pkt.players[count++];
        pi.id   = c.id;
        pi.team = c.team;
        strncpy_s(pi.name, c.name, 15);
    }
    pkt.count = count;
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastTeamStats()
{
    int g = GunnerCount();
    int s = ShielderCount();
    m_balancer.Update(g, s);

    PktTeamStats pkt = m_balancer.MakePacket();
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastGameStart()
{
    PacketHeader pkt{ PacketType::GameStart, sizeof(PacketHeader), 0 };
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastPlayerStates(const PktPlayerState& pkt)
{
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastBulletSpawn(const PktBulletSpawn& pkt)
{
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastBulletHit(const PktBulletHit& pkt)
{
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastPlayerDead(const PktPlayerDead& pkt)
{
    Broadcast(&pkt, sizeof(pkt));
}

void Server::BroadcastGameEnd(Team winner, uint32_t elapsedSec)
{
    PktGameEnd pkt = {};
    pkt.header  = { PacketType::GameEnd, sizeof(PktGameEnd), 0 };
    pkt.winner  = winner;
    pkt.elapsedSec = elapsedSec;
    Broadcast(&pkt, sizeof(pkt));
}

int Server::GunnerCount() const
{
    int c = 0;
    for (auto& cl : m_clients)
        if (cl.active && cl.team == Team::Gunner) ++c;
    return c;
}

int Server::ShielderCount() const
{
    int c = 0;
    for (auto& cl : m_clients)
        if (cl.active && cl.team == Team::Shielder) ++c;
    return c;
}

uint8_t Server::AssignId()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
        if (!m_clients[i].active) return static_cast<uint8_t>(i);
    return 0xFF;
}

Team Server::AssignTeam(Team preferred)
{
    int g = GunnerCount();
    int s = ShielderCount();

    if (preferred != Team::None)
    {
        if (preferred == Team::Gunner   && g < MAX_GUNNERS)   return Team::Gunner;
        if (preferred == Team::Shielder && s < MAX_SHIELDERS) return Team::Shielder;
    }

    return (g <= s) ? Team::Gunner : Team::Shielder;
}

void Server::SendTo(uint8_t clientId, const void* data, int size)
{
    if (clientId >= MAX_PLAYERS || !m_clients[clientId].active) return;
    NetworkBase::SendTo(m_clients[clientId].sock, data, size);
}

void Server::Broadcast(const void* data, int size, uint8_t excludeId)
{
    for (auto& c : m_clients)
    {
        if (!c.active || c.id == excludeId) continue;
        NetworkBase::SendTo(c.sock, data, size);
    }
}
