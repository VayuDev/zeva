#pragma once
#include <filesystem>
#include "Script.hpp"
#include <optional>
#include <drogon/HttpAppFramework.h>

Json::Value scriptValueToJson(class ScriptValue&&);

std::string readWholeFile(const std::filesystem::path& pPath);
ScriptValue wrenValueToScriptValue(struct WrenVM* pVM, int pSlot);

inline std::function<void(const drogon::orm::DrogonDbException&)> genErrorHandler(std::function<void(const drogon::HttpResponsePtr &)> call) {
    return [call](const drogon::orm::DrogonDbException &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
        resp->setBody(e.base().what());
        call(resp);
    };
}
inline drogon::HttpResponsePtr genError(const std::string& pMsg) {
    auto errResp = drogon::HttpResponse::newHttpResponse();
    errResp->setStatusCode(drogon::k500InternalServerError);
    errResp->setBody(pMsg);
    return errResp;
}

inline drogon::HttpResponsePtr genResponse(const std::string& pMsg) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
    resp->setBody(pMsg);
    return resp;
}