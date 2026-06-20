#pragma once
#include "NetworkBase.h"

class Client : public NetworkBase
{
public:
    bool Connect(const char* host, int port = DEFAULT_PORT);
    bool Connect(const std::string& host, int port = DEFAULT_PORT) { return Connect(host.c_str(), port); }
    bool Start() override { return true; }
    void Stop()  override;
    void Disconnect() { Stop(); }
    void Update() override;

    void SendJoin(const char* name, Team preferred = Team::None);
    void SendJoin(const std::string& name, Team preferred = Team::None) { SendJoin(name.c_str(), preferred); }
    void SendInput(const PlayerInput& input, uint32_t sequence);
    void SendPing();

    uint8_t   LocalPlayerId() const { return m_localId; }
    uint8_t   GetLocalId()    const { return m_localId; }
    Team      LocalTeam()     const { return m_localTeam; }
    Team      GetLocalTeam()  const { return m_localTeam; }
    bool      IsConnected()   const { return m_sock != INVALID_SOCKET; }

    PacketQueue& RecvQueue() { return m_recvQueue; }

private:
    void RecvLoop();

    SOCKET      m_sock      = INVALID_SOCKET;
    uint8_t     m_localId   = 0xFF;
    Team        m_localTeam = Team::None;
    std::thread m_recvThread;
    uint32_t    m_sequence  = 0;
};
