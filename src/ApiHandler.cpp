#include "ApiHandler.hpp"

#include <utility>
#include <seasocks/Response.h>
#include "Util.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"

std::shared_ptr<seasocks::Response> ApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {

    std::unique_ptr<QueryResult> responseData;
    std::optional<nlohmann::json> responseJson;
    seasocks::ResponseCode responseCode = seasocks::ResponseCode::Ok;
    if(pUrl.path().at(0) == "api") {
        switch(pRequest.verb()) {
            case seasocks::Request::Verb::Get:
                //GET SCRIPTS
                if(pUrl.path().at(1) == "scripts") {
                    if(pUrl.path().at(2) == "all") {
                        responseData = mDb->query("SELECT id,name FROM scripts");
                    } else if(pUrl.path().at(2) == "get" && pUrl.hasParam("scriptid")) {
                        auto idStr = pUrl.queryParam("scriptid");
                        auto id = std::stoll(idStr);
                        responseData = mDb->query("SELECT * FROM scripts WHERE id=$1 ORDER BY id ASC", {QueryValue::makeInt(id)});
                    }
                }
                //GET DB
                if(pUrl.path().at(1) == "db") {
                    if(pUrl.path().at(2) == "all") {
                        responseData = mDb->query(
                                R"--(
SELECT table_name AS name,(table_name IN (SELECT name FROM protected)) AS is_protected FROM information_schema.tables WHERE table_schema = 'public')--");
                    }
                }
                break;
            case seasocks::Request::Verb::Post: {
                std::string s{(char *) pRequest.content(), pRequest.contentLength()};
                seasocks::CrackedUri body("/test?" + s);
                //POST SCRIPTS
                if (pUrl.path().at(1) == "scripts") {


                    if (pUrl.path().at(2) == "update" && body.hasParam("scriptid") && body.hasParam("code")) {
                        log().info("Posting new script!");
                        auto idStr = body.queryParam("scriptid");
                        auto id = std::stoll(idStr);
                        auto code = body.queryParam("code");
                        responseData = mDb->query("UPDATE scripts SET code=$1 WHERE id=$2",
                                                  {QueryValue::makeString(code), QueryValue::makeInt(id)});
                        auto scriptName = mDb->query("SELECT name FROM scripts WHERE id=$1", {QueryValue::makeInt(id)});
                        mScriptManager->addScript(scriptName->getValue(0, 0).stringValue, code);
                    } else if (pUrl.path().at(2) == "run" && body.hasParam("scriptid")) {
                        auto id = std::stoll(body.queryParam("scriptid"));
                        auto scriptName = mDb->query("SELECT name FROM scripts WHERE id=$1", {QueryValue::makeInt(id)});

                        std::vector<ScriptValue> params;
                        if (body.hasParam("param")) {
                            auto paramString = body.queryParam("param");
                            bool number = true;
                            for (char c: paramString) {
                                if (!isdigit(c) && c != '.') {
                                    number = false;
                                    break;
                                }
                            }
                            if (number && !paramString.empty()) {
                                params.push_back(ScriptValue::makeDouble(std::stod(body.queryParam("param"))));
                            } else {
                                params.push_back(ScriptValue::makeString(body.queryParam("param")));
                            }
                        }

                        auto scriptReturn = mScriptManager->executeScript(scriptName->getValue(0, 0).stringValue,
                                                                          "onRunOnce", params).get();
                        auto scriptValue = std::get_if<ScriptValue>(&scriptReturn);

                        if (!scriptValue) {
                            return seasocks::Response::error(seasocks::ResponseCode::InternalServerError,
                                                             std::get<std::string>(scriptReturn));
                        }

                        nlohmann::json resp;
                        resp["return"] = scriptValueToJson(std::move(*scriptValue));
                        responseJson = std::move(resp);

                    } else if (pUrl.path().at(2) == "create" && body.hasParam("scriptname")) {
                        auto name = body.queryParam("scriptname");
                        auto code = readWholeFile("assets/template.wren");
                        auto res = mDb->query("INSERT INTO scripts (name, code) VALUES ($1, $2) RETURNING id",
                                              {QueryValue::makeString(name), QueryValue::makeString(code)});
                        mScriptManager->addScript(name, code);
                        nlohmann::json resp;
                        resp["name"] = name;
                        resp["id"] = res->getValue(0, 0).intValue;
                        responseJson = std::move(resp);
                    } else if (pUrl.path().at(2) == "delete" && body.hasParam("scriptid")) {
                        auto id = std::stoll(body.queryParam("scriptid"));
                        auto scriptName =
                                mDb->query("SELECT name FROM scripts WHERE id=$1", {QueryValue::makeInt(id)})->getValue(
                                        0, 0).stringValue;
                        mDb->query("DELETE FROM scripts WHERE id=$1", {QueryValue::makeInt(id)});
                        mScriptManager->deleteScript(scriptName);
                        nlohmann::json resp;
                        resp = "ok";
                        responseJson = std::move(resp);
                    }
                }
                //POST DATABASE
                if (pUrl.path().at(1) == "db") {
                    if (pUrl.path().at(2) == "delete" && body.hasParam("tablename")) {

                        auto tablename =  body.queryParam("tablename");
                        bool isProteced = mDb->query("SELECT exists(SELECT name FROM protected WHERE name=$1)", {QueryValue::makeString(tablename)})
                                ->getValue(0, 0).boolValue;
                        if(isProteced) {
                            return seasocks::Response::error(seasocks::ResponseCode::Forbidden, "Table is protected!");
                        } else {
                            //TODO sanitize tablename
                            mDb->query("DROP TABLE " + tablename);
                            nlohmann::json resp;
                            resp = "ok";
                            responseJson = std::move(resp);
                        }
                    }
                }
                break;
            }
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
    } else if(responseJson) {
        if(responseCode != seasocks::ResponseCode::Ok)
            return seasocks::Response::error(seasocks::ResponseCode::InternalServerError, responseJson->dump());
        return seasocks::Response::jsonResponse(responseJson->dump());
    }
    return nullptr;

}

ApiHandler::ApiHandler(std::shared_ptr<DatabaseWrapper> pDb, std::shared_ptr<ScriptManager> pScriptManager)
: mDb(std::move(pDb)), mScriptManager(std::move(pScriptManager)) {

}
