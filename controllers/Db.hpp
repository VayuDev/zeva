#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>
namespace Api {
class Db : public drogon::HttpController<Db> {
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(Db::getAllTables, "all", drogon::Get);
  METHOD_ADD(Db::getTableCsv, "csv/{tablename}", drogon::Get);
  METHOD_ADD(Db::insertRow, "insertRow?json={}&tablename={}", drogon::Post);
  METHOD_LIST_END

  void
  getAllTables(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void
  getTableCsv(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              std::string &&pName);
  void
  insertRow(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback,
            std::string &&pJsonRow, std::string &&pTableName);
};
} // namespace Api
