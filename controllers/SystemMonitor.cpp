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
void Api::Db::SystemMonitor::addRow(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&data) {
  Json::Value receivedJson;
  std::stringstream receivedStream{data};
  receivedStream >> receivedJson;
  drogon::app().getDbClient()->execSqlAsync(
      "INSERT INTO system_monitor_entry (deviceid, time, total_cpu_usage, "
      "cpu_core_count, uptime, load_1, load_5, load_15, total_ram, free_ram, "
      "shared_ram, buffer_ram, total_swap, free_swap, proc_count, total_high, "
      "free_high) VALUES ($1, current_timestamp, $2, $3, $4, $5, $6, $7, $8, "
      "$9, $10, $11, "
      "$12, $13, $14, $15, $16)",
      [callback](const drogon::orm::Result &res) mutable {

      },
      genErrorHandler(callback), receivedJson["machine_id"].asInt64(),
      receivedJson["total_cpu_usage"].asDouble(),
      receivedJson["cpu_core_count"].asInt(), receivedJson["uptime"].asInt64(),
      receivedJson["load_1"].asDouble(), receivedJson["load_5"].asDouble(),
      receivedJson["load_15"].asDouble(), receivedJson["total_ram"].asInt(),
      receivedJson["free_ram"].asInt(), receivedJson["shared_ram"].asInt(),
      receivedJson["buffer_ram"].asInt(), receivedJson["total_swap"].asInt(),
      receivedJson["free_swap"].asInt(), receivedJson["proc_count"].asInt(),
      receivedJson["total_high"].asInt(), receivedJson["free_high"].asInt());
}
