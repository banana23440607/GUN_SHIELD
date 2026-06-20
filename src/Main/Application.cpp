#include "Application.h"
#include "../Graphics/DX11Manager.h"
#include "../Scene/SceneManager.h"
#include "../Scene/LobbyScene.h"
#include "../Input/Input.h"

bool Application::Init(HINSTANCE hInst)
{
    if (!CreateAppWindow(hInst)) return false;
    if (!DX11Manager::Get().Init(m_hwnd)) return false;

    RegisterScenes();
    SceneManager::Get().Change(SceneID::Lobby);

    m_isRunning = true;
    return true;
}

int Application::Run()
{
    MSG msg = {};
    while (m_isRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) { m_isRunning = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!m_isRunning) break;

        m_timer.Tick();
        float dt = m_timer.DeltaTime();

        // フレームレート制限（最大120fps）
        if (dt < 1.0f / 120.0f) continue;

        DX11Manager::Get().BeginFrame();
        SceneManager::Get().Update(dt);
        SceneManager::Get().Draw();
        DX11Manager::Get().EndFrame();
    }

    return static_cast<int>(msg.wParam);
}

void Application::Shutdown()
{
    DX11Manager::Get().Shutdown();
}

bool Application::CreateAppWindow(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"GunShieldWnd";
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect(&rc, style, FALSE);

    m_hwnd = CreateWindowExW(
        0, L"GunShieldWnd", L"GUN SHIELD",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);

    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    return true;
}

void Application::RegisterScenes()
{
    // ホストモードでロビーを開始（コマンドライン引数で切替可能にする予定）
    SceneManager::Get().RegisterFactory(SceneID::Lobby, []
    {
        return std::make_unique<LobbyScene>(true, "Player1");
    });
    // Title/Battle/Result は今後実装
}

LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    case WM_INPUT:
    {
        UINT size = 0;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size > 0)
        {
            std::vector<uint8_t> buf(size);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, buf.data(), &size, sizeof(RAWINPUTHEADER)) == size)
            {
                const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buf.data());
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    Input::Get().OnMouseRawDelta(
                        raw->data.mouse.lLastX,
                        raw->data.mouse.lLastY);
                }
            }
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
