#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>

namespace  Api {
class Apps : public drogon::HttpController<Apps> {
public:
    METHOD_LIST_BEGIN
        METHOD_ADD(Apps::getCategories, "all", drogon::Get);
        METHOD_ADD(Apps::getSubapps, "subsAll?appname={}", drogon::Get);
        METHOD_ADD(Apps::addSub, "addSub?appname={}&subname={}", drogon::Post);
        METHOD_ADD(Apps::delSub, "delSub?appname={}&subid={}", drogon::Post);
    METHOD_LIST_END

    void getCategories(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void getSubapps(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       std::string&& pAppname);

    void addSub(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                    std::string&& pAppname,
                    std::string&& pSubname);
    void delSub(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                std::string&& pAppname,
                int64_t pSubId);
};
}
