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
        std::string selection = "";

        size_t i = 0;
        selection.clear();
        for (const auto &row : result) {
          const std::string datatype = row["data_type"].as<std::string>();
          const std::string columnName = row["column_name"].as<std::string>();
          if(i == 0 && skipId) {
            i++;
            continue;
          }
          if (truncateTimestamps && (datatype == "timestamp without time zone" ||
              datatype == "timestamp with time zone")) {
            selection +=
                "date_trunc('second', " + columnName + ") AS " + columnName;
          } else {
            selection += columnName;
          }
          if (i < result.size() - 1) {
            selection += ",";
          }
          ++i;
        }

        std::string firstColumnName;
        if(skipId && result.at(0)["column_name"] .as<std::string>() == "id") {
          firstColumnName = result.at(1)["column_name"] .as<std::string>();
        } else {
          firstColumnName = result.at(0)["column_name"] .as<std::string>();
        }


        auto query = "COPY (SELECT " + selection + " FROM " + tablename +
            " ORDER BY " + firstColumnName +
            " ASC)\n"
            " TO STDOUT WITH (DELIMITER ',', FORMAT CSV, HEADER);";
        try {
          PostgreSQLDatabase db;
          auto csvStr = db.performCopyToStdout(query);
          callback(genResponse(csvStr));
        } catch (std::exception &e) {
          LOG_ERROR << "Failed copy query: " + query;
          callback(genError(e.what()));
        }
      },
      genErrorHandler(callback), pName);
}
void Api::Db::insertRow(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pJsonRow, std::string &&pTableName) {
  LOG_DEBUG << "Inserting row into table: '" << pTableName << "'";
  try {
    std::stringstream inputJsonStringStream{pJsonRow};
    Json::Value inputJson;
    inputJsonStringStream >> inputJson;

    const auto &data = inputJson;

    std::vector<std::string> keys;
    std::vector<QueryValue> values;
    for (auto it = data.begin(); it != data.end(); ++it) {
      keys.push_back(it.key().asString());
      switch (it->type()) {
      case Json::ValueType::stringValue:
        values.push_back(QueryValue::makeString(it->asString()));
        break;
      case Json::ValueType::intValue:
        values.push_back(QueryValue::makeInt(it->asInt64()));
        break;
      default:
        throw std::runtime_error("Unknown value type");
      }
    }

    std::stringstream queryString;
    queryString << "INSERT INTO ";
    queryString << pTableName;
    queryString << " (";
    bool first = true;
    for (const auto &column : keys) {
      if (!first) {
        queryString << ',';
      } else {
        first = false;
      }
      queryString << column;
    }
    queryString << ") VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
      if (values.at(i).stringValue != "CURRENT_TIMESTAMP") {
        queryString << '$' << (i + 1);
      } else {
        values.erase(values.cbegin() + i);
        i -= 1;
        queryString << "CURRENT_TIMESTAMP";
      }
      if (i < values.size() - 1) {
        queryString << ',';
      }
    }
    queryString << ')';
    auto db = std::make_shared<PostgreSQLDatabase>();
    db->query(queryString.str(), values);
    callback(genResponse("ok"));
  } catch (std::exception &e) {
    callback(genError(e.what()));
  }
}
