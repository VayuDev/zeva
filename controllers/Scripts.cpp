#include "Scripts.hpp"
#include "Util.hpp"

void Api::Scripts::getAllScripts(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto call = callback;
    drogon::app().getDbClient()->execSqlAsync("SELECT * FROM scripts",
    [call](const drogon::orm::Result& r) {
        Json::Value ret;
        for(const auto& row: r) {
            Json::Value jsonRow;
            jsonRow["id"] = row["id"].as<std::string>();
            jsonRow["name"] = row["name"].as<std::string>();
            ret.append(std::move(jsonRow));
        }
        auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(ret));
        call(resp);
    }, genErrorHandler(callback));
}

void Api::Scripts::getScript(const drogon::HttpRequestPtr &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback, int scriptid) {
    auto call = callback;
    drogon::app().getDbClient()->execSqlAsync("SELECT * FROM scripts WHERE id=$1",
    [call](const drogon::orm::Result& r) {
        Json::Value jsonRow;
        jsonRow["id"] = r.at(0)["id"].as<std::string>();
        jsonRow["name"] = r.at(0)["name"].as<std::string>();
        jsonRow["code"] = r.at(0)["code"].as<std::string>();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(jsonRow));
        call(resp);
    }, genErrorHandler(callback), scriptid);
}
