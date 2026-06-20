#pragma once
#include "Packet.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

// 受信パケットキュー（スレッドセーフ）
class PacketQueue
{
public:
    void Push(std::vector<uint8_t>&& data)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_queue.push(std::move(data));
    }

    bool Pop(std::vector<uint8_t>& out)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // 戻り値版（空のとき空ベクター）
    std::vector<uint8_t> Pop()
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_queue.empty()) return {};
        auto v = std::move(m_queue.front());
        m_queue.pop();
        return v;
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_queue.empty();
    }

private:
    std::queue<std::vector<uint8_t>> m_queue;
    mutable std::mutex               m_mtx;
};

// ネットワーク基底クラス
class NetworkBase
{
public:
    virtual ~NetworkBase() { Shutdown(); }

    bool InitWinsock();
    void Shutdown();

    virtual bool Start() = 0;
    virtual void Stop()  = 0;

    // メインスレッドから毎フレーム呼ぶ
    virtual void Update() = 0;

    bool IsRunning() const { return m_running.load(); }

protected:
    void SendTo(SOCKET sock, const void* data, int size);
    bool RecvExact(SOCKET sock, uint8_t* buf, int size);

    PacketQueue       m_recvQueue;
    std::atomic<bool> m_running{ false };
};
