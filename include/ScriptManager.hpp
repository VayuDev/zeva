#pragma once
#include <map>
#include "Script.hpp"
#include <thread>
#include <future>
#include <variant>
#include <shared_mutex>

class ScriptManager {
public:
    void addScript(const std::string& pName, const std::string& pCode, bool pCheckIfCodeChanged = false);
    void deleteScript(const std::string& pName);
    std::future<ScriptReturn> executeScript(const std::string& pName, const std::string& pFunction, const std::vector<ScriptValue>& pParamSetter);
    void executeScriptWithCallback(const std::string& pName, const std::string& pFunction, const std::vector<ScriptValue>& pParamSetter,
            std::function<void(ScriptReturn&&)> &&pCallback, std::function<void(std::exception& e)>&& pErrorCallback);
    void onTableChanged(const std::string& pTable, const std::string& pType);

    static ScriptManager& the() {
        static ScriptManager scriptManager;
        return scriptManager;
    }
private:
    std::map<std::string, Script> mScripts;
    std::shared_mutex mScriptsMutex;
};