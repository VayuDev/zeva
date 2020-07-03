#include "Browser.hpp"
#include "DrogonUtil.hpp"

void Api::Apps::Browser::addVisitedUrl(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&browserName, std::string &&url) {
  // slightly modified exception callback to always add:
  // Access-Control-Allow-Origin: *
  auto onException = [callback](const drogon::orm::DrogonDbException &e) {
    auto resp = genError(e.base().what());
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
  };
  // get the id of the browser by its name
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT id FROM browser_name WHERE name=$1",
      [=](const drogon::orm::Result &res) mutable {
        // this callback will be execute later to insert the row based on its id
        auto insertRow = [url = std::move(url), onException,
                          callback](int64_t browserId) {
          drogon::app().getDbClient()->execSqlAsync(
              "INSERT INTO browser_visit (browserid, time, url) VALUES ($1, "
              "CURRENT_TIMESTAMP, $2)",
              [=](const drogon::orm::Result &) mutable {
                auto resp = genResponse("ok");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
              },
              onException, browserId, url);
        };
        // we need to add the browser to obtain a new id for it
        if (res.empty()) {
          drogon::app().getDbClient()->execSqlAsync(
              "INSERT INTO browser_name (name) VALUES ($1) RETURNING id",
              [callback, insertRow, browserName](const drogon::orm::Result &res) {
                LOG_INFO << "[Browser Integration] Added new name: " << browserName;
                insertRow(res.at(0)["id"].as<int64_t>());
              },
              onException, browserName);
        }
        // we already got the id, now insert it
        insertRow(res.at(0)["id"].as<int64_t>());
      },
      onException, browserName);
}