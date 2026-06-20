#include "DX11Manager.h"

bool DX11Manager::Init(HWND hwnd)
{
    m_hwnd = hwnd;
    if (!CreateSwapChain(hwnd)) return false;
    if (!CreateRenderTarget())  return false;
    if (!CreateDepthStencil())  return false;

    D3D11_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(SCREEN_WIDTH);
    vp.Height   = static_cast<float>(SCREEN_HEIGHT);
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    return true;
}

void DX11Manager::Shutdown()
{
    m_context->ClearState();
}

void DX11Manager::BeginFrame(float r, float g, float b, float a)
{
    float color[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_rtv.Get(), color);
    m_context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), m_dsv.Get());
}

void DX11Manager::EndFrame()
{
    m_swapChain->Present(1, 0);
}

bool DX11Manager::CreateSwapChain(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = SCREEN_WIDTH;
    sd.BufferDesc.Height                  = SCREEN_HEIGHT;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, featureLevels, 1, D3D11_SDK_VERSION,
        &sd, m_swapChain.GetAddressOf(),
        m_device.GetAddressOf(), nullptr,
        m_context.GetAddressOf());

    return SUCCEEDED(hr);
}

bool DX11Manager::CreateRenderTarget()
{
    ComPtr<ID3D11Texture2D> backBuf;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuf.GetAddressOf()));
    if (FAILED(hr)) return false;
    hr = m_device->CreateRenderTargetView(backBuf.Get(), nullptr, m_rtv.GetAddressOf());
    return SUCCEEDED(hr);
}

bool DX11Manager::CreateDepthStencil()
{
    D3D11_TEXTURE2D_DESC td = {};
    td.Width              = SCREEN_WIDTH;
    td.Height             = SCREEN_HEIGHT;
    td.MipLevels          = 1;
    td.ArraySize          = 1;
    td.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.SampleDesc.Count   = 1;
    td.Usage              = D3D11_USAGE_DEFAULT;
    td.BindFlags          = D3D11_BIND_DEPTH_STENCIL;

    HRESULT hr = m_device->CreateTexture2D(&td, nullptr, m_depthTex.GetAddressOf());
    if (FAILED(hr)) return false;
    hr = m_device->CreateDepthStencilView(m_depthTex.Get(), nullptr, m_dsv.GetAddressOf());
    return SUCCEEDED(hr);
}
