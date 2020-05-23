#include "ScriptsApiHandler.hpp"

#include <utility>
#include <seasocks/Response.h>
#include <seasocks/ResponseBuilder.h>
#include "Util.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"
#include "PostgreSQLDatabase.hpp"
#include <sstream>

std::shared_ptr<seasocks::Response> ScriptsApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {

    std::unique_ptr<QueryResult> responseData;
    std::optional<nlohmann::json> responseJson;
    seasocks::ResponseCode responseCode = seasocks::ResponseCode::Ok;
    if(pRequest.verb() == seasocks::Request::Verb::Get) {
        if(pUrl.path().at(0) == "all") {
            responseData = mDb->query("SELECT id,name FROM scripts");
        } else if(pUrl.path().at(0) == "get" && pUrl.hasParam("scriptid")) {
            auto idStr = pUrl.queryParam("scriptid");
            auto id = std::stoll(idStr);
            responseData = mDb->query("SELECT * FROM scripts WHERE id=$1 ORDER BY id ASC", {QueryValue::makeInt(id)});
        } else if(pUrl.path().at(0) == "draw" && pUrl.hasParam("scriptid")) {
            auto id = std::stoll(pUrl.queryParam("scriptid"));
            auto scriptName = mDb->query("SELECT name FROM scripts WHERE id=$1", {QueryValue::makeInt(id)})
                    ->getValue(0, 0).stringValue;

            ScriptReturn ret;
            if (pUrl.hasParam("width") && pUrl.hasParam("height")) {
                auto width = std::stoll(pUrl.queryParam("width"));
                auto height = std::stoll(pUrl.queryParam("height"));
                ret = mScriptManager->executeScript(scriptName, "drawImage",
                                                    {ScriptValue::makeInt(width), ScriptValue::makeInt(height)}).get();
            } else {
                ret = mScriptManager->executeScript(scriptName, "drawImage", {}).get();
            }
            auto imageVal = std::get_if<ScriptValue>(&ret.value);
            if (imageVal) {
                return seasocks::ResponseBuilder().withContentType("image/png").operator<<(
                        imageVal->stringValue).build();
            } else {
                return seasocks::Response::error(seasocks::ResponseCode::InternalServerError,
                                                 std::get<std::string>(ret.value));
            }
        }
    } else if(pRequest.verb() == seasocks::Request::Verb::Post) {
        auto body = getBodyParamsFromRequest(pRequest);
        if (pUrl.path().at(0) == "update" && body.hasParam("scriptid") && body.hasParam("code")) {
            log().info("Posting new script!");
            auto idStr = body.queryParam("scriptid");
            auto id = std::stoll(idStr);
            auto code = body.queryParam("code");
            responseData = mDb->query("UPDATE scripts SET code=$1 WHERE id=$2",
                                      {QueryValue::makeString(code), QueryValue::makeInt(id)});
            auto scriptName = mDb->query("SELECT name FROM scripts WHERE id=$1", {QueryValue::makeInt(id)});
            mScriptManager->addScript(scriptName->getValue(0, 0).stringValue, code);
        } else if (pUrl.path().at(0) == "run" && body.hasParam("scriptid")) {
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
                if(paramString.empty()) {
                    params.push_back(ScriptValue::makeNull());
                } else if (number) {
                    params.push_back(ScriptValue::makeDouble(std::stod(body.queryParam("param"))));
                } else {
                    params.push_back(ScriptValue::makeString(body.queryParam("param")));
                }
            }

            auto scriptReturn = mScriptManager->executeScript(scriptName->getValue(0, 0).stringValue,
                                                              "onRunOnce", params).get();
            auto scriptValue = std::get_if<ScriptValue>(&scriptReturn.value);

            if (!scriptValue) {
                return seasocks::Response::error(seasocks::ResponseCode::InternalServerError,
                                                 std::get<std::string>(scriptReturn.value));
            }

            nlohmann::json resp;
            resp["return"] = scriptValueToJson(std::move(*scriptValue));
            responseJson = std::move(resp);

        } else if (pUrl.path().at(0) == "create" && body.hasParam("scriptname")) {
            auto name = body.queryParam("scriptname");
            auto code = readWholeFile("assets/template.wren");
            auto res = mDb->query("INSERT INTO scripts (name, code) VALUES ($1, $2) RETURNING id",
                                  {QueryValue::makeString(name), QueryValue::makeString(code)});
            mScriptManager->addScript(name, code);
            nlohmann::json resp;
            resp["name"] = name;
            resp["id"] = res->getValue(0, 0).intValue;
            responseJson = std::move(resp);
        } else if (pUrl.path().at(0) == "delete" && body.hasParam("scriptid")) {
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
    return responseFromJsonOrQuery(responseJson, responseData, pUrl);
}

ScriptsApiHandler::ScriptsApiHandler(std::shared_ptr<DatabaseWrapper> pDb, std::shared_ptr<ScriptManager> pScriptManager)
: mDb(std::move(pDb)), mScriptManager(std::move(pScriptManager)) {

}