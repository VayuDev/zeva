#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>
namespace  Api {
    class Scripts : public drogon::HttpController<Scripts> {
    public:
        METHOD_LIST_BEGIN
            METHOD_ADD(Scripts::getAllScripts, "all", drogon::Get);
            METHOD_ADD(Scripts::getScript, "get?scriptid={}", drogon::Get);
        METHOD_LIST_END

        void getAllScripts(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        void getScript(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int scriptid);
    };
}