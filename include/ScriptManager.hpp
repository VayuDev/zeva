#pragma once
#include <map>
#include <string>
#include <functional>
#include <wren.hpp>

struct ScriptValue {
    WrenType type;
    union {
        char *stringValue;
        double doubleValue;
        bool boolValue;
    };
};

class Script final {
public:
    Script(const std::string& pModule, const std::string& pCode);
    ~Script();
    void setLastError(std::string pLastError);
    ScriptValue execute(const std::string& pFunctionName, std::function<void(WrenVM*)> pParamSetter);
private:
    std::string popLastError();
    
    std::string mModuleName;
    WrenVM *mVM;
    WrenHandle *mInstance;
    std::string mLastError;
    std::map<std::string, WrenHandle*> mFunctions;
};