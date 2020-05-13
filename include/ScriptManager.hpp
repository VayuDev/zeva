#pragma once
#include <map>
#include <string>
#include "Script.hpp"
#include <functional>
#include <wren.hpp>

class Script;

struct ScriptValue {
    WrenType type;
    union {
        char *stringValue;
        double doubleValue;
        bool boolValue;
    };
};

class ScriptManager final {
public:
    ScriptManager();
    ~ScriptManager();
    void setLastError(std::string pLastError);
    void compile(const std::string& pModule, const std::string& pCode);
    ScriptValue execute(const std::string& pModule, const std::string& pFunctionName, std::function<void(WrenVM*)> pParamSetter);
private:
    std::string popLastError();
    WrenVM *mVM;
    std::string mLastError;
    std::map<std::string, WrenHandle*> mFunctions;
    std::map<std::string, Script> mScripts;
};