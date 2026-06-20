#include "PlayerManager.h"
#include <cstring>

void PlayerManager::RegisterPlayer(uint8_t id, Team team, const char* name, const TeamStatus& teamStatus)
{
    if (id >= MAX_PLAYERS) return;

    PlayerData& pd = m_players[id];
    pd.id    = id;
    pd.team  = team;
    pd.state = PlayerState::Alive;
    strncpy_s(pd.name, name, 15);

    BaseStats base = GetBaseStats(team);
    pd.stats = RuntimeStats::FromBase(base, teamStatus, team);

    // チーム別スポーン位置（仮）
    if (team == Team::Gunner)
        pd.position = { -20.0f, 0.0f, 0.0f };
    else
        pd.position = {  20.0f, 0.0f, 0.0f };

    m_active[id] = true;
}

void PlayerManager::UnregisterPlayer(uint8_t id)
{
    if (id >= MAX_PLAYERS) return;
    m_active[id] = false;
}

void PlayerManager::Update(float dt)
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!m_active[i]) continue;
        PlayerData& pd = m_players[i];
        if (pd.state != PlayerState::Alive) continue;

        // 射撃クールダウン
        if (pd.fireCooldown > 0.0f)
            pd.fireCooldown = std::max(0.0f, pd.fireCooldown - dt);

        // 位置更新（重力なし版、後でコライダー追加）
        pd.position.x += pd.velocity.x * dt;
        pd.position.y += pd.velocity.y * dt;
        pd.position.z += pd.velocity.z * dt;
    }
}

void PlayerManager::ApplyInput(uint8_t id, const PlayerInput& input)
{
    if (id >= MAX_PLAYERS || !m_active[id]) return;
    PlayerData& pd = m_players[id];
    if (pd.state != PlayerState::Alive) return;

    float speed = pd.stats.moveSpeed;

    // ヨー角から移動ベクトルを計算
    float rad = pd.yaw * (XM_PI / 180.0f);
    float fw_x = sinf(rad);
    float fw_z = cosf(rad);
    float rt_x = cosf(rad);
    float rt_z = -sinf(rad);

    pd.velocity.x = (fw_x * input.moveZ + rt_x * input.moveX) * speed;
    pd.velocity.z = (fw_z * input.moveZ + rt_z * input.moveX) * speed;
    pd.velocity.y = input.jump ? 5.0f : 0.0f;

    pd.yaw   = input.yaw;
    pd.pitch = input.pitch;

    if (pd.team == Team::Shielder)
        pd.shieldActive = input.shield;
}

void PlayerManager::ApplyTeamStats(const TeamStatus& gunnerStatus, const TeamStatus& shielderStatus)
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!m_active[i]) continue;
        PlayerData& pd = m_players[i];
        if (pd.state != PlayerState::Alive) continue;

        const TeamStatus& ts = (pd.team == Team::Gunner) ? gunnerStatus : shielderStatus;
        BaseStats base = GetBaseStats(pd.team);

        float hpRatio = pd.stats.currentHp / pd.stats.maxHp;  // HP割合を保持

        pd.stats = RuntimeStats::FromBase(base, ts, pd.team);
        pd.stats.currentHp = pd.stats.maxHp * hpRatio;
    }
}

bool PlayerManager::TryShoot(uint8_t shooterId, XMFLOAT3& outOrigin, XMFLOAT3& outDir, float& outDamage)
{
    if (shooterId >= MAX_PLAYERS || !m_active[shooterId]) return false;
    PlayerData& pd = m_players[shooterId];

    if (pd.team != Team::Gunner) return false;
    if (pd.state != PlayerState::Alive) return false;
    if (pd.fireCooldown > 0.0f) return false;

    pd.fireCooldown = FIRE_RATE;

    float yawRad   = pd.yaw   * (XM_PI / 180.0f);
    float pitchRad = pd.pitch * (XM_PI / 180.0f);

    outOrigin = pd.position;
    outOrigin.y += 1.5f;  // 目線の高さ

    outDir.x = sinf(yawRad) * cosf(pitchRad);
    outDir.y = -sinf(pitchRad);
    outDir.z = cosf(yawRad) * cosf(pitchRad);

    outDamage = pd.stats.damage;
    return true;
}

void PlayerManager::ApplyDamage(uint8_t targetId, float damage, uint8_t& deadKillerId)
{
    deadKillerId = 0xFF;
    if (targetId >= MAX_PLAYERS || !m_active[targetId]) return;
    PlayerData& pd = m_players[targetId];
    if (pd.state != PlayerState::Alive) return;

    if (pd.team == Team::Shielder && pd.shieldActive)
    {
        float shieldDmg = damage * SHIELD_BLOCK_RATE;
        float bodyDmg   = damage * (1.0f - SHIELD_BLOCK_RATE);

        pd.stats.currentShieldDurability -= shieldDmg;
        if (pd.stats.currentShieldDurability <= 0.0f)
        {
            // 盾が壊れた余剰ダメージをHPへ
            float overflow = -pd.stats.currentShieldDurability;
            pd.stats.currentShieldDurability = 0.0f;
            pd.shieldActive = false;
            pd.stats.currentHp -= (bodyDmg + overflow);
        }
        else
        {
            pd.stats.currentHp -= bodyDmg;
        }
    }
    else
    {
        pd.stats.currentHp -= damage;
    }

    if (pd.stats.currentHp <= 0.0f)
    {
        pd.stats.currentHp = 0.0f;
        pd.state = PlayerState::Dead;
    }
}

PlayerData* PlayerManager::GetPlayer(uint8_t id)
{
    if (id >= MAX_PLAYERS || !m_active[id]) return nullptr;
    return &m_players[id];
}

const PlayerData* PlayerManager::GetPlayer(uint8_t id) const
{
    if (id >= MAX_PLAYERS || !m_active[id]) return nullptr;
    return &m_players[id];
}

PktPlayerState PlayerManager::MakeStatePacket(uint32_t sequence) const
{
    PktPlayerState pkt = {};
    pkt.header.type     = PacketType::PlayerState;
    pkt.header.senderId = 0;

    uint8_t count = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!m_active[i]) continue;
        const PlayerData& pd = m_players[i];
        PlayerSnapshot& snap = pkt.snapshots[count++];
        snap.id           = pd.id;
        snap.position     = NetVec3(pd.position);
        snap.yaw          = pd.yaw;
        snap.hp           = pd.stats.currentHp;
        snap.state        = pd.state;
        snap.shieldActive = pd.shieldActive;
    }
    pkt.count    = count;
    pkt.sequence = sequence;
    pkt.header.size = sizeof(PktPlayerState);
    return pkt;
}

int PlayerManager::AliveCount(Team team) const
{
    int c = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!m_active[i]) continue;
        if (m_players[i].team == team && m_players[i].state == PlayerState::Alive)
            ++c;
    }
    return c;
}

int PlayerManager::TotalCount(Team team) const
{
    int c = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!m_active[i]) continue;
        if (m_players[i].team == team) ++c;
    }
    return c;
}
