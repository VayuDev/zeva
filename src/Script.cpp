#include "Script.hpp"
#include "wren.hpp"
#include <iostream>
#include <cstring>
#include "ScriptLibs.hpp"
#include <cassert>
#include <Util.hpp>
#include <nlohmann/json.hpp>
#include <drogon/HttpAppFramework.h>

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
        std::string msg = (module ? module : "(null)") + std::string{" ("} + std::to_string(line) + "): " + message;
        auto *self = (Script*) wrenGetUserData(pVM);

        LOG_ERROR << self->mModuleName << ": " << msg;
        self->setLastError(std::move(msg));
    };
    config.writeFn = [] (WrenVM* pVM, const char* text) {
        std::string str{text};
        auto *self = (Script*) wrenGetUserData(pVM);
        if(!str.empty() && str != "\n") {
            LOG_INFO << self->mModuleName.c_str() << ": " << str.c_str();
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
    append("onTableChanged", 2);
    append("onRunOnce", 1);
    append("drawImage", 2);
    append("new", 0);
    
    //compile parent script
    auto compileRes = wrenInterpret(mVM, pModule.c_str(), 
    R"(
class Script {
    construct new() {}
    onRunOnce(a) {}
    onTableChanged(a, b) {}
    drawImage(width, height) {}
})");

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
    auto& thread = mExecuteThread.emplace([this] {
        while(mShouldRun) {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            mQueueAwaitCV.wait(lock);
            while(!mWorkQueue.empty()) {
                auto& work = mWorkQueue.front();
                ScriptReturn ret;
                try {
                    ret = work.second();
                } catch(std::exception& e) {
                    ret.value = std::string{e.what()};
                }
                auto *returnError = std::get_if<std::string>(&ret.value);
                if(returnError) {
                    LOG_ERROR << mModuleName << " failed with " << returnError;
                } else {
                    ScriptValue val = std::get<ScriptValue>(ret.value);
                    if(val.type != WREN_TYPE_NULL) {
                        std::string jsonStr;
                        try {
                            jsonStr = scriptValueToJson(std::move(val)).asString();
                        } catch(...) {
                            jsonStr = "(unable to parse)";
                        }

                        LOG_INFO << mModuleName << " returned with " << jsonStr;
                    }
                }

                //delete old return values that haven't been taken out yet
                const auto now = ret.timestamp;
                for(auto it = mResultList.cbegin(); it != mResultList.cend();) {
                    if(it->second.timestamp + 60 <= now) {
                        it = mResultList.erase(it);
                    } else {
                        ++it;
                    }
                }

                mResultList.emplace_front(std::make_pair(work.first, ret));
                mWorkQueue.pop();
            }
        }
    });
    char threadNameBuff[16];
    snprintf(threadNameBuff, 16, "script_%s", mModuleName.c_str());
    threadNameBuff[15] = '\0';
    pthread_setname_np(thread.native_handle(), threadNameBuff);
}
Script::Script(const std::string& pModule, const std::string& pCode) {
    create(pModule, pCode);
}

Script::~Script() {
    mShouldRun = false;
    mQueueAwaitCV.notify_all();

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
    mWorkQueue.emplace(std::make_pair(this_id, [this, pFunctionName, pParamSetter] () -> ScriptReturn {
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
        auto interpretResult = wrenCall(mVM, mFunctions.at(pFunctionName));
        if(interpretResult != WrenInterpretResult::WREN_RESULT_SUCCESS) {
            throw std::runtime_error("Running script failed: " + popLastError());
        }
        ScriptReturn ret;
        ret.value = wrenValueToScriptValue(mVM, 0);
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC_COARSE, &t);
        ret.timestamp = t.tv_sec;
        return ret;
    }));
    lock.unlock();
    mQueueAwaitCV.notify_all();
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