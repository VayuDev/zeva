#include "Db.hpp"
#include "DrogonUtil.hpp"
#include "PostgreSQLDatabase.hpp"
#include "Util.hpp"

void Api::Db::getAllTables(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  drogon::app().getDbClient()->execSqlAsync(
      R"(
SELECT table_name AS name,(table_name IN (SELECT name FROM protected)) AS is_protected
FROM information_schema.tables
WHERE table_schema = 'public')",
      [callback = std::move(callback)](const drogon::orm::Result &result) {
        Json::Value resp;
        for (const auto &row : result) {
          Json::Value jsonRow;
          jsonRow["name"] = row["name"].as<std::string>();
          jsonRow["is_protected"] = row["is_protected"].as<bool>();
          resp.append(std::move(jsonRow));
        }
        callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
      },
      genErrorHandler(callback));
}

void Api::Db::getTableCsv(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pName) {
  bool truncateTimestamps = false;
  auto truncateTimestampsParm = req->getParameters().find("truncateTimestamps");
  if (truncateTimestampsParm != req->getParameters().end() &&
      truncateTimestampsParm->second == "true") {
    truncateTimestamps = true;
  }
  bool skipId = false;
  auto skipIdParam = req->getParameters().find("skipId");
  if (skipIdParam != req->getParameters().end() &&
      skipIdParam->second == "true") {
    skipId = true;
  }

  drogon::app().getDbClient()->execSqlAsync(
      "SELECT column_name,data_type FROM information_schema.columns WHERE "
      "table_name=$1 ORDER BY information_schema.columns.ordinal_position",
      [callback = std::move(callback), skipId, truncateTimestamps,
       tablename = pName](const drogon::orm::Result &result) {
        std::string selection = "*";
        std::string firstColumnName = "id";
        if (truncateTimestamps) {
          size_t i = 0;
          selection.clear();
          if (skipId) {
            firstColumnName.clear();
          }
          for (const auto &row : result) {
            const std::string datatype = row["data_type"].as<std::string>();
            const std::string columnName = row["column_name"].as<std::string>();
            if (firstColumnName.empty() && skipId) {
              firstColumnName = columnName;
            } else {
              if (datatype == "timestamp without time zone" ||
                  datatype == "timestamp with time zone") {
                selection +=
                    "date_trunc('second', " + columnName + ") AS " + columnName;
              } else {
                selection += columnName;
              }
              if (i < result.size() - 1) {
                selection += ",";
              }
            }
            ++i;
          }
        }
        try {
          PostgreSQLDatabase db;
          auto csvStr = db.performCopyToStdout(
              "COPY (SELECT " + selection + " FROM " + tablename +
              " ORDER BY " + firstColumnName +
              " ASC)\n"
              " TO STDOUT WITH (DELIMITER ',', FORMAT CSV, HEADER);");
          callback(genResponse(csvStr));
        } catch (std::exception &e) {

          callback(genError(e.what()));
        }
      },
      genErrorHandler(callback), pName);
}
