#pragma once
#include "../../Utility/Common.h"
#include "../../Network/Packet.h"
class TeamBalancer{
public:
    void Update(int g,int s);
    const TeamStatus& GetGunnerStatus()const{return m_gunner;}
    const TeamStatus& GetShielderStatus()const{return m_shielder;}
    PktTeamStats MakePacket()const;
private:
    float CalcMultiplier(int d)const;
    TeamStatus m_gunner={},m_shielder={};
    static constexpr float BUFF_PER_PERSON=0.15f,MAX_BUFF=0.60f;
};
