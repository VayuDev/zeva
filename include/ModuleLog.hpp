#pragma once
#include <seasocks/WebSocket.h>
#include <set>
#include <shared_mutex>

class ModuleLogWebsocket : public seasocks::WebSocket::Handler {
public:
    ModuleLogWebsocket();
    void onConnect(seasocks::WebSocket* connection) override;
    void onData(seasocks::WebSocket* connection, const char* data) override;
    void onDisconnect(seasocks::WebSocket* connection) override;
private:
    std::map<seasocks::WebSocket*, std::pair<std::shared_ptr<class Logger>, int>> mConnections;
    std::shared_mutex mConnectionsMutex;
};