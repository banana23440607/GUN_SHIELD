#pragma once
#include "NetworkBase.h"
#include "../Game/Team/TeamBalancer.h"

// クライアント接続情報
struct ClientSession
{
    SOCKET      sock    = INVALID_SOCKET;
    uint8_t     id      = 0xFF;
    Team        team    = Team::None;
    bool        active  = false;
    char        name[16] = {};
    std::thread recvThread;
};

// サーバークラス（最大8人受付）
class Server : public NetworkBase
{
public:
    bool Start() override { return Start(DEFAULT_PORT); }
    bool Start(int port);
    void Stop()  override;
    void Update() override;

    // ゲームロジックから呼ぶ
    void BroadcastGameStart();
    void BroadcastPlayerStates(const PktPlayerState& pkt);
    void BroadcastBulletSpawn(const PktBulletSpawn& pkt);
    void BroadcastBulletHit(const PktBulletHit& pkt);
    void BroadcastPlayerDead(const PktPlayerDead& pkt);
    void BroadcastGameEnd(Team winner, uint32_t elapsedSec);

    // テンプレート版 Broadcast（任意のパケット型）
    template<typename T>
    void Broadcast(const T& pkt) { Broadcast(&pkt, static_cast<int>(sizeof(T))); }

    PacketQueue& RecvQueue() { return m_recvQueue; }

    int  GunnerCount()   const;
    int  ShielderCount() const;

private:
    void AcceptLoop();
    void RecvLoop(uint8_t clientId);
    void ProcessPacket(const std::vector<uint8_t>& data);

    void HandleJoin(uint8_t clientId, const PktJoin& pkt);
    void SendPlayerList();
    void BroadcastTeamStats();
    uint8_t AssignId();
    Team    AssignTeam(Team preferred);

    void SendTo(uint8_t clientId, const void* data, int size);
    void Broadcast(const void* data, int size, uint8_t excludeId = 0xFF);

    SOCKET                              m_listenSock = INVALID_SOCKET;
    std::array<ClientSession, MAX_PLAYERS> m_clients;
    std::thread                         m_acceptThread;
    std::mutex                          m_clientMtx;
    TeamBalancer                        m_balancer;
    uint8_t                             m_nextId = 0;
};
