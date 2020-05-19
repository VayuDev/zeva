#include "Script.hpp"
#include "wren.hpp"
#include "Logger.hpp"
#include <iostream>
#include <cstring>
#include "ScriptLibs.hpp"
#include <cassert>
#include <Util.hpp>

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

void Script::create(const std::string& pModule, const std::string& pCode) {
    mModuleName = pModule;
    WrenConfiguration config; 
    wrenInitConfiguration(&config);
    config.errorFn = [] (WrenVM* pVM, WrenErrorType, const char* module, int line, const char* message) {
        std::string msg = (module ? module : "(null)") + std::string{" ("} + std::to_string(line) + "): " + message + "\n";
        log().error(msg.c_str());
        Script *self = (Script*) wrenGetUserData(pVM);
        self->setLastError(std::move(msg));
    };
    config.writeFn = [] (WrenVM*, const char* text) {
        std::string str{text};
        if(str != "" && str != "\n") {
            log().info(str.c_str());
        }
        
    };
    config.bindForeignClassFn = bindForeignClass;
    config.bindForeignMethodFn = bindForeignMethod;
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
    
    //compile parent script
    auto compileRes = wrenInterpret(mVM, pModule.c_str(), 
    "class Script {\n"
    "  construct new() {\n"
    "  }\n"
    "  \n"
    "  onRunOnce(a) {\n"
    "  }\n"
    "  onFileChange(a, b) {\n"
    "  }\n"
    "  onAudio(a, b) {\n"
    "  }\n"
    "}\n");

    if(compileRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
        throw std::runtime_error("Base Script compilation failed: " + popLastError());
    }
    auto compileDbModuleRes = wrenInterpret(mVM, pModule.c_str(), foreignClassesString());
    if(compileDbModuleRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
        throw std::runtime_error("Db module compilation failed: " + popLastError());
    }

    //compile module
    compileRes = wrenInterpret(mVM, pModule.c_str(), pCode.c_str());
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
    
    mExecuteThread.emplace([this] {
        while(mShouldRun) {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            if(!mWorkQueue.empty()) {
                auto& work = mWorkQueue.front();
                std::variant<ScriptValue, std::basic_string<char>> ret;
                try {
                    ret = work.second();
                } catch(std::exception& e) {
                    ret = std::string{e.what()};
                }
                mResultList.emplace_front(std::make_pair(work.first, ret));
                mWorkQueue.pop();
            }
            lock.unlock();
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
        }
    });
}
Script::Script(const std::string& pModule, const std::string& pCode) {
    create(pModule, pCode);
}

Script::~Script() {
    mShouldRun = false;
    std::unique_lock<std::mutex> lock(mQueueMutex);
    if(mExecuteThread && mExecuteThread->joinable()) {
        mExecuteThread->join();
    }
    for(auto& func: mFunctions) {
        wrenReleaseHandle(mVM, func.second);
    }
    wrenReleaseHandle(mVM, mInstance);
    
    wrenFreeVM(mVM);
}

void Script::setLastError(std::string pLastError) {
    mLastError += pLastError;
}

std::string Script::popLastError() {
    std::string ret = std::move(mLastError);
    mLastError.clear();
    return ret;
}

std::future<ScriptReturn> Script::execute(const std::string& pFunctionName, const std::vector<ScriptValue>& pParamSetter) {
    int this_id = mIdCounter++;
    std::unique_lock<std::mutex> lock{mQueueMutex};
    mWorkQueue.emplace(std::make_pair(this_id, [this, pFunctionName, pParamSetter] {
        wrenEnsureSlots(mVM, 4);
        wrenSetSlotHandle(mVM, 0, mInstance);
        for(size_t i = 0; i < pParamSetter.size(); ++i) {
            switch(pParamSetter.at(i).type) {
                case WREN_TYPE_STRING:
                    wrenSetSlotString(mVM, i + 1, pParamSetter.at(i).stringValue.c_str());
                    break;
                case WREN_TYPE_NUM:
                    wrenSetSlotDouble(mVM, i + 1, pParamSetter.at(i).doubleValue);
                    break;
                case WREN_TYPE_BOOL:
                    wrenSetSlotBool(mVM, i + 1, pParamSetter.at(i).boolValue);
                    break;
                case WREN_TYPE_NULL:
                    wrenSetSlotNull(mVM, i + 1);
                    break;
                default:
                    assert(false);
            }
        }
        auto interpretResult = wrenCall(mVM, mFunctions[pFunctionName]);
        if(interpretResult != WrenInterpretResult::WREN_RESULT_SUCCESS) {
            throw std::runtime_error("Running script failed: " + popLastError());
        }
        return wrenValueToScriptValue(mVM, 0);
    }));
    return std::async(std::launch::deferred, [this, this_id] {
        while(mShouldRun) {
            std::unique_lock<std::mutex> lock{mQueueMutex};
            for(auto it = mResultList.begin(); it != mResultList.end(); it++) {
                if(it->first == this_id) {
                    ScriptReturn ret = it->second;
                    mResultList.erase(it);
                    return ret;
                }
            }
            lock.unlock();
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
        }
        return ScriptReturn{std::string{"Error"}};
    });
    
}
