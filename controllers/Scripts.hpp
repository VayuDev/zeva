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
            METHOD_ADD(Scripts::updateScript, "update?scriptid={}&code={}", drogon::Post);
            METHOD_ADD(Scripts::runScript, "run?scriptid={}&param={}", drogon::Post);
            METHOD_ADD(Scripts::drawScript, "draw?scriptid={}", drogon::Get);
        METHOD_LIST_END

        void getAllScripts(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        void getScript(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int scriptid);
        void updateScript(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int scriptid, std::string&& pCode);
        void runScript(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int scriptid, std::string&& pParam);
        void drawScript(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int scriptid);
    };
}