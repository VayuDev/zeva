#include "ScriptManager.hpp"
#include "wren.hpp"
#include "Logger.hpp"
#include <iostream>

static std::string toFunctionSignature(const std::string& pName, size_t pArity) {
    bool first = true;
    std::string ret = pName + "(";
    for(size_t i = 0; i < pArity; ++i) {
        if(first) {
            first = false;
        } else {
            ret += ",";
        }
        ret += "_";
    }
    ret += ")";
    return ret;
}

ScriptManager::ScriptManager() {
    WrenConfiguration config; 
    wrenInitConfiguration(&config);
    config.errorFn = [] (WrenVM* pVM, WrenErrorType, const char* module, int line, const char* message) {
        std::string msg = (module ? module : "(null)") + std::string{" ("} + std::to_string(line) + "): " + message + "\n";
        log().error(msg);
        ScriptManager *self = (ScriptManager*) wrenGetUserData(pVM);
        self->setLastError(std::move(msg));
    };
    config.writeFn = [] (WrenVM*, const char* text) {
        log() << text;
    };
    mVM = wrenNewVM(&config);
    wrenSetUserData(mVM, this);

    //setup functions
    auto append = [this] (std::string pFuncName, size_t pArity) {
        auto sign = toFunctionSignature(pFuncName, pArity);
        mFunctions[pFuncName] = wrenMakeCallHandle(mVM, sign.c_str());
    };
    append("onFileChange", 2);
    append("onRunOnce", 1);
    append("onAudio", 2);
    append("new", 0);
}

ScriptManager::~ScriptManager() {
    for(auto& func: mFunctions) {
        wrenReleaseHandle(mVM, func.second);
    }
    for(auto& script: mScripts) {
        wrenReleaseHandle(mVM, script.second.mInstance);
    }
    wrenFreeVM(mVM);
}

void ScriptManager::setLastError(std::string pLastError) {
    mLastError = pLastError;
}

std::string ScriptManager::popLastError() {
    std::string ret = std::move(mLastError);
    mLastError.clear();
    return ret;
}
void ScriptManager::compile(const std::string& pModule, const std::string& pCode) {
    auto compileRes = wrenInterpret(mVM, pModule.c_str(), pCode.c_str());
    if(compileRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
        throw std::runtime_error("Compilation failed: " + popLastError());
    }
    wrenEnsureSlots(mVM, 1);
    wrenGetVariable(mVM, pModule.c_str(), "ScriptModule", 0);
    auto interpretRes = wrenCall(mVM, mFunctions["new"]);
    if(interpretRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
        throw std::runtime_error("Instantiation failed: " + popLastError());
    }
    auto instance = wrenGetSlotHandle(mVM, 0);
    mScripts.emplace(std::make_pair(pModule, instance));
}
