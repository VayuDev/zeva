#include "Util.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"
#include "DatabaseWrapper.hpp"
#include "Script.hpp"

nlohmann::json queryResultToJson(const QueryResult& queryResult) {
    nlohmann::json ret;
    auto columns = queryResult.getColumnNames();

    auto names = nlohmann::json::array();
    names = std::move(columns);
    ret["columnNames"] = std::move(names);


    auto result = nlohmann::json::array();
    for(size_t r = 0; r < queryResult.getRowCount(); ++r) {
        auto row = nlohmann::json::array();
        for(size_t c = 0; c < queryResult.getColumnCount(); ++c) {
            const auto& sval = queryResult.getValue(r, c);
            switch(sval.type) {
                case QueryValueType::INTEGER:
                    row.push_back(sval.intValue);
                    break;
                case QueryValueType::STRING:
                    row.push_back(sval.stringValue);
                    break;
                case QueryValueType::TNULL:
                    row.push_back(nullptr);
                    break;
                default:
                    assert(false);
            }
        }
        result.push_back(std::move(row));
    }
    ret["result"] = std::move(result);
    return ret;
}

nlohmann::json queryResultToJsonArray(QueryResult &&pQueryResult) {
    nlohmann::json ret;
    ret["columnNames"] = std::move(pQueryResult.getColumnNames());
    auto result = nlohmann::json::array();
    for(size_t r = 0; r < pQueryResult.getRowCount(); ++r) {
        auto row = nlohmann::json::array();
        for(size_t c = 0; c < pQueryResult.getColumnCount(); ++c) {
            const auto& sval = pQueryResult.getValue(r, c);
            switch(sval.type) {
                case QueryValueType::INTEGER:
                    row.push_back(sval.intValue);
                    break;
                case QueryValueType::STRING:
                    row.push_back(sval.stringValue);
                    break;
                case QueryValueType::TNULL:
                    row.push_back(nullptr);
                    break;
                default:
                    assert(false);
            }
        }
        result.push_back(std::move(row));
    }
    ret["result"] = std::move(result);
    return ret;
}

nlohmann::json queryResultToJsonMap(const QueryResult& pQueryResult) {
    auto ret = nlohmann::json::array();
    for(size_t r = 0; r < pQueryResult.getRowCount(); ++r) {
        nlohmann::json row;
        for(size_t c = 0; c < pQueryResult.getColumnCount(); ++c) {
            const auto& sval = pQueryResult.getValue(r, c);
            switch(sval.type) {
                case QueryValueType::INTEGER:
                    row[pQueryResult.getColumnNames().at(c)] = sval.intValue;
                    break;
                case QueryValueType::STRING:
                    row[pQueryResult.getColumnNames().at(c)] = sval.stringValue;
                    break;
                case QueryValueType::TNULL:
                    row[pQueryResult.getColumnNames().at(c)] = nullptr;
                    break;
                default:
                    assert(false);
            }
        }
        ret.push_back(std::move(row));
    }
    return ret;
}

nlohmann::json scriptValueToJson(ScriptValue && pVal) {
    switch(pVal.type) {
        case WREN_TYPE_STRING:
            return std::move(pVal.stringValue);
        case WREN_TYPE_NUM:
            return pVal.doubleValue;
        case WREN_TYPE_NULL:
            return nullptr;
        case WREN_TYPE_BOOL:
            return pVal.boolValue;
        default:
            assert(false);
    }
}
