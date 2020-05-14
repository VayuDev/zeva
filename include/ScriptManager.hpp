#pragma once
#include <map>
#include "Script.hpp"
#include <thread>
#include <future>
#include <variant>



class ScriptManager {
public:
    ScriptManager() = default;
    void addScript(const std::string& pName, const std::string& pCode);
    std::future<ScriptReturn> executeScript(const std::string& pName, const std::string& pFunction, std::function<void(WrenVM*)> pParamSetter);
private:
    std::map<std::string, Script> mScripts;
};
