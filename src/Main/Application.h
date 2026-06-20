#pragma once
#include "../Utility/Common.h"
#include "../Utility/Timer.h"

class Application
{
public:
    static Application& Get()
    {
        static Application inst;
        return inst;
    }

    bool Init(HINSTANCE hInst);
    int  Run();
    void Shutdown();

    HWND GetHwnd() const { return m_hwnd; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    Application() = default;

    bool CreateAppWindow(HINSTANCE hInst);
    void RegisterScenes();
    void MainLoop();

    HWND  m_hwnd = nullptr;
    Timer m_timer;
    bool  m_isRunning = false;
};
