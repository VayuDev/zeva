#pragma once
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>

namespace Api::Db {

class SystemMonitor : public drogon::HttpController<SystemMonitor> {
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(SystemMonitor::getConfig, "config?devicename={}", drogon::Get);
  METHOD_LIST_END

  void
  getConfig(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback,
            std::string &&deviceName);
};

} // namespace Api::Db