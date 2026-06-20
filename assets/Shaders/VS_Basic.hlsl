cbuffer cbPerObject:register(b0){float4x4 gWorld;float4x4 gViewProj;};
struct VSIn{float3 pos:POSITION;float3 nor:NORMAL;float2 uv:TEXCOORD;};
struct VSOut{float4 posH:SV_POSITION;float3 posW:POSITION;float3 norW:NORMAL;float2 uv:TEXCOORD;};
VSOut main(VSIn v){VSOut o;float4 pw=mul(float4(v.pos,1),gWorld);o.posH=mul(pw,gViewProj);o.posW=pw.xyz;o.norW=mul(v.nor,(float3x3)gWorld);o.uv=v.uv;return o;}
