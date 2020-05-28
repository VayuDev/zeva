#include "Timelogger.hpp"
#include "Util.hpp"

void Api::Apps::Timelogger::getStatus(const drogon::HttpRequestPtr&,
                                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                      int64_t pSubid) {
    drogon::app().getDbClient()->execSqlAsync("SELECT name FROM timelog WHERE id=$1",
    [callback = std::move(callback), pSubid](const drogon::orm::Result& r) mutable {
        if(r.size() == 1) {
            Json::Value resp;
            resp["name"] = r.at(0)["name"].as<std::string>();

            drogon::app().getDbClient()->execSqlAsync("SELECT * FROM timelog_activity WHERE timelogid=$1",
            [callback = std::move(callback), resp = std::move(resp)](const drogon::orm::Result& r) mutable {
                Json::Value activities{Json::arrayValue};
                for(const auto& row: r) {
                    Json::Value jsonRow;
                    jsonRow["id"] = row["id"].as<int64_t>();
                    jsonRow["name"] = row["name"].as<std::string>();
                    activities.append(std::move(jsonRow));
                }
                resp["activities"] = std::move(activities);
                callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
            }, genErrorHandler(callback), pSubid);
        } else {
            callback(genError("Invalid timelog id"));
        }
    }, genErrorHandler(callback), pSubid);
}

void Api::Apps::Timelogger::createActivity(const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                           int64_t pTimelogId,
                                           std::string &&pActivityName) {
    drogon::app().getDbClient()->execSqlAsync("INSERT INTO timelog_activity (timelogid, name) VALUES ($1, $2)",
    [callback = std::move(callback)](const drogon::orm::Result& r) mutable {
        callback(genResponse("Success!"));
    }, genErrorHandler(callback), pTimelogId, std::move(pActivityName));
}
