#include "ApiHandler.hpp"

#include <utility>
#include <seasocks/Response.h>
#include "Util.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"

std::shared_ptr<seasocks::Response> ApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {
    try {
        std::unique_ptr<QueryResult> responseData;
        if(pUrl.path().at(0) == "api") {
            switch(pRequest.verb()) {
                case seasocks::Request::Verb::Get:
                    if(pUrl.path().at(1) == "scripts") {
                        if(pUrl.path().at(2) == "all") {
                            responseData = mDb->query("SELECT id,name FROM scripts");
                        } else if(pUrl.path().at(2) == "get" && pUrl.hasParam("scriptid")) {
                            auto idStr = pUrl.queryParam("scriptid");
                            auto id = std::stoll(idStr);
                            responseData = mDb->query("SELECT * FROM scripts WHERE id=$1 ORDER BY id ASC", {QueryValue::makeInt(id)});
                        }
                    }
                    break;
                case seasocks::Request::Verb::Post:
                    if(pUrl.path().at(1) == "scripts") {
                        std::string s{(char*)pRequest.content(), pRequest.contentLength()};
                        seasocks::CrackedUri body("/test?" + s);

                        if(pUrl.path().at(2) == "update" && body.hasParam("scriptid") && body.hasParam("code")) {
                            log().warning("Posting new script!");
                            auto idStr = body.queryParam("scriptid");
                            auto id = std::stoll(idStr);
                            auto code = body.queryParam("code");
                            responseData = mDb->query("UPDATE scripts SET code=$1 WHERE id=$2", {QueryValue::makeString(code), QueryValue::makeInt(id)});
                            auto scriptName = mDb->query("SELECT name FROM scripts WHERE id=$1", {QueryValue::makeInt(id)});
                            mScriptManager->addScript(scriptName->getValue(0, 0).stringValue, code);
                        }

                    }
                    break;
                default:
                    break;
            }
        }
        if(responseData) {
            if(pUrl.hasParam("format") && pUrl.queryParam("format") == "list") {
                auto json = queryResultToJson(*responseData);
                return seasocks::Response::jsonResponse(json.dump());
            } else {
                auto json = queryResultToJsonMap(*responseData);
                return seasocks::Response::jsonResponse(json.dump());
            }
        }
        return nullptr;
    } catch(std::exception& e) {
        return seasocks::Response::error(seasocks::ResponseCode::InternalServerError, e.what());
    }
}

ApiHandler::ApiHandler(std::shared_ptr<DatabaseWrapper> pDb, std::shared_ptr<ScriptManager> pScriptManager)
: mDb(std::move(pDb)), mScriptManager(std::move(pScriptManager)) {

}
