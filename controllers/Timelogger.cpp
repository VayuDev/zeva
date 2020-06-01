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

void Api::Apps::Timelogger::createActivity(const drogon::HttpRequestPtr&,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                           int64_t pTimelogId,
                                           std::string &&pActivityName) {
    drogon::app().getDbClient()->execSqlAsync("INSERT INTO timelog_activity (timelogid, name) VALUES ($1, $2)",
    [callback = std::move(callback)](const drogon::orm::Result& r) mutable {
        callback(genResponse("ok"));
    }, genErrorHandler(callback), pTimelogId, std::move(pActivityName));
}

static void isActivityRunning(int64_t pTimelogId, std::function<void(bool)>&& pCallback, std::function<void(const drogon::HttpResponsePtr &)> errReceiver) {
    drogon::app().getDbClient()->execSqlAsync("SELECT * FROM timelog_entry WHERE timelogid = $1 ORDER BY created DESC LIMIT 1",
    [pCallback = std::move(pCallback)](const drogon::orm::Result& r) mutable {
        if(r.empty()) {
            pCallback(false);
            return;
        }
        pCallback(r.at(0)["duration"].isNull());
    }, genErrorHandler(std::move(errReceiver)), pTimelogId);
}

void Api::Apps::Timelogger::startActivity(const drogon::HttpRequestPtr&,
                                          std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                          int64_t pTimelogId, int64_t pActivityId) {
    isActivityRunning(pTimelogId, [callback=std::move(callback),pTimelogId,pActivityId] (bool pRunning) mutable {
        if(pRunning) {
            callback(genError("Activity is already running!"));
        } else {
            drogon::app().getDbClient()->execSqlAsync("INSERT INTO timelog_entry (timelogid, activityid, created) VALUES ($1, $2, current_timestamp)",
            [callback = std::move(callback)](const drogon::orm::Result& r) mutable {
                callback(genResponse("ok"));
            }, genErrorHandler(callback), pTimelogId, pActivityId);
        }
    }, callback);
}
