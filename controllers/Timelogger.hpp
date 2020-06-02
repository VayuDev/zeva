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
        METHOD_ADD(Timelogger::startActivity, "startActivity?subid={}&activityid={}", drogon::Post);
        METHOD_ADD(Timelogger::stopActivity, "stopActivity?subid={}", drogon::Post);
        METHOD_ADD(Timelogger::deleteActivity, "deleteActivity?subid={}&activityid={}", drogon::Post);
        METHOD_ADD(Timelogger::abortActivity, "abortActivity?subid={}", drogon::Post);
    METHOD_LIST_END

    void getStatus(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int64_t pSubid);

    void createActivity(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        int64_t pTimelogId,
                        std::string&& pActivityName);
    void startActivity(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        int64_t pTimelogId,
                        int64_t pActivityId);
    void stopActivity(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int64_t pTimelogId);
    void abortActivity(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      int64_t pTimelogId);
    void deleteActivity(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      int64_t pTimelogId,
                      int64_t pActivityId);
};
}
}
