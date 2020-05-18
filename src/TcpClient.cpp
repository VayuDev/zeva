#include "TcpClient.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdexcept>
#include <Logger.hpp>

static void error(std::string pMsg) {
    char error[512];
    bzero(error, 512);
    strerror_r(errno, error, 512);
    std::string errString;
    if(error[0] != '\0')
        errString = error;
    throw std::runtime_error(pMsg + (errString.empty() ? "" : (": " + errString)));
}

TcpClient::TcpClient(const std::string& pHostname, uint16_t pPort) {
    mSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mSockFd < 0)
        error("Failed to create socket");
    struct hostent *server = gethostbyname(pHostname.c_str());
    if (server == nullptr) {
        error("No such host: " + pHostname);
    }
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(pPort);
    if (connect(mSockFd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("Failed to connect to " + pHostname + ":" + std::to_string(pPort));
    connected = true;
    setTimeout(1000);

    //keepalive
    int val = 1;
    setsockopt(mSockFd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
    log().warning("TcpClient constructed");
}

void TcpClient::sendByte(uint8_t pByte) {
    if(!connected)
        error("Not connected!");
    int ret = write(mSockFd, &pByte, sizeof(pByte));
    if(ret <= 0) {
        if(ret < 0 && errno != EAGAIN)
            connected = false;
        error("Failed to send byte");
    }
}

uint8_t TcpClient::recvByte() {
    uint8_t byte;
    if(!connected)
        error("Not connected!");
    int ret = read(mSockFd, &byte, sizeof(byte));
    if(ret <= 0) {
        if(ret < 0 && errno != EAGAIN)
            connected = false;
        error("Failed to receive byte");
    }
    return byte;
}

void TcpClient::setTimeout(size_t pMilliseconds) {
    struct timeval timeout;
    timeout.tv_sec = pMilliseconds / 1000;
    timeout.tv_usec = (pMilliseconds * 1000) % 1'000'000;

    if (setsockopt(mSockFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                    sizeof(timeout)) < 0)
        error("Failed to set timeout");

    if (setsockopt(mSockFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                    sizeof(timeout)) < 0)
        error("Failed to set timeout");
}

bool TcpClient::isConnected() const {
    return connected;
}

TcpClient::~TcpClient() {
    close(mSockFd);
    connected = false;
    log().warning("TcpClient destructed");
}

void TcpClient::sendString(const char *pStr) {
    if(!connected)
        error("Not connected!");
    auto len = htonl(strlen(pStr));
    int ret = write(mSockFd, &len, sizeof(len));
    if(ret <= 0) {
        if(ret < 0 && errno != EAGAIN)
            connected = false;
        error("Failed to send string length");
    }
    ret = write(mSockFd, pStr, strlen(pStr));
    if(ret <= 0) {
        if(ret < 0 && errno != EAGAIN)
            connected = false;
        error("Failed to send string");
    }
}

std::string TcpClient::recvString() {
    if(!connected)
        error("Not connected!");
    uint32_t len;
    int ret = read(mSockFd, &len, sizeof(len));
    len = ntohl(len);
    if(ret <= 0) {
        if(ret < 0 && errno != EAGAIN)
            connected = false;
        error("Failed to receive string length");
    }
    if(len <= 0 || len > 1024 * 1024 * 10) {
        error("String is too long: " + std::to_string(len));
    }
    char buff[len + 1];
    ret = read(mSockFd, buff, len);
    if(ret <= 0) {
        if(ret < 0 && errno != EAGAIN)
            connected = false;
        error("Failed to receive string");
    }
    buff[len] = '\0';
    return buff;
}

