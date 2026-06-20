#pragma once
#include "../Utility/Common.h"

// DirectX11 デバイス・スワップチェーン管理
class DX11Manager
{
public:
    static DX11Manager& Get()
    {
        static DX11Manager inst;
        return inst;
    }

    bool Init(HWND hwnd);
    void Shutdown();

    void BeginFrame(float r = 0.1f, float g = 0.1f, float b = 0.15f, float a = 1.0f);
    void EndFrame();

    ID3D11Device*           Device()        const { return m_device.Get(); }
    ID3D11DeviceContext*    Context()       const { return m_context.Get(); }
    IDXGISwapChain*         SwapChain()     const { return m_swapChain.Get(); }
    ID3D11RenderTargetView* RTV()           const { return m_rtv.Get(); }
    ID3D11DepthStencilView* DSV()           const { return m_dsv.Get(); }
    HWND                    GetHwnd()       const { return m_hwnd; }

private:
    DX11Manager() = default;
    DX11Manager(const DX11Manager&) = delete;

    bool CreateSwapChain(HWND hwnd);
    bool CreateRenderTarget();
    bool CreateDepthStencil();

    HWND                             m_hwnd = nullptr;
    ComPtr<ID3D11Device>            m_device;
    ComPtr<ID3D11DeviceContext>     m_context;
    ComPtr<IDXGISwapChain>          m_swapChain;
    ComPtr<ID3D11RenderTargetView>  m_rtv;
    ComPtr<ID3D11DepthStencilView>  m_dsv;
    ComPtr<ID3D11Texture2D>         m_depthTex;
};
