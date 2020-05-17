#include <fstream>
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
        case WREN_TYPE_LIST: {
            auto list = nlohmann::json::array();
            for(auto & i : pVal.listValue) {
                list.push_back(scriptValueToJson(std::move(i)));
            }
            return list;
        }
        case WREN_TYPE_UNKNOWN:
            return "(unknown)";
        default:
            log().error("Unknown type %i", (int)pVal.type);
            assert(false);
    }
}

std::string readWholeFile(const std::filesystem::path& pPath) {
    std::ifstream t(pPath);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    return str;
}

ScriptValue wrenValueToScriptValue(struct WrenVM *pVM, int pSlot) {
    ScriptValue ret = {.type = wrenGetSlotType(pVM, pSlot) };
    switch(ret.type) {
        case WrenType::WREN_TYPE_BOOL:
            ret.boolValue = wrenGetSlotBool(pVM, pSlot);
            break;
        case WrenType::WREN_TYPE_NUM:
            ret.doubleValue = wrenGetSlotDouble(pVM, pSlot);
            break;
        case WrenType::WREN_TYPE_STRING:
            ret.stringValue = wrenGetSlotString(pVM, pSlot);
            break;
        case WrenType::WREN_TYPE_NULL:
            break;
        case WrenType::WREN_TYPE_LIST: {
            const size_t count = wrenGetListCount(pVM, pSlot);
            wrenEnsureSlots(pVM, pSlot + 2);
            for(size_t i = 0; i < count; ++i) {
                wrenGetListElement(pVM, pSlot, i, pSlot + 1);
                ret.listValue.push_back(wrenValueToScriptValue(pVM, pSlot + 1));
            }
            break;
        }
        default:
            assert(false);
            break;
    }
    return ret;
}
