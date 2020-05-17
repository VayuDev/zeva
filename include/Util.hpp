#pragma once
#include "DatabaseWrapper.hpp"
#include "nlohmann/json_fwd.hpp"


nlohmann::json queryResultToJson(const QueryResult&);
nlohmann::json queryResultToJsonArray(QueryResult&&);
nlohmann::json queryResultToJsonMap(const QueryResult&);
nlohmann::json scriptValueToJson(class ScriptValue&&);