#include "SystemMonitor.hpp"
#include "DrogonUtil.hpp"

void Api::Db::SystemMonitor::getConfig(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&deviceName) {
  Json::Value config;
  config["refresh_interval"] = 0;
  config["measure_duration"] = 2 * 60 * 1000;
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT id FROM system_monitor_device WHERE name=$1;",
      [callback, deviceName, config](const drogon::orm::Result &res) mutable {
        if (res.empty()) {
          drogon::app().getDbClient()->execSqlAsync(
              "INSERT INTO system_monitor_device (name) VALUES ($1) RETURNING "
              "id;",
              [callback, config](const drogon::orm::Result &res) mutable {
                config["id"] = res.at(0)["id"].as<int64_t>();
                callback(drogon::HttpResponse::newHttpJsonResponse(
                    std::move(config)));
              },
              genErrorHandler(callback), deviceName);
        } else {
          config["id"] = res.at(0)["id"].as<int64_t>();
          callback(
              drogon::HttpResponse::newHttpJsonResponse(std::move(config)));
        }
      },
      genErrorHandler(callback), deviceName);
}
