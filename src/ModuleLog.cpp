#include "ModuleLog.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>

static nlohmann::json rowToJson(const std::pair<seasocks::Logger::Level, std::string>& msg) {
    nlohmann::json row;
    row["level"] = seasocks::Logger::levelToString(msg.first);
    row["msg"] = msg.second;
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
        int id = log->appendHandler([connection, log, this] (const std::pair<seasocks::Logger::Level, std::string>& msg) {
            connection->send(rowToJson(msg).dump());
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

        if(id.has_value()) {
            std::unique_lock<std::mutex> logLock{id->first->mMutex};
            id->first->deleteHandler(id->second);
        }
        std::lock_guard<std::shared_mutex> lock2{mConnectionsMutex};
        mConnections.erase(connection);
    }
}

ModuleLogWebsocket::ModuleLogWebsocket() {

}
