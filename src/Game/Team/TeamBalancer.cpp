#include "TeamBalancer.h"
#include <algorithm>
void TeamBalancer::Update(int g,int s){
    m_gunner.count=(uint8_t)g;m_shielder.count=(uint8_t)s;
    auto reset=[](TeamStatus& t){t.hpMultiplier=t.speedMultiplier=t.attackMultiplier=t.shieldMultiplier=1.0f;};
    int d=g-s;
    if(d==0){reset(m_gunner);reset(m_shielder);}
    else if(d>0){reset(m_gunner);float b=CalcMultiplier(d);m_shielder.hpMultiplier=1+b;m_shielder.speedMultiplier=1+b*0.8f;m_shielder.attackMultiplier=1;m_shielder.shieldMultiplier=1+b;}
    else{reset(m_shielder);float b=CalcMultiplier(-d);m_gunner.hpMultiplier=1+b;m_gunner.speedMultiplier=1+b*0.8f;m_gunner.attackMultiplier=1+b;m_gunner.shieldMultiplier=1;}
}
float TeamBalancer::CalcMultiplier(int d)const{return std::min(BUFF_PER_PERSON*(float)d,MAX_BUFF);}
PktTeamStats TeamBalancer::MakePacket()const{PktTeamStats p={};p.header={PacketType::TeamStats,sizeof(PktTeamStats),0};p.gunner=m_gunner;p.shielder=m_shielder;return p;}
