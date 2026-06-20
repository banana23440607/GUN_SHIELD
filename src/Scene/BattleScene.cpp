#include "BattleScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Graphics/DX11Manager.h"

BattleScene::BattleScene(bool isHost, Client* client, Server* server)
    : m_isHost(isHost), m_client(client), m_server(server)
{
}

bool BattleScene::Init()
{
    if (m_client)
    {
        m_localId   = m_client->GetLocalId();
        m_localTeam = m_client->GetLocalTeam();
    }

    HWND hwnd = DX11Manager::Get().GetHwnd();
    Input::Get().SetCaptureMouse(true, hwnd);

    return true;
}

void BattleScene::Update(float dt)
{
    if (m_gameOver) return;

    Input::Get().Update();

    if (m_client && m_client->IsConnected())
    {
        PlayerInput pi = Input::Get().ToPlayerInput();
        m_client->SendInput(pi, m_inputSequence++);
    }

    if (m_isHost)
    {
        ServerUpdate(dt);
    }

    ProcessClientPackets();
}

void BattleScene::ServerUpdate(float dt)
{
    m_fixedAccum += dt;
    m_timeRemaining -= dt;

    while (m_fixedAccum >= FIXED_TIMESTEP)
    {
        m_fixedAccum -= FIXED_TIMESTEP;

        if (!m_server) break;

        auto& sq = m_server->RecvQueue();
        while (!sq.Empty())
        {
            auto raw = sq.Pop();
            if (raw.size() < sizeof(PacketHeader)) continue;

            const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(raw.data());
            if (hdr->type == PacketType::PlayerInput &&
                raw.size() >= sizeof(PktPlayerInput))
            {
                const PktPlayerInput* pi = reinterpret_cast<const PktPlayerInput*>(raw.data());
                m_playerManager.ApplyInput(hdr->senderId, pi->input);
            }
        }

        PktPlayerState state = m_playerManager.MakeStatePacket(m_inputSequence);
        if (m_server) m_server->Broadcast(state);
    }

    if (m_timeRemaining <= 0.0f)
    {
        m_winner   = Team::Shielder;
        m_gameOver = true;
        if (m_server) m_server->BroadcastGameEnd(m_winner, static_cast<uint32_t>(BATTLE_TIME));
    }

    CheckWinCondition();
}

void BattleScene::CheckWinCondition()
{
    if (m_gameOver) return;

    int aliveShielders = 0;
    for (uint8_t id = 0; id < MAX_PLAYERS; ++id)
    {
        PlayerData* pd = m_playerManager.GetPlayer(id);
        if (pd && pd->team == Team::Shielder && pd->state == PlayerState::Alive)
            ++aliveShielders;
    }

    if (aliveShielders == 0)
    {
        m_winner   = Team::Gunner;
        m_gameOver = true;
        uint32_t elapsed = static_cast<uint32_t>(BATTLE_TIME - m_timeRemaining);
        if (m_server) m_server->BroadcastGameEnd(m_winner, elapsed);
    }
}

void BattleScene::ProcessClientPackets()
{
    if (!m_client) return;
    auto& q = m_client->RecvQueue();

    while (!q.Empty())
    {
        auto raw = q.Pop();
        if (raw.size() < sizeof(PacketHeader)) continue;

        const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(raw.data());
        switch (hdr->type)
        {
        case PacketType::PlayerState:
            if (raw.size() >= sizeof(PktPlayerState))
                memcpy(&m_lastState, raw.data(), sizeof(PktPlayerState));
            break;

        case PacketType::GameEnd:
            if (raw.size() >= sizeof(PktGameEnd))
            {
                const PktGameEnd* ge = reinterpret_cast<const PktGameEnd*>(raw.data());
                m_winner   = ge->winner;
                m_gameOver = true;
                SceneManager::Get().Change(SceneID::Result);
            }
            break;

        default:
            break;
        }
    }
}

void BattleScene::Draw()
{
    // TODO: プレイヤーの3Dモデル描画
}

void BattleScene::Shutdown()
{
    Input::Get().SetCaptureMouse(false, nullptr);
}
