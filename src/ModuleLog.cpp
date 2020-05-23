#include "ModuleLog.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>
#include <seasocks/Server.h>

static nlohmann::json rowToJson(const std::tuple<seasocks::Logger::Level, time_t, std::string>& msg) {
    nlohmann::json row;
    const auto&[level, timestamp, logmsg] = msg;
    row["level"] = seasocks::Logger::levelToString(level);
    row["msg"] = logmsg;
    row["timestamp"] = timestamp;
    return row;
}

void ModuleLogWebsocket::onConnect(seasocks::WebSocket *connection) {
    (void)connection;
}

void ModuleLogWebsocket::onData(seasocks::WebSocket *connection, const char *data) {
    try {
        auto log = Logger::getLog(data);
        nlohmann::json json = nlohmann::json::array();
        std::unique_lock logLock{log->mMutex};
        for(const auto& msg: log->mLog) {
            json.push_back(rowToJson(msg));
        }
        int id = log->appendHandler([connection, log, this] (const std::tuple<seasocks::Logger::Level, time_t, std::string>& msg) {
            auto msgCpy = msg;
            this->mServer.execute([connection, msgCpy = std::move(msgCpy), this] {
                std::lock_guard<std::shared_mutex> lock{mConnectionsMutex};
                if(mConnections.count(connection) > 0) {
                    auto toSend = rowToJson(msgCpy).dump();
                    connection->send(toSend);
                }

            });
        });
        logLock.unlock();
        connection->send(json.dump());

        auto pair = std::make_pair(log, id);

        std::lock_guard<std::shared_mutex> lock2{mConnectionsMutex};
        mConnections[connection] = std::move(pair);
    } catch(...) {

    }
}

void ModuleLogWebsocket::onDisconnect(seasocks::WebSocket *connection) {
    if(mConnections.count(connection) > 0) {
        auto id = mConnections.at(connection);

        std::unique_lock<std::mutex> logLock{id.first->mMutex};
        id.first->deleteHandler(id.second);
        logLock.unlock();

        std::lock_guard<std::shared_mutex> lock2{mConnectionsMutex};
        mConnections.erase(connection);
    }
}

ModuleLogWebsocket::ModuleLogWebsocket(seasocks::Server& pServer)
: mServer(pServer) {

}
