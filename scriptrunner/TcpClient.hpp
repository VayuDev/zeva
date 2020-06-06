#pragma once
#include <string>
#include <cstdint>

class TcpClient final {
public:
    TcpClient(const std::string& pHostname, uint16_t pPort);
    ~TcpClient();
    void sendByte(uint8_t pByte);
    uint8_t recvByte();
    void sendString(const char* pStr);
    std::string recvString();
    void setTimeout(size_t pMilliseconds);
    [[nodiscard]] bool isConnected() const;
private:
    bool connected = false;
    int mSockFd;
};