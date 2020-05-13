#pragma once
#include <map>
#include <string>
#include "Script.hpp"

class Script;
struct WrenVM;

class ScriptManager final {
public:
    ScriptManager();
    ~ScriptManager();
    void setLastError(std::string pLastError);
    void compile(const std::string& pModule, const std::string& pCode);
private:
    std::string popLastError();
    WrenVM *mVM;
    std::string mLastError;
    std::map<std::string, WrenHandle*> mFunctions;
    std::map<std::string, Script> mScripts;
};