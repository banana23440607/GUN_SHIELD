#include "NetworkBase.h"

bool NetworkBase::InitWinsock()
{
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

void NetworkBase::Shutdown()
{
    WSACleanup();
}

void NetworkBase::SendTo(SOCKET sock, const void* data, int size)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    int sent = 0;
    while (sent < size)
    {
        int r = ::send(sock, reinterpret_cast<const char*>(p + sent), size - sent, 0);
        if (r == SOCKET_ERROR) break;
        sent += r;
    }
}

bool NetworkBase::RecvExact(SOCKET sock, uint8_t* buf, int size)
{
    int received = 0;
    while (received < size)
    {
        int r = ::recv(sock, reinterpret_cast<char*>(buf + received), size - received, 0);
        if (r <= 0) return false;
        received += r;
    }
    return true;
}
