#pragma once
#include "../Utility/Common.h"

// 頂点レイアウト（VS_Basic.hlsl と一致）
struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT3 nor;
    XMFLOAT2 uv;
};

// cbPerObject (b0)
struct CBPerObject
{
    XMFLOAT4X4 world;
    XMFLOAT4X4 viewProj;
};

// cbPerFrame (b1)
struct CBPerFrame
{
    XMFLOAT3 lightDir;   float pad0;
    XMFLOAT3 lightColor; float pad1;
    XMFLOAT3 ambient;    float pad2;
    XMFLOAT4 tint;
};

// 単純なメッシュ（頂点・インデックスバッファ）
struct Mesh
{
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT                 indexCount = 0;
};

// シェーダー・定数バッファ・サンプラーをまとめて管理
class MeshRenderer
{
public:
    static MeshRenderer& Get()
    {
        static MeshRenderer inst;
        return inst;
    }

    bool Init();
    void Shutdown();

    // 定数バッファを更新して描画
    void Draw(const Mesh& mesh,
              const XMMATRIX& world,
              const XMMATRIX& viewProj,
              const XMFLOAT4& tint = { 1,1,1,1 });

    // プリミティブ生成ユーティリティ
    static Mesh CreateBox(float w, float h, float d);

private:
    MeshRenderer() = default;

    bool CreateShaders();
    bool CreateConstantBuffers();
    bool CreateSamplerAndTexture();

    ComPtr<ID3D11VertexShader>   m_vs;
    ComPtr<ID3D11PixelShader>    m_ps;
    ComPtr<ID3D11InputLayout>    m_layout;
    ComPtr<ID3D11Buffer>         m_cbPerObject;
    ComPtr<ID3D11Buffer>         m_cbPerFrame;
    ComPtr<ID3D11SamplerState>   m_sampler;
    ComPtr<ID3D11ShaderResourceView> m_whiteTex;  // ダミー白テクスチャ
};
