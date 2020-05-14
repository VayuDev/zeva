#include "ScriptManager.hpp"
#include "wren.hpp"
#include "Logger.hpp"
#include <iostream>
#include <cstring>

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

Script::Script(const std::string& pModule, const std::string& pCode)
: mModuleName(pModule) {
    WrenConfiguration config; 
    wrenInitConfiguration(&config);
    config.errorFn = [] (WrenVM* pVM, WrenErrorType, const char* module, int line, const char* message) {
        std::string msg = (module ? module : "(null)") + std::string{" ("} + std::to_string(line) + "): " + message + "\n";
        log().error(msg);
        Script *self = (Script*) wrenGetUserData(pVM);
        self->setLastError(std::move(msg));
    };
    config.writeFn = [] (WrenVM*, const char* text) {
        std::string_view str{text};
        if(str != "" && str != "\n") {
            log() << text;
        }
        
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

    //compile
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
    mInstance = wrenGetSlotHandle(mVM, 0);
    
}

Script::~Script() {
    for(auto& func: mFunctions) {
        wrenReleaseHandle(mVM, func.second);
    }
    wrenReleaseHandle(mVM, mInstance);
    
    wrenFreeVM(mVM);
}

void Script::setLastError(std::string pLastError) {
    mLastError = pLastError;
}

std::string Script::popLastError() {
    std::string ret = std::move(mLastError);
    mLastError.clear();
    return ret;
}

ScriptValue Script::execute(const std::string& pFunctionName, std::function<void(WrenVM*)> pParamSetter) {
    wrenSetSlotHandle(mVM, 0, mInstance);
    pParamSetter(mVM);
    auto interpretResult = wrenCall(mVM, mFunctions[pFunctionName]);
    if(interpretResult != WrenInterpretResult::WREN_RESULT_SUCCESS) {
        throw std::runtime_error("Running script failed: " + popLastError());
    }
    ScriptValue ret = {.type = wrenGetSlotType(mVM, 0) };
    switch(ret.type) {
    case WrenType::WREN_TYPE_BOOL:
        ret.boolValue = wrenGetSlotBool(mVM, 0);
        break;
    case WrenType::WREN_TYPE_NUM:
        ret.doubleValue = wrenGetSlotDouble(mVM, 0);
        break;
    case WrenType::WREN_TYPE_STRING: {
        auto srcStr = wrenGetSlotString(mVM, 0);
        ret.stringValue = (char*) malloc(strlen(srcStr) + 1);
        strcpy(ret.stringValue, srcStr);
        break;
    }
    default:
        break;
    }
    return ret;
}