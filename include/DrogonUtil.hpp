#pragma once
#include "../drogon/orm_lib/inc/drogon/orm/Exception.h"
#include "ScriptValue.hpp"
#include <drogon/HttpAppFramework.h>
#include <filesystem>
#include <optional>

inline std::function<void(const drogon::orm::DrogonDbException &)>
genErrorHandler(std::function<void(const drogon::HttpResponsePtr &)> call) {
  return [call](const drogon::orm::DrogonDbException &e) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
    resp->setBody(e.base().what());
    call(resp);
  };
}

inline std::function<void(const std::exception &)>
genDefErrorHandler(std::function<void(const drogon::HttpResponsePtr &)> call) {
  return [call](const std::exception &e) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
    resp->setBody(e.what());
    call(resp);
  };
}

inline drogon::HttpResponsePtr
genError(const std::string &pMsg,
         drogon::HttpStatusCode pStatusCode = drogon::k500InternalServerError) {
  LOG_ERROR << "Generating error: " << pMsg;
  auto errResp = drogon::HttpResponse::newHttpResponse();
  errResp->setStatusCode(pStatusCode);
  errResp->setBody(pMsg);
  return errResp;
}

inline drogon::HttpResponsePtr genResponse(const std::string &pMsg) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
  resp->setBody(pMsg);
  return resp;
}

inline trantor::Logger::LogLevel stringToLogLevel(const std::string &logLevel) {
  if (logLevel == "TRACE") {
    return trantor::Logger::kTrace;
  } else if (logLevel == "DEBUG") {
    return trantor::Logger::kDebug;
  } else if (logLevel == "INFO") {
    return trantor::Logger::kInfo;
  } else if (logLevel == "WARN") {
    return trantor::Logger::kWarn;
  } else if (logLevel == "ERROR") {
    return trantor::Logger::kError;
  } else if (logLevel == "FATAL") {
    return trantor::Logger::kFatal;
  } else if (logLevel == "SYSERR") {
    return trantor::Logger::kFatal;
  }
  throw std::runtime_error("Unknown log level '" + logLevel + "'");
}