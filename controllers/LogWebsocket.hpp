#pragma once
#include <drogon/WebSocketController.h>
#include <shared_mutex>
#include <pqxx/pqxx>

class LogWebsocket : public drogon::WebSocketController<LogWebsocket, false> {
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr&, std::string &&, const drogon::WebSocketMessageType &) override;
    void handleNewConnection(const drogon::HttpRequestPtr &, const drogon::WebSocketConnectionPtr&) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr&) override;

    void newLogMessage(int pLevel, const std::string& pMsg);

    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/api/log/ws_log");
    WS_PATH_LIST_END
private:
    std::shared_mutex mConnectionsMutex;
    std::map<drogon::WebSocketConnectionPtr, trantor::Logger::LogLevel> mConnections;
};
