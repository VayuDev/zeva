#include "Scripts.hpp"
#include "Util.hpp"
#include "ScriptManager.hpp"

void Api::Scripts::getAllScripts(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    drogon::app().getDbClient()->execSqlAsync("SELECT * FROM scripts",
    [callback=std::move(callback)](const drogon::orm::Result& r) {
        Json::Value ret;
        for(const auto& row: r) {
            Json::Value jsonRow;
            jsonRow["id"] = row["id"].as<std::string>();
            jsonRow["name"] = row["name"].as<std::string>();
            ret.append(std::move(jsonRow));
        }
        auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(ret));
        callback(resp);
    }, genErrorHandler(callback));
}

void Api::Scripts::getScript(const drogon::HttpRequestPtr&,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback, int scriptid) {
    drogon::app().getDbClient()->execSqlAsync("SELECT * FROM scripts WHERE id=$1",
    [callback=std::move(callback)](const drogon::orm::Result& r) {
        Json::Value jsonRow;
        jsonRow["id"] = r.at(0)["id"].as<std::string>();
        jsonRow["name"] = r.at(0)["name"].as<std::string>();
        jsonRow["code"] = r.at(0)["code"].as<std::string>();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(jsonRow));
        callback(resp);
    }, genErrorHandler(callback), scriptid);
}

void Api::Scripts::updateScript(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback, int scriptid,
                                std::string &&pCode) {
    std::string codeCopy = pCode;
    drogon::app().getDbClient()->execSqlAsync("UPDATE SCRIPTS SET code=$1 WHERE id=$2 RETURNING name",
    [callback=std::move(callback), pCode=std::move(pCode)](const drogon::orm::Result& r) {
        auto name = r.at(0)["name"].as<std::string>();
        try {
            ScriptManager::the().addScript(name, pCode);
            callback(drogon::HttpResponse::newHttpResponse());
        } catch(std::exception& e) {
            callback(genError(e.what()));
        }

    }, genErrorHandler(callback), std::move(codeCopy), scriptid);
}

void Api::Scripts::runScript(const drogon::HttpRequestPtr&,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback, int scriptid, std::string&& pParam) {
    drogon::app().getDbClient()->execSqlAsync("SELECT name FROM scripts WHERE id=$1",
    [callback=std::move(callback),param=std::move(pParam)](const drogon::orm::Result& r) {
        auto name = r.at(0)["name"].as<std::string>();
        //TODO convert to int
        ScriptManager::the().executeScriptWithCallback(name, "onRunOnce", {ScriptValue::makeString(param)},
                [callback=std::move(callback)](ScriptReturn&& ret) {
            auto error = std::get_if<std::string>(&ret.value);
            if(error) {
                auto errResp = drogon::HttpResponse::newHttpResponse();
                errResp->setStatusCode(drogon::k500InternalServerError);
                errResp->setBody(*error);
                callback(errResp);
            } else {
                auto val = std::get<ScriptValue>(ret.value);
                Json::Value json;
                json["return"] = scriptValueToJson(std::move(val));
                json["type"] = "ok";
                callback(drogon::HttpResponse::newHttpJsonResponse(std::move(json)));
            }
        });
    }, genErrorHandler(callback), scriptid);
}

void Api::Scripts::drawScript(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback, int scriptid) {
    drogon::app().getDbClient()->execSqlAsync("SELECT name FROM scripts WHERE id=$1",
    [callback=std::move(callback)](const drogon::orm::Result& r) {
        auto name = r.at(0)["name"].as<std::string>();
        ScriptManager::the().executeScriptWithCallback(name, "drawImage", {},
                [callback=std::move(callback)](ScriptReturn&& ret) {
            auto error = std::get_if<std::string>(&ret.value);
            if(error) {
                callback(genError(*error));
            } else {
                auto val = std::get<ScriptValue>(ret.value);
                if(val.type == WREN_TYPE_STRING) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setBody(std::move(val.stringValue));
                    resp->setContentTypeCode(drogon::CT_IMAGE_PNG);
                    callback(resp);
                } else {
                    callback(genError("Script didn't return a string!"));
                }
            }
        });
    }, genErrorHandler(callback), scriptid);
}

void Api::Scripts::deleteScript(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback, int scriptid) {
    drogon::app().getDbClient()->execSqlAsync("DELETE FROM scripts WHERE id=$1 RETURNING name",
    [callback=std::move(callback)](const drogon::orm::Result& r) {
        auto name = r.at(0)["name"].as<std::string>();
        ScriptManager::the().deleteScript(name);
        callback(genResponse("Deleted script"));
    }, genErrorHandler(callback), scriptid);
}
