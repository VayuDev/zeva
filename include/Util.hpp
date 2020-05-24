#pragma once
#include "nlohmann/json_fwd.hpp"
#include <filesystem>
#include "Script.hpp"
#include <optional>
#include <drogon/HttpAppFramework.h>

nlohmann::json scriptValueToJson(class ScriptValue&&);

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