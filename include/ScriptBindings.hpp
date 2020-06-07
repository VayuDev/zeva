#pragma once
#include <array>
#include <string>
#include <json/json.h>
#include <variant>
#include "ScriptValue.hpp"

class ProcessDiedException : public std::exception {

};

class ScriptBindings final {
public:
    ScriptBindings(const std::string& pModule, const std::string& pCode);
    ~ScriptBindings();

    //delete move and copy constructors
    ScriptBindings(const ScriptBindings&) = delete;
    ScriptBindings(ScriptBindings&&) = delete;
    void operator=(const ScriptBindings&) = delete;
    void operator=(const ScriptBindings&&) = delete;

    std::future<ScriptBindingsReturn> execute(const std::string& pFunctionName, const std::vector<ScriptValue>&, size_t pDepth = 0);
    const std::string& getCode();
private:
    int mInputFd, mOutputFd;
    pid_t mPid;
    std::string mCode, mModule;
    std::recursive_mutex mFdMutex;
    std::string safeRead(char& cmd);
    void safeWrite(char cmd, const void* buffer, size_t length);
    void killChild();
    void spawnChild();

    size_t mSpawns = 0;
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
