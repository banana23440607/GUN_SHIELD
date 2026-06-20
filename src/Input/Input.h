#pragma once
#include "../Utility/Common.h"
#include "../Network/Packet.h"

// キーボード＋マウス入力をまとめて管理するシングルトン
class Input
{
public:
    static Input& Get()
    {
        static Input inst;
        return inst;
    }

    // メインループから毎フレーム呼ぶ
    void Update();

    // WndProc からのメッセージを受け取る
    void OnMouseMove(LPARAM lParam);
    void OnMouseRawDelta(int dx, int dy);

    // キー状態（仮想キーコード指定）
    bool IsDown(int vk)     const { return (m_keys[vk] & 0x80) != 0; }
    bool IsPressed(int vk)  const { return IsDown(vk) && !(m_prevKeys[vk] & 0x80); }
    bool IsReleased(int vk) const { return !IsDown(vk) && (m_prevKeys[vk] & 0x80); }

    // 視点変化量（ピクセル → 角度ラジアン変換済み）
    float GetYawDelta()   const { return m_yawDelta; }
    float GetPitchDelta() const { return m_pitchDelta; }

    // 現在の累積視点角度
    float GetYaw()   const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

    // PlayerInput 構造体へ変換
    PlayerInput ToPlayerInput() const;

    // マウスカーソルをウィンドウ中央に固定する（バトル中）
    void SetCaptureMouse(bool capture, HWND hwnd);

private:
    Input() = default;

    uint8_t m_keys[256]     = {};
    uint8_t m_prevKeys[256] = {};

    int   m_rawDx = 0, m_rawDy = 0;
    float m_yawDelta   = 0.0f;
    float m_pitchDelta = 0.0f;
    float m_yaw        = 0.0f;
    float m_pitch      = 0.0f;

    bool  m_captureMouse = false;
    HWND  m_hwnd         = nullptr;

    static constexpr float MOUSE_SENSITIVITY = 0.002f;  // rad/pixel
    static constexpr float PITCH_LIMIT       = XM_PIDIV2 * 0.9f;
};
