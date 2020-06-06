#pragma once
#include <array>
#include <string>
#include <json/json.h>
#include <variant>
#include "ScriptValue.hpp"

class ScriptBindings final {
public:
    ScriptBindings(const std::string& pModule, const std::string& pCode);
    ~ScriptBindings();

    //delete move and copy constructors
    ScriptBindings(const ScriptBindings&) = delete;
    ScriptBindings(ScriptBindings&&) = delete;
    void operator=(const ScriptBindings&) = delete;
    void operator=(const ScriptBindings&&) = delete;

    std::future<ScriptBindingsReturn> execute(const std::string& pFunctionName, const std::vector<ScriptValue>&);
    const std::string& getCode();
private:
    int mInputFd, mOutputFd;
    pid_t mPid;
    std::string mCode, mModule;
    std::mutex mFdMutex;
    /*
     *

    std::atomic<bool> mShouldRun = true;
    std::atomic<int> mIdCounter = 0;
    std::mutex mQueueMutex;
    std::condition_variable mQueueAwaitCV;
    std::optional<std::thread> mExecuteThread;

    std::queue<std::pair<int, std::function<ScriptReturn()>>> mWorkQueue;
    std::list<std::pair<int, ScriptReturn>> mResultList;
     */
};
