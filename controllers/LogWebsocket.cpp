#include "LogWebsocket.hpp"
#include "Util.hpp"
#include "DrogonUtil.hpp"

static Json::Value formatJsonRow(const std::string& pMsg, int pLevel) {
    Json::Value jsonRow;
    jsonRow["msg"] = pMsg;
    jsonRow["level"] = pLevel;
    return jsonRow;
}

void LogWebsocket::handleNewMessage(const drogon::WebSocketConnectionPtr& pConn, std::string&& pMsg, const drogon::WebSocketMessageType &) {
    std::lock_guard<std::shared_mutex> lock(mConnectionsMutex);
    try {
        mConnections.emplace(pConn, stringToLogLevel(pMsg));
        drogon::app().getDbClient()->execSqlAsync("SELECT * FROM log ORDER BY created",
        [this, conn = pConn](const drogon::orm::Result& r) {
            Json::Value response;
            for(const auto& row: r) {
                response.append(formatJsonRow(row["msg"].as<std::string>(), row["level"].as<int>()));
            }
            std::shared_lock<std::shared_mutex> lock(mConnectionsMutex);
            conn->send(response.toStyledString());
        },
        [](const drogon::orm::DrogonDbException& e) {

        });
    } catch(...) {}
}

void LogWebsocket::handleNewConnection(const drogon::HttpRequestPtr&, const drogon::WebSocketConnectionPtr& pConn) {


}

void LogWebsocket::handleConnectionClosed(const drogon::WebSocketConnectionPtr& pConn) {
    std::lock_guard<std::shared_mutex> lock(mConnectionsMutex);
    mConnections.erase(pConn);
}

void LogWebsocket::newLogMessage(int pLevel, const std::string &pMsg) {
    auto fullJsonString = formatJsonRow(pMsg, pLevel).toStyledString();

    std::shared_lock<std::shared_mutex> lock(mConnectionsMutex);
    for(auto& conn: mConnections) {
        if(conn.second <= pLevel) {
            conn.first->send(fullJsonString);
        }
    }
}
