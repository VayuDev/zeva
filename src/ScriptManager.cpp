#include "ScriptManager.hpp"

void ScriptManager::addScript(const std::string& pName, const std::string& pCode) {
    mScripts.emplace(std::piecewise_construct,
                     std::forward_as_tuple(pName),
                     std::forward_as_tuple(pName, pCode));
}    
