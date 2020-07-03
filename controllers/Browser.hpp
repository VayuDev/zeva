#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>

namespace Api::Apps {
class Browser : public drogon::HttpController<Browser> {
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(Browser::addVisitedUrl, "addVisitedUrl?browser={}&url={}",
             drogon::Post);
  METHOD_LIST_END

  void
  addVisitedUrl(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                std::string &&browserName, std::string &&url);
};
} // namespace Api::Apps
