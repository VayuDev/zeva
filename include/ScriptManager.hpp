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
    void deleteScript(const std::string& pName);
    std::future<ScriptReturn> executeScript(const std::string& pName, const std::string& pFunction, const std::vector<ScriptValue>& pParamSetter);
private:
    std::map<std::string, Script> mScripts;
};
