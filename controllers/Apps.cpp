#include "Apps.hpp"
#include "Util.hpp"

void Api::Apps::getCategories(const drogon::HttpRequestPtr &,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    Json::Value resp;
    auto genRow = [&](const std::string& pName, const std::string& pPath, bool hasSub) {
        Json::Value row;
        row["name"] = pName;
        row["path"] = pPath;
        row["hasSub"] = hasSub;
        resp.append(std::move(row));
    };
    genRow("Timelogger", "timelogger.html", true);
    genRow("Music Player", "player.html", false);
    genRow("Playlist", "playlist.html", true);
    callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
}

void Api::Apps::getSubapps(const drogon::HttpRequestPtr &,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string &&pAppname) {
    if(pAppname == "Timelogger") {
        drogon::app().getDbClient()->execSqlAsync("SELECT * FROM timelog ORDER BY id DESC",
        [callback = std::move(callback)](const drogon::orm::Result& r) {
            Json::Value resp{Json::arrayValue};
            for(const auto& sub: r) {
                Json::Value row;
                row["name"] = sub["name"].as<std::string>();
                row["id"] = sub["id"].as<int64_t>();
                resp.append(std::move(row));
            }
            callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
        },genErrorHandler(callback));
    } else {
        Json::Value resp{Json::arrayValue};
        callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
    }
}

void Api::Apps::addSub(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  std::string &&pAppname, std::string&& pSubname) {
    std::string table;
    if(pAppname == "Timelogger" && !pSubname.empty()) {
        table = "timelog";
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    drogon::app().getDbClient()->execSqlAsync("INSERT INTO " + table + " (name) VALUES ($1)",
    [callback = std::move(callback)](const drogon::orm::Result& r) {
        callback(genResponse("Success!"));
    }, genErrorHandler(callback), std::move(pSubname));
}
