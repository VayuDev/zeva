#pragma once
#include <filesystem>
#include "ScriptValue.hpp"
#include <optional>
#include <json/json.h>

Json::Value scriptValueToJson(class ScriptValue&&);

std::string readWholeFile(const std::filesystem::path& pPath);
ScriptValue wrenValueToScriptValue(struct WrenVM* pVM, int pSlot);

bool isValidAscii(const signed char *c, size_t len);