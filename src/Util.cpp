#include "Util.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"
#include "DatabaseWrapper.hpp"

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