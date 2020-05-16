#pragma once
#include "DatabaseWrapper.hpp"
#include "nlohmann/json_fwd.hpp"


nlohmann::json queryResultToJson(const QueryResult&);