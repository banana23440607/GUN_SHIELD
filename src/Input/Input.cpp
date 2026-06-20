#include "Input.h"
#include <algorithm>

void Input::Update()
{
    // 前フレームのキー状態を保存
    memcpy(m_prevKeys, m_keys, sizeof(m_keys));
    GetKeyboardState(m_keys);

    // マウス角速度（生入力から）
    m_yawDelta   = static_cast<float>(m_rawDx) * MOUSE_SENSITIVITY;
    m_pitchDelta = static_cast<float>(m_rawDy) * MOUSE_SENSITIVITY;
    m_rawDx = m_rawDy = 0;

    m_yaw   += m_yawDelta;
    m_pitch += m_pitchDelta;
    m_pitch  = std::clamp(m_pitch, -PITCH_LIMIT, PITCH_LIMIT);

    // マウスキャプチャ中はカーソルを中央に戻す
    if (m_captureMouse && m_hwnd)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        ClientToScreen(m_hwnd, &center);
        SetCursorPos(center.x, center.y);
    }
}

void Input::OnMouseMove(LPARAM lParam)
{
    (void)lParam;
}

void Input::OnMouseRawDelta(int dx, int dy)
{
    m_rawDx += dx;
    m_rawDy += dy;
}

PlayerInput Input::ToPlayerInput() const
{
    PlayerInput pi = {};

    // WASD 移動
    if (IsDown('W')) pi.moveZ += 1.0f;
    if (IsDown('S')) pi.moveZ -= 1.0f;
    if (IsDown('D')) pi.moveX += 1.0f;
    if (IsDown('A')) pi.moveX -= 1.0f;

    // 正規化（斜め移動で速度が増えないように）
    float len = sqrtf(pi.moveX * pi.moveX + pi.moveZ * pi.moveZ);
    if (len > 1.0f) { pi.moveX /= len; pi.moveZ /= len; }

    pi.yaw   = m_yaw;
    pi.pitch = m_pitch;
    pi.fire   = IsDown(VK_LBUTTON);
    pi.shield = IsDown(VK_RBUTTON);
    pi.jump   = IsPressed(VK_SPACE);

    return pi;
}

void Input::SetCaptureMouse(bool capture, HWND hwnd)
{
    m_captureMouse = capture;
    m_hwnd         = hwnd;
    ShowCursor(!capture);

    if (capture)
    {
        RAWINPUTDEVICE rid = {};
        rid.usUsagePage = 0x01;
        rid.usUsage     = 0x02;  // マウス
        rid.dwFlags     = RIDEV_INPUTSINK;
        rid.hwndTarget  = hwnd;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));
    }
    else
    {
        RAWINPUTDEVICE rid = {};
        rid.usUsagePage = 0x01;
        rid.usUsage     = 0x02;
        rid.dwFlags     = RIDEV_REMOVE;
        rid.hwndTarget  = nullptr;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));
    }
}
