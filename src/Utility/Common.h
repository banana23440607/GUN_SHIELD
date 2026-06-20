#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"ws2_32.lib")
using Microsoft::WRL::ComPtr;
using namespace DirectX;
constexpr int   MAX_PLAYERS   =8;
constexpr int   MAX_GUNNERS   =4;
constexpr int   MAX_SHIELDERS =4;
constexpr UINT  SCREEN_WIDTH  =1280;
constexpr UINT  SCREEN_HEIGHT =720;
constexpr int   DEFAULT_PORT  =54321;
enum class Team        :uint8_t{Gunner=0,Shielder=1,None=0xFF};
enum class PlayerState :uint8_t{Alive=0,Dead=1,Spectating=2};
enum class GamePhase   :uint8_t{Lobby=0,Countdown=1,Battle=2,Result=3};
struct NetVec3{float x,y,z;NetVec3():x(0),y(0),z(0){}NetVec3(XMFLOAT3 v):x(v.x),y(v.y),z(v.z){}XMFLOAT3 ToFloat3()const{return{x,y,z};}  };
