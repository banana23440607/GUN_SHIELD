#pragma once
#include "../../Utility/Common.h"
#include "../../Network/Packet.h"
struct BaseStats{float maxHp=100,moveSpeed=5,damage=25,shieldDurability=200;};
inline BaseStats GetBaseStats(Team t){BaseStats s;if(t==Team::Gunner){s.maxHp=80;s.moveSpeed=5.5f;s.damage=30;}else{s.maxHp=120;s.moveSpeed=4.5f;s.shieldDurability=250;}return s;}
struct RuntimeStats{
    float maxHp,currentHp,moveSpeed,damage,shieldDurability,currentShieldDurability;
    static RuntimeStats FromBase(const BaseStats& b,const TeamStatus& ts,Team t){
        RuntimeStats r;r.maxHp=b.maxHp*ts.hpMultiplier;r.currentHp=r.maxHp;r.moveSpeed=b.moveSpeed*ts.speedMultiplier;
        if(t==Team::Gunner){r.damage=b.damage*ts.attackMultiplier;r.shieldDurability=r.currentShieldDurability=0;}
        else{r.damage=0;r.shieldDurability=r.currentShieldDurability=b.shieldDurability*ts.shieldMultiplier;}
        return r;
    }
};
struct PlayerData{
    uint8_t id=0xFF;Team team=Team::None;PlayerState state=PlayerState::Alive;
    char name[16]={};XMFLOAT3 position={0,0,0},velocity={0,0,0};
    float yaw=0,pitch=0;RuntimeStats stats={};bool shieldActive=false;float fireCooldown=0;
};
