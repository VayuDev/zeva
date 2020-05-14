#include "ScriptManager.hpp"

void ScriptManager::addScript(const std::string& pName, const std::string& pCode) {
    mScripts.emplace(std::piecewise_construct,
                     std::forward_as_tuple(pName),
                     std::forward_as_tuple(pName, pCode));
}    

std::future<ScriptReturn> ScriptManager::executeScript(const std::string& pName, const std::string& pFunction, std::function<void(WrenVM*)> pParamSetter) {
    return mScripts.at(pName).execute(pFunction, pParamSetter);
}