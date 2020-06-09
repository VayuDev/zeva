#include "LogWebsocket.hpp"
#include "DrogonUtil.hpp"
#include "Util.hpp"

static Json::Value formatJsonRow(const std::string &pMsg, int pLevel) {
  Json::Value jsonRow;
  jsonRow["msg"] = pMsg;
  jsonRow["level"] = pLevel;
  return jsonRow;
}

void LogWebsocket::handleNewMessage(const drogon::WebSocketConnectionPtr &pConn,
                                    std::string &&pMsg,
                                    const drogon::WebSocketMessageType &) {
  auto start = std::chrono::high_resolution_clock::now();
  std::unique_lock<std::shared_mutex> lock(mConnectionsMutex);
  try {
    mConnections.emplace(pConn, stringToLogLevel(pMsg));
    lock.unlock();

    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "\t";
    std::shared_lock<std::shared_mutex> lockShared(mConnectionsMutex);
    auto str = Json::writeString(wbuilder, mAllLog);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Response preperation took " << (end - start).count() / 1'000'000.0f << "ms\n";
    pConn->send(str);
  } catch (...) {
  }
}

void LogWebsocket::handleNewConnection(
    const drogon::HttpRequestPtr &,
    const drogon::WebSocketConnectionPtr &pConn) {}

void LogWebsocket::handleConnectionClosed(
    const drogon::WebSocketConnectionPtr &pConn) {
  std::lock_guard<std::shared_mutex> lock(mConnectionsMutex);
  mConnections.erase(pConn);
}

void LogWebsocket::newLogMessage(int pLevel, const std::string &pMsg) {
  auto fullJson = formatJsonRow(pMsg, pLevel);

  std::shared_lock<std::shared_mutex> lock(mConnectionsMutex);
  for (auto &conn : mConnections) {
    if (conn.second <= pLevel) {
      conn.first->send(fullJson.toStyledString());
    }
  }
  lock.unlock();
  {
    std::lock_guard<std::shared_mutex> lock2(mConnectionsMutex);
    mAllLog.append(fullJson);
  }
}
void LogWebsocket::init() {
  std::unique_lock<std::shared_mutex> lock(mConnectionsMutex);
  auto result = drogon::app().getDbClient()->execSqlSync("SELECT * FROM log ORDER BY created");
  Json::Value response;
  for (const auto &row : result) {
    response.append(formatJsonRow(row["msg"].as<std::string>(),
                                  row["level"].as<int>()));
  }
  mAllLog = std::move(response);
}
