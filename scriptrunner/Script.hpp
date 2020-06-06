#pragma once
#include <map>
#include <string>
#include <functional>
#include <wren.hpp>
#include <variant>
#include <future>
#include <list>
#include <queue>
#include <filesystem>
#include "ScriptValue.hpp"

class Script final {
public:
    Script(const std::string& pModule, const std::string& pCode);
    Script(const Script&) = delete;
    Script(Script&&) = delete;
    void operator=(const Script&) = delete;
    void operator=(Script&&) = delete;
    ~Script();
    
    void setLastError(std::string pLastError);
    std::future<ScriptReturn> execute(const std::string& pFunctionName, const std::vector<ScriptValue>&);

    inline const std::string& getCode() {
        return mCode;
    }
private:
    void create(const std::string& pModule, const std::string& pCode);

    std::string popLastError();
    std::string mModuleName;
    WrenVM *mVM;
    WrenHandle *mInstance;
    std::string mLastError;
    std::map<std::string, WrenHandle*> mFunctions;
    
    std::atomic<bool> mShouldRun = true;
    std::atomic<int> mIdCounter = 0;
    std::mutex mQueueMutex;
    std::condition_variable mQueueAwaitCV;
    std::optional<std::thread> mExecuteThread;
    
    std::queue<std::pair<int, std::function<ScriptReturn()>>> mWorkQueue;
    std::list<std::pair<int, ScriptReturn>> mResultList;

    std::string mCode;

    //void executeScriptWithCallback(const std::string &pName, const std::string &pFunction,
    //                               const std::vector<ScriptValue> &pParamSetter,
    //                               std::function<void(const ScriptReturn &)> &&pCallback);
};
