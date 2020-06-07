#pragma once
#include <filesystem>
#include "ScriptValue.hpp"
#include <optional>
#include <json/json.h>

Json::Value scriptValueToJson(const ScriptValue& pVal);
Json::Value wrenValueToJsonValue(struct WrenVM* pVM, int pSlot);

std::string readWholeFile(const std::filesystem::path& pPath);
ScriptValue wrenValueToScriptValue(struct WrenVM* pVM, int pSlot);

bool isValidAscii(const signed char *c, size_t len);

inline void throwError(const char* msg) {
    char buff[1024];
    strerror_r(errno, buff, sizeof(buff));
    std::string err{msg};
    err += ": ";
    err += buff;
    throw std::runtime_error(err);
}