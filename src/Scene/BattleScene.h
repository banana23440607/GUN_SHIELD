#pragma once
#include "../Utility/Common.h"
#include "SceneBase.h"
#include "../Network/Client.h"
#include "../Network/Server.h"
#include "../Game/Player/PlayerManager.h"
#include "../Network/Packet.h"
#include "../Graphics/MeshRenderer.h"
#include <array>

// バトルシーン
// ホスト (isHost=true) はサーバー権限も持ち、60fps でゲームループを回す
class BattleScene : public SceneBase
{
public:
    explicit BattleScene(bool isHost, Client* client, Server* server = nullptr);

    bool Init()     override;
    void Update(float dt) override;
    void Draw()     override;
    void Shutdown() override;

    static constexpr float BATTLE_TIME = 180.0f;

private:
    void ServerUpdate(float dt);
    void ProcessClientPackets();
    void CheckWinCondition();

    bool     m_isHost;
    Client*  m_client;
    Server*  m_server;

    PlayerManager  m_playerManager;
    float          m_timeRemaining = BATTLE_TIME;
    float          m_fixedAccum    = 0.0f;
    bool           m_gameOver      = false;
    Team           m_winner        = Team::None;

    PktPlayerState m_lastState = {};
    uint8_t        m_localId   = 0xFF;
    Team           m_localTeam = Team::None;

    uint32_t m_inputSequence = 0;

    // 描画
    Mesh   m_playerMesh;   // 全プレイヤー共通のボックスメッシュ
    Mesh   m_groundMesh;

    // TPS カメラ
    XMFLOAT3 m_camPos   = { 0, 5, -10 };
    XMMATRIX BuildViewProj() const;
};
