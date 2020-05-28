#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>

namespace  Api {
namespace Apps{

class Timelogger : public drogon::HttpController<Timelogger> {
public:
    METHOD_LIST_BEGIN
        METHOD_ADD(Timelogger::getStatus, "status?subid={}", drogon::Get);
        METHOD_ADD(Timelogger::createActivity, "createActivity?subid={}&activityname={}", drogon::Post);
    METHOD_LIST_END

    void getStatus(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int64_t pSubid);

    void createActivity(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        int64_t pTimelogId,
                        std::string&& pActivityName);
};
}
}
