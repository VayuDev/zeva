#pragma once
#include "DatabaseWrapper.hpp"
#include "nlohmann/json_fwd.hpp"
#include <filesystem>
#include <seasocks/util/CrackedUri.h>
#include <seasocks/Request.h>
#include <seasocks/Response.h>
#include "Script.hpp"
#include <optional>


nlohmann::json queryResultToJson(const QueryResult&);
nlohmann::json queryResultToJsonArray(QueryResult&&);
nlohmann::json queryResultToJsonMap(const QueryResult&);
nlohmann::json scriptValueToJson(class ScriptValue&&);

std::string readWholeFile(const std::filesystem::path& pPath);
ScriptValue wrenValueToScriptValue(struct WrenVM* pVM, int pSlot);

inline seasocks::CrackedUri getBodyParamsFromRequest(const seasocks::Request& pRequest) {
    std::string s{(char *) pRequest.content(), pRequest.contentLength()};
    seasocks::CrackedUri body("/test?" + s);
    return body;
}

std::shared_ptr<seasocks::Response> responseFromJsonOrQuery(
        const std::optional<nlohmann::json>& pJson,
        const std::unique_ptr<QueryResult>& pQueryResult,
        const seasocks::CrackedUri& pRequestUri);