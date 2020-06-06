#pragma once
#include <map>
#include <string>
#include <functional>
#include <wren.hpp>
#include <variant>
#include <future>
#include <list>
#include <queue>
#include <filesystem>
#include <variant>
#include <json/json.h>

class ScriptValue {
public:
    WrenType type = WREN_TYPE_NULL;
    union {
        double doubleValue;
        bool boolValue;
    };
    std::vector<ScriptValue> listValue;
    std::string stringValue;
    static ScriptValue makeString(const std::string& pStr) {
        ScriptValue ret;
        ret.type = WREN_TYPE_STRING;
        ret.stringValue = pStr;
        return ret;
    }
    static ScriptValue makeInt(int64_t pInt) {
        ScriptValue ret;
        ret.type = WREN_TYPE_NUM;
        ret.doubleValue = pInt;
        return ret;
    }
    static ScriptValue makeDouble(double pDouble) {
        ScriptValue ret;
        ret.type = WREN_TYPE_NUM;
        ret.doubleValue = pDouble;
        return ret;
    }
    static ScriptValue makeNull() {
        return ScriptValue{};
    }
};

class ScriptReturn {
public:
    time_t timestamp = -1;
    ScriptValue value;
    ScriptReturn() = default;
};

using ScriptBindingsReturn = std::variant<std::string, Json::Value>;