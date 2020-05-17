#include "ScriptManager.hpp"

#include <utility>

void ScriptManager::addScript(const std::string& pName, const std::string& pCode) {
    if(mScripts.count(pName) > 0) {
        mScripts.erase(pName);
    }
    mScripts.emplace(std::piecewise_construct,
                     std::forward_as_tuple(pName),
                     std::forward_as_tuple(pName, pCode));
}    

std::future<ScriptReturn> ScriptManager::executeScript(const std::string& pName, const std::string& pFunction, std::function<void(WrenVM*)> pParamSetter) {
    return mScripts.at(pName).execute(pFunction, std::move(pParamSetter));
}