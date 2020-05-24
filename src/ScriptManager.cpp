#include "ScriptManager.hpp"

#include <utility>

void ScriptManager::addScript(const std::string& pName, const std::string& pCode) {
    if(mScripts.count(pName) > 0) {
        mScripts.erase(pName);
    }
    mScripts.emplace(std::piecewise_construct,
                     std::forward_as_tuple(pName),
                     std::forward_as_tuple(pName, pCode, mLogger));
}    

std::future<ScriptReturn> ScriptManager::executeScript(const std::string& pName, const std::string& pFunction, const std::vector<ScriptValue>& pParamSetter) {
    return mScripts.at(pName).execute(pFunction, pParamSetter);
}

void ScriptManager::deleteScript(const std::string &pName) {
    if(mScripts.count(pName) > 0) {
        mScripts.erase(pName);
    }
}

void ScriptManager::onTableChanged(const std::string& pTable, const std::string &pType) {
    for(auto& script: mScripts) {
        script.second.execute("onTableChanged", {ScriptValue::makeString(pTable), ScriptValue::makeString(pType)});
    }
}

ScriptManager::ScriptManager(std::shared_ptr<Logger> pLogger)
: mLogger(std::move(pLogger)) {

}
