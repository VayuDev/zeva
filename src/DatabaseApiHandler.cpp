#include "ScriptsApiHandler.hpp"

#include <utility>
#include <seasocks/Response.h>
#include <seasocks/ResponseBuilder.h>
#include "Util.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"
#include "PostgreSQLDatabase.hpp"
#include <sstream>
#include <DatabaseApiHandler.hpp>


std::shared_ptr<seasocks::Response> DatabaseApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {

    std::unique_ptr<QueryResult> responseData;
    std::optional<nlohmann::json> responseJson;
    seasocks::ResponseCode responseCode = seasocks::ResponseCode::Ok;
    auto body = getBodyParamsFromRequest(pRequest);
    if(pRequest.verb() == seasocks::Request::Verb::Get) {
        if(pUrl.path().at(0) == "all") {
            responseData = mDb->query(
                    R"--(
SELECT table_name AS name,(table_name IN (SELECT name FROM protected)) AS is_protected
FROM information_schema.tables
WHERE table_schema = 'public')--");
        } else if(pUrl.path().at(0) == "csv" && pUrl.path().size() == 2) {
            //TODO sanitize tablename
            auto tablename = pUrl.path().at(1);
            auto pdb = std::dynamic_pointer_cast<PostgreSQLDatabase>(mDb);
            assert(pdb);
            std::string selection = "*";
            std::string firstColumnName = "id";
            if(pUrl.hasParam("truncateTimestamps") && pUrl.queryParam("truncateTimestamps") == "true") {
                auto res = pdb->query(
                        "SELECT column_name,data_type FROM information_schema.columns WHERE table_name=$1 ORDER BY information_schema.columns.ordinal_position",
                        {QueryValue::makeString(tablename)});
                selection = "";
                for(size_t r = 1; r < res->getRowCount(); ++r) {
                    const std::string& datatype = res->getValue(r, 1).stringValue;
                    const std::string& columnName = res->getValue(r, 0).stringValue;
                    if(firstColumnName.empty() && pUrl.hasParam("skipId") && pUrl.queryParam("skipId") == "true") {
                        firstColumnName = columnName;
                    }
                    if(datatype == "timestamp without time zone"
                       || datatype == "timestamp with time zone") {
                        selection += "date_trunc('second', " + columnName + ") AS " + columnName;
                    } else {
                        selection += columnName;
                    }
                    if(r != res->getRowCount() - 1) {
                        selection += ",";
                    }
                }
            } else if(pUrl.path().at(0) == "json") {
                auto jsonStr = mDb->query("SELECT array_to_json(array_agg(row_to_json(r))) FROM scripts AS r")->getValue(0, 0).stringValue;
                return seasocks::Response::jsonResponse(jsonStr);
            }
            auto ret = pdb->performCopyToStdout("COPY (SELECT " + selection + " FROM " + tablename + " ORDER BY " + firstColumnName + " ASC)\n"
                                                                                                                                      " TO STDOUT WITH (DELIMITER ',', FORMAT CSV, HEADER);");
            return seasocks::Response::textResponse(ret);
        }
    } else if(pRequest.verb() == seasocks::Request::Verb::Post) {
        //POST DATABASE
            if (pUrl.path().at(0) == "delete" && body.hasParam("tablename")) {

            auto tablename =  body.queryParam("tablename");
            bool isProteced = mDb->query("SELECT exists(SELECT name FROM protected WHERE name=$1)", {QueryValue::makeString(tablename)})
                    ->getValue(0, 0).boolValue;
            if(isProteced) {
                return seasocks::Response::error(seasocks::ResponseCode::Forbidden, "Table is protected!");
            } else {
                //TODO sanitize tablename
                mDb->query("DROP TABLE " + tablename);
                nlohmann::json resp;
                resp = "ok";
                responseJson = std::move(resp);
            }
        }
    }
    return responseFromJsonOrQuery(responseJson, responseData, pUrl);
}

DatabaseApiHandler::DatabaseApiHandler(std::shared_ptr<DatabaseWrapper> pDb, std::shared_ptr<ScriptManager> pScriptManager)
: mDb(std::move(pDb)), mScriptManager(std::move(pScriptManager)) {

}