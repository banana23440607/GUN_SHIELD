#include "MeshRenderer.h"
#include "DX11Manager.h"
#include <d3dcompiler.h>

bool MeshRenderer::Init()
{
    if (!CreateShaders())         return false;
    if (!CreateConstantBuffers()) return false;
    if (!CreateSamplerAndTexture()) return false;

    // 初期ライト設定
    auto* ctx = DX11Manager::Get().Context();
    CBPerFrame frame = {};
    frame.lightDir   = { 0.577f, -0.577f, 0.577f };
    frame.lightColor = { 1.0f, 1.0f, 1.0f };
    frame.ambient    = { 0.2f, 0.2f, 0.2f };
    frame.tint       = { 1, 1, 1, 1 };
    ctx->UpdateSubresource(m_cbPerFrame.Get(), 0, nullptr, &frame, 0, 0);
    ctx->PSSetConstantBuffers(1, 1, m_cbPerFrame.GetAddressOf());
    ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
    ctx->PSSetShaderResources(0, 1, m_whiteTex.GetAddressOf());

    return true;
}

void MeshRenderer::Shutdown()
{
    m_vs.Reset(); m_ps.Reset(); m_layout.Reset();
    m_cbPerObject.Reset(); m_cbPerFrame.Reset();
    m_sampler.Reset(); m_whiteTex.Reset();
}

void MeshRenderer::Draw(const Mesh& mesh,
                        const XMMATRIX& world,
                        const XMMATRIX& viewProj,
                        const XMFLOAT4& tint)
{
    auto* ctx = DX11Manager::Get().Context();

    // 定数バッファ更新
    CBPerObject obj = {};
    XMStoreFloat4x4(&obj.world,    XMMatrixTranspose(world));
    XMStoreFloat4x4(&obj.viewProj, XMMatrixTranspose(viewProj));
    ctx->UpdateSubresource(m_cbPerObject.Get(), 0, nullptr, &obj, 0, 0);

    // tint を毎フレーム更新
    CBPerFrame frame = {};
    frame.lightDir   = { 0.577f, -0.577f, 0.577f };
    frame.lightColor = { 1.0f, 1.0f, 1.0f };
    frame.ambient    = { 0.2f, 0.2f, 0.2f };
    frame.tint       = tint;
    ctx->UpdateSubresource(m_cbPerFrame.Get(), 0, nullptr, &frame, 0, 0);

    // シェーダーセット
    ctx->VSSetShader(m_vs.Get(), nullptr, 0);
    ctx->PSSetShader(m_ps.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
    ctx->PSSetConstantBuffers(1, 1, m_cbPerFrame.GetAddressOf());
    ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
    ctx->PSSetShaderResources(0, 1, m_whiteTex.GetAddressOf());
    ctx->IASetInputLayout(m_layout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点・インデックスバッファセット
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->DrawIndexed(mesh.indexCount, 0, 0);
}

Mesh MeshRenderer::CreateBox(float w, float h, float d)
{
    float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;

    // 6面 × 4頂点
    Vertex verts[24] = {
        // +Y (上)
        {{ -hw, hh, -hd }, { 0,1,0 }, { 0,0 }},
        {{  hw, hh, -hd }, { 0,1,0 }, { 1,0 }},
        {{  hw, hh,  hd }, { 0,1,0 }, { 1,1 }},
        {{ -hw, hh,  hd }, { 0,1,0 }, { 0,1 }},
        // -Y (下)
        {{ -hw,-hh,  hd }, { 0,-1,0 }, { 0,0 }},
        {{  hw,-hh,  hd }, { 0,-1,0 }, { 1,0 }},
        {{  hw,-hh, -hd }, { 0,-1,0 }, { 1,1 }},
        {{ -hw,-hh, -hd }, { 0,-1,0 }, { 0,1 }},
        // +Z (前)
        {{ -hw,-hh, hd }, { 0,0,1 }, { 0,1 }},
        {{  hw,-hh, hd }, { 0,0,1 }, { 1,1 }},
        {{  hw, hh, hd }, { 0,0,1 }, { 1,0 }},
        {{ -hw, hh, hd }, { 0,0,1 }, { 0,0 }},
        // -Z (後)
        {{  hw,-hh,-hd }, { 0,0,-1 }, { 0,1 }},
        {{ -hw,-hh,-hd }, { 0,0,-1 }, { 1,1 }},
        {{ -hw, hh,-hd }, { 0,0,-1 }, { 1,0 }},
        {{  hw, hh,-hd }, { 0,0,-1 }, { 0,0 }},
        // +X (右)
        {{ hw,-hh, hd }, { 1,0,0 }, { 0,1 }},
        {{ hw,-hh,-hd }, { 1,0,0 }, { 1,1 }},
        {{ hw, hh,-hd }, { 1,0,0 }, { 1,0 }},
        {{ hw, hh, hd }, { 1,0,0 }, { 0,0 }},
        // -X (左)
        {{ -hw,-hh,-hd }, { -1,0,0 }, { 0,1 }},
        {{ -hw,-hh, hd }, { -1,0,0 }, { 1,1 }},
        {{ -hw, hh, hd }, { -1,0,0 }, { 1,0 }},
        {{ -hw, hh,-hd }, { -1,0,0 }, { 0,0 }},
    };

    uint16_t idx[36];
    for (int f = 0; f < 6; ++f)
    {
        int b = f * 6, v = f * 4;
        idx[b+0] = v;   idx[b+1] = v+1; idx[b+2] = v+2;
        idx[b+3] = v;   idx[b+4] = v+2; idx[b+5] = v+3;
    }

    auto* dev = DX11Manager::Get().Device();
    Mesh mesh;

    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth = sizeof(verts);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vd = { verts };
    dev->CreateBuffer(&vbd, &vd, mesh.vertexBuffer.GetAddressOf());

    D3D11_BUFFER_DESC ibd = {};
    ibd.ByteWidth = sizeof(idx);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA id2 = { idx };
    dev->CreateBuffer(&ibd, &id2, mesh.indexBuffer.GetAddressOf());

    mesh.indexCount = 36;
    return mesh;
}

bool MeshRenderer::CreateShaders()
{
    auto* dev = DX11Manager::Get().Device();
    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;

    // シェーダーファイルパス
    HRESULT hr = D3DCompileFromFile(L"assets/Shaders/VS_Basic.hlsl", nullptr, nullptr,
        "main", "vs_5_0", D3DCOMPILE_DEBUG, 0, vsBlob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = D3DCompileFromFile(L"assets/Shaders/PS_Basic.hlsl", nullptr, nullptr,
        "main", "ps_5_0", D3DCOMPILE_DEBUG, 0, psBlob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) return false;

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vs.GetAddressOf());
    dev->CreatePixelShader(psBlob->GetBufferPointer(),  psBlob->GetBufferSize(), nullptr, m_ps.GetAddressOf());

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = dev->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_layout.GetAddressOf());
    return SUCCEEDED(hr);
}

bool MeshRenderer::CreateConstantBuffers()
{
    auto* dev = DX11Manager::Get().Device();

    D3D11_BUFFER_DESC bd = {};
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(CBPerObject);
    if (FAILED(dev->CreateBuffer(&bd, nullptr, m_cbPerObject.GetAddressOf()))) return false;

    bd.ByteWidth = sizeof(CBPerFrame);
    if (FAILED(dev->CreateBuffer(&bd, nullptr, m_cbPerFrame.GetAddressOf()))) return false;

    return true;
}

bool MeshRenderer::CreateSamplerAndTexture()
{
    auto* dev = DX11Manager::Get().Device();

    // サンプラー
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.MaxLOD   = D3D11_FLOAT32_MAX;
    if (FAILED(dev->CreateSamplerState(&sd, m_sampler.GetAddressOf()))) return false;

    // 1x1 白テクスチャ（テクスチャなしでも描画できるダミー）
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = td.Height = 1;
    td.MipLevels = td.ArraySize = 1;
    td.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage     = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    uint32_t white = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA srd = { &white, 4, 0 };
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(dev->CreateTexture2D(&td, &srd, tex.GetAddressOf()))) return false;
    if (FAILED(dev->CreateShaderResourceView(tex.Get(), nullptr, m_whiteTex.GetAddressOf()))) return false;

    return true;
}
