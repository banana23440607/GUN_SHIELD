#include "Application.h"
int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE,LPWSTR,int){
    auto& app=Application::Get();
    if(!app.Init(hInst)){MessageBoxW(nullptr,L"初期化に失敗しました",L"エラー",MB_OK|MB_ICONERROR);return -1;}
    int r=app.Run();app.Shutdown();return r;
}
