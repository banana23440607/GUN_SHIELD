#pragma once
#include "SceneBase.h"
#include "../Network/Client.h"
#include "../Network/Server.h"
#include "../Network/Packet.h"
#include <string>

// LobbyScene: ホストはサーバーを起動し、クライアントは接続して待機
// スペースキーでゲーム開始（ホストのみ）
class LobbyScene : public SceneBase
{
public:
    explicit LobbyScene(bool isHost, const std::string& playerName,
                        const std::string& hostIp = "127.0.0.1");
    ~LobbyScene() override;

    bool Init()     override;
    void Update(float dt) override;
    void Draw()     override;
    void Shutdown() override;

private:
    void ProcessPackets();

    bool        m_isHost;
    std::string m_playerName;
    std::string m_hostIp;

    std::unique_ptr<Server> m_server;   // ホスト時のみ使用
    std::unique_ptr<Client> m_client;

    PktPlayerList  m_playerList   = {};
    PktTeamStats   m_teamStats    = {};
    bool           m_gameStarted  = false;
    float          m_countdown    = 0.0f;
};
