#include "BattleScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Graphics/DX11Manager.h"

static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;

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

    if (!MeshRenderer::Get().Init()) return false;

    // プレイヤー: 幅0.6 高さ1.8 奥行0.6 のボックス
    m_playerMesh = MeshRenderer::CreateBox(0.6f, 1.8f, 0.6f);
    // 地面: 大きな平たいボックス
    m_groundMesh = MeshRenderer::CreateBox(40.0f, 0.2f, 40.0f);

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

XMMATRIX BattleScene::BuildViewProj() const
{
    // 自分のスナップショットを探す
    XMFLOAT3 target = { 0, 0, 0 };
    for (int i = 0; i < m_lastState.count; ++i)
    {
        if (m_lastState.snapshots[i].id == m_localId)
        {
            target = m_lastState.snapshots[i].position.ToFloat3();
            break;
        }
    }

    // TPS カメラ: プレイヤーの後ろ上方から追従
    float yaw = Input::Get().GetYaw();
    XMFLOAT3 offset = {
        -sinf(yaw) * 6.0f,
        3.5f,
        -cosf(yaw) * 6.0f
    };
    XMVECTOR eye    = XMVectorSet(target.x + offset.x,
                                   target.y + offset.y,
                                   target.z + offset.z, 0);
    XMVECTOR focus  = XMVectorSet(target.x, target.y + 1.0f, target.z, 0);
    XMVECTOR up     = XMVectorSet(0, 1, 0, 0);

    XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(60.0f),
        static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT,
        0.1f, 500.0f);
    return view * proj;
}

void BattleScene::Draw()
{
    // バトル: 暗い緑（屋外フィールドイメージ）
    DX11Manager::Get().BeginFrame(0.08f, 0.12f, 0.08f);

    XMMATRIX viewProj = BuildViewProj();

    // 地面（灰色）
    XMMATRIX groundWorld = XMMatrixTranslation(0, -0.1f, 0);
    MeshRenderer::Get().Draw(m_groundMesh, groundWorld, viewProj, { 0.4f, 0.4f, 0.4f, 1.0f });

    // 各プレイヤーを描画
    for (int i = 0; i < m_lastState.count; ++i)
    {
        const auto& snap = m_lastState.snapshots[i];
        if (snap.state != PlayerState::Alive) continue;

        XMFLOAT3 pos = snap.position.ToFloat3();
        XMMATRIX world = XMMatrixRotationY(snap.yaw) *
                         XMMatrixTranslation(pos.x, pos.y + 0.9f, pos.z);

        // 銃陣営: オレンジ、盾陣営: 水色
        bool isLocal = (snap.id == m_localId);
        XMFLOAT4 tint;
        if (isLocal)
            tint = { 1.0f, 1.0f, 0.0f, 1.0f };   // 自分: 黄
        else if (m_localTeam == Team::Gunner)
            tint = { 1.0f, 0.4f, 0.1f, 1.0f };   // 敵: オレンジ
        else
            tint = { 0.2f, 0.7f, 1.0f, 1.0f };   // 敵: 水色

        MeshRenderer::Get().Draw(m_playerMesh, world, viewProj, tint);
    }
}

void BattleScene::Shutdown()
{
    Input::Get().SetCaptureMouse(false, nullptr);
}
