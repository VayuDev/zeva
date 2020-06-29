#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>
namespace Api {
class Scripts : public drogon::HttpController<Scripts> {
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(Scripts::getAllScripts, "all", drogon::Get);
  METHOD_ADD(Scripts::getScript, "get?scriptid={}", drogon::Get);
  METHOD_ADD(Scripts::updateScript, "update?scriptid={}&code={}", drogon::Post);
  METHOD_ADD(Scripts::setTimeout, "setTimeout?scriptid={}&timeout={}", drogon::Post);
  METHOD_ADD(Scripts::runScript, "run?scriptid={}&param={}", drogon::Post);
  METHOD_ADD(Scripts::drawScript, "draw?scriptid={}", drogon::Get);
  METHOD_ADD(Scripts::deleteScript, "delete?scriptid={}", drogon::Post);
  METHOD_ADD(Scripts::createScript, "create?scriptname={}", drogon::Post);
  METHOD_LIST_END

  void getAllScripts(
      const drogon::HttpRequestPtr &,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void
  getScript(const drogon::HttpRequestPtr &,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback,
            int64_t scriptid);
  void
  updateScript(const drogon::HttpRequestPtr &,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
               int64_t scriptid, std::string &&pCode);
  void
  runScript(const drogon::HttpRequestPtr &,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback,
            int64_t scriptid, std::string &&pParam);
  void
  drawScript(const drogon::HttpRequestPtr &,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
             int64_t scriptid);
  void
  deleteScript(const drogon::HttpRequestPtr &,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
               int64_t scriptid);
  void
  createScript(const drogon::HttpRequestPtr &,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
               std::string &&pName);
  void
  setTimeout(const drogon::HttpRequestPtr &,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
               int64_t scriptid,
               uint32_t timeout);
};
} // namespace Api