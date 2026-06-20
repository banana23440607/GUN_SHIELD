cbuffer cbPerFrame:register(b1){float3 gLightDir;float p0;float3 gLightColor;float p1;float3 gAmbient;float p2;float4 gTint;};
Texture2D gTex:register(t0);SamplerState gSampler:register(s0);
struct PSIn{float4 posH:SV_POSITION;float3 posW:POSITION;float3 norW:NORMAL;float2 uv:TEXCOORD;};
float4 main(PSIn p):SV_TARGET{float3 n=normalize(p.norW);float d=max(dot(n,-gLightDir),0);float4 t=gTex.Sample(gSampler,p.uv);return float4(t.rgb*(gAmbient+gLightColor*d),t.a)*gTint;}
