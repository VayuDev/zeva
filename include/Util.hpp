#pragma once
#include "DatabaseWrapper.hpp"
#include "nlohmann/json_fwd.hpp"
#include <filesystem>


nlohmann::json queryResultToJson(const QueryResult&);
nlohmann::json queryResultToJsonArray(QueryResult&&);
nlohmann::json queryResultToJsonMap(const QueryResult&);
nlohmann::json scriptValueToJson(class ScriptValue&&);

std::string readWholeFile(const std::filesystem::path& pPath);