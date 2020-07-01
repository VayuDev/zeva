#include "Browser.hpp"
#include "DrogonUtil.hpp"

void Api::Apps::Browser::addVisitedUrl(const drogon::HttpRequestPtr &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 std::string &&browserId,
                                 std::string &&url) {
  auto resp = genResponse("ok");
  resp->addHeader("Access-Control-Allow-Origin", "*");
  callback(resp);
}