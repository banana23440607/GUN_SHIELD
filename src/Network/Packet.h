#pragma once
#include "../Utility/Common.h"
enum class PacketType:uint8_t{Join=0x01,JoinAck=0x02,PlayerList=0x03,TeamStats=0x04,GameStart=0x05,PlayerInput=0x10,PlayerState=0x11,BulletSpawn=0x12,BulletHit=0x13,PlayerDead=0x14,GameEnd=0x20,Ping=0xF0,Pong=0xF1,Disconnect=0xFF};
#pragma pack(push,1)
struct PacketHeader{PacketType type;uint16_t size;uint8_t senderId;};
struct PktJoin{PacketHeader header;char name[16];Team preferredTeam;};
struct PktJoinAck{PacketHeader header;uint8_t assignedId;Team assignedTeam;bool accepted;};
struct PlayerInfo{uint8_t id;Team team;bool ready;char name[16];};
struct PktPlayerList{PacketHeader header;uint8_t count;PlayerInfo players[MAX_PLAYERS];};
struct TeamStatus{uint8_t count;float hpMultiplier,speedMultiplier,attackMultiplier,shieldMultiplier;};
struct PktTeamStats{PacketHeader header;TeamStatus gunner,shielder;};
struct PlayerInput{float moveX,moveZ,yaw,pitch;bool fire,shield,jump;};
struct PktPlayerInput{PacketHeader header;PlayerInput input;uint32_t sequence;};
struct PlayerSnapshot{uint8_t id;NetVec3 position;float yaw,hp;PlayerState state;bool shieldActive;};
struct PktPlayerState{PacketHeader header;uint32_t sequence;uint8_t count;PlayerSnapshot snapshots[MAX_PLAYERS];};
struct PktBulletSpawn{PacketHeader header;uint32_t bulletId;uint8_t ownerId;NetVec3 origin,direction;float speed,damage;};
struct PktBulletHit{PacketHeader header;uint32_t bulletId;uint8_t targetId;float damage,remainingHp;};
struct PktPlayerDead{PacketHeader header;uint8_t deadId,killerId;};
struct PktGameEnd{PacketHeader header;Team winner;uint32_t elapsedSec;};
#pragma pack(pop)
