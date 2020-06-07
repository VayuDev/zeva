#include "ScriptManager.hpp"

#include <utility>
#include <drogon/HttpAppFramework.h>
#include <csignal>

void ScriptManager::addScript(const std::string& pName, const std::string& pCode, bool pCheckIfCodeChanged) {
    std::lock_guard<std::shared_mutex> lock{mScriptsMutex};
    if(mScripts.count(pName) > 0) {
        if(pCheckIfCodeChanged && mScripts.at(pName).getCode() == pCode)
            return;
        mScripts.erase(pName);
    }
    if(pCheckIfCodeChanged) {
        LOG_INFO << "Script was changed by someone else!";
    }

    mScripts.emplace(std::piecewise_construct,
                     std::forward_as_tuple(pName),
                     std::forward_as_tuple(pName, pCode));
}    

std::future<ScriptBindingsReturn> ScriptManager::executeScript(const std::string& pName, const std::string& pFunction, const std::vector<ScriptValue>& pParamSetter) {
    std::shared_lock<std::shared_mutex> lock{mScriptsMutex};
    return mScripts.at(pName).execute(pFunction, pParamSetter);
}

void ScriptManager::deleteScript(const std::string &pName) {
    std::lock_guard<std::shared_mutex> lock{mScriptsMutex};
    if(mScripts.count(pName) > 0) {
        mScripts.erase(pName);
    }
}

void ScriptManager::onTableChanged(const std::string& pTable, const std::string &pType) {
    std::shared_lock<std::shared_mutex> lock{mScriptsMutex};
    if(pTable == "scripts") {
        auto result = drogon::app().getDbClient()->execSqlSync("SELECT name,code FROM scripts");

        lock.unlock();
        for(const auto& row: result) {
            try {
                addScript(row["name"].as<std::string>(), row["code"].as<std::string>(), true);
            } catch(std::exception& e) {
                LOG_ERROR << "Error adding script " << e.what();
            }

        }
        lock.lock();
    }
    for(auto& script: mScripts) {
        try {
            script.second.execute("onTableChanged", {ScriptValue::makeString(pTable), ScriptValue::makeString(pType)}).get();
        } catch(std::exception& e) {
            LOG_ERROR << "Error executing script: " << e.what();
        }
    }
}

void ScriptManager::executeScriptWithCallback(const std::string &pName, const std::string &pFunction,
                                              const std::vector<ScriptValue> &pParamSetter,
                                              std::function<void(ScriptBindingsReturn &&)> &&pCallback,
                                              std::function<void(std::exception& e)>&& pErrorCallback) {
    std::thread t{[=] {
        try {
            std::shared_lock<std::shared_mutex> lock{mScriptsMutex};
            auto future = mScripts.at(pName).execute(pFunction, pParamSetter);
            lock.unlock();
            pCallback(future.get());
        } catch(std::exception& e) {
            pErrorCallback(e);
        }

    }};
    t.detach();
}
