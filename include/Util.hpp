#pragma once
#include <filesystem>
#include "Script.hpp"
#include <optional>
#include <drogon/HttpAppFramework.h>

Json::Value scriptValueToJson(class ScriptValue&&);

std::string readWholeFile(const std::filesystem::path& pPath);
ScriptValue wrenValueToScriptValue(struct WrenVM* pVM, int pSlot);

inline std::function<void(const drogon::orm::DrogonDbException&)> genErrorHandler(std::function<void(const drogon::HttpResponsePtr &)> call) {
    return [call](const drogon::orm::DrogonDbException &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
        resp->setBody(e.base().what());
        call(resp);
    };
}
inline drogon::HttpResponsePtr genError(const std::string& pMsg) {
    LOG_ERROR << "Generating error: " << pMsg;
    auto errResp = drogon::HttpResponse::newHttpResponse();
    errResp->setStatusCode(drogon::k500InternalServerError);
    errResp->setBody(pMsg);
    return errResp;
}

inline drogon::HttpResponsePtr genResponse(const std::string& pMsg) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
    resp->setBody(pMsg);
    return resp;
}

bool isValidAscii(const signed char *c, size_t len);

inline trantor::Logger::LogLevel stringToLogLevel(const std::string& logLevel) {
    if(logLevel == "TRACE") {
        return trantor::Logger::kTrace;
    } else if(logLevel == "DEBUG") {
        return trantor::Logger::kDebug;
    } else if(logLevel == "INFO") {
        return trantor::Logger::kInfo;
    } else if(logLevel == "WARN") {
        return trantor::Logger::kWarn;
    } else if(logLevel == "ERROR") {
        return trantor::Logger::kError;
    } else if(logLevel == "FATAL") {
        return trantor::Logger::kFatal;
    } else if(logLevel == "SYSERR") {
        return trantor::Logger::kFatal;
    }
    throw std::runtime_error("Unknown log level '" + logLevel + "'");
}