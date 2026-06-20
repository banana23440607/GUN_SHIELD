#include "LobbyScene.h"
#include "SceneManager.h"
#include "BattleScene.h"
#include "../Input/Input.h"

LobbyScene::LobbyScene(bool isHost, const std::string& playerName, const std::string& hostIp)
    : m_isHost(isHost), m_playerName(playerName), m_hostIp(hostIp)
{
}

LobbyScene::~LobbyScene()
{
    Shutdown();
}

bool LobbyScene::Init()
{
    if (m_isHost)
    {
        m_server = std::make_unique<Server>();
        if (!m_server->Start(DEFAULT_PORT)) return false;
    }

    m_client = std::make_unique<Client>();
    if (!m_client->Connect(m_hostIp, DEFAULT_PORT)) return false;

    m_client->SendJoin(m_playerName, Team::None);
    return true;
}

void LobbyScene::Update(float dt)
{
    ProcessPackets();

    // ホストがスペースキーで開始
    if (m_isHost && Input::Get().IsPressed(VK_SPACE))
    {
        if (m_server) m_server->BroadcastGameStart();
        m_gameStarted = true;
    }

    if (m_gameStarted)
    {
        m_countdown -= dt;
        if (m_countdown <= 0.0f)
        {
            SceneManager::Get().Change(SceneID::Battle);
        }
    }
}

void LobbyScene::Draw()
{
    // TODO: テキスト描画（参加者一覧、チームステータス）
}

void LobbyScene::Shutdown()
{
    if (m_client) m_client->Disconnect();
    if (m_server) m_server->Stop();
}

void LobbyScene::ProcessPackets()
{
    if (!m_client) return;
    auto& q = m_client->RecvQueue();

    while (!q.Empty())
    {
        auto pkt = q.Pop();
        if (pkt.empty()) break;

        const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(pkt.data());
        switch (hdr->type)
        {
        case PacketType::PlayerList:
            if (pkt.size() >= sizeof(PktPlayerList))
                memcpy(&m_playerList, pkt.data(), sizeof(PktPlayerList));
            break;

        case PacketType::TeamStats:
            if (pkt.size() >= sizeof(PktTeamStats))
                memcpy(&m_teamStats, pkt.data(), sizeof(PktTeamStats));
            break;

        case PacketType::GameStart:
            m_gameStarted = true;
            m_countdown   = 0.0f;
            break;

        default:
            break;
        }
    }
}
