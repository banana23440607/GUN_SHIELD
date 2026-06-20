#pragma once
#include "PlayerData.h"

class PlayerManager
{
public:
    void RegisterPlayer(uint8_t id, Team team, const char* name, const TeamStatus& teamStatus);
    void UnregisterPlayer(uint8_t id);

    void Update(float dt);
    void ApplyInput(uint8_t id, const PlayerInput& input);
    void ApplyTeamStats(const TeamStatus& gunnerStatus, const TeamStatus& shielderStatus);

    bool TryShoot(uint8_t shooterId, XMFLOAT3& outOrigin, XMFLOAT3& outDir, float& outDamage);
    void ApplyDamage(uint8_t targetId, float damage, uint8_t& deadKillerId);

    PlayerData*       GetPlayer(uint8_t id);
    const PlayerData* GetPlayer(uint8_t id) const;

    // スナップショット生成（ネットワーク送信用）
    PktPlayerState MakeStatePacket(uint32_t sequence) const;

    int AliveCount(Team team) const;
    int TotalCount(Team team) const;

private:
    std::array<PlayerData, MAX_PLAYERS> m_players;
    std::array<bool, MAX_PLAYERS>       m_active = {};

    static constexpr float FIRE_RATE       = 0.15f;  // 秒/発
    static constexpr float BULLET_RANGE    = 50.0f;  // m
    static constexpr float SHIELD_BLOCK_RATE = 0.8f; // 盾で80%軽減
};
