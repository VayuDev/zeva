#pragma once
#include <map>
#include <string>
#include <functional>
#include <wren.hpp>
#include <variant>
#include <future>
#include <list>
#include <queue>

struct ScriptValue {
    WrenType type;
    union {
        char *stringValue;
        double doubleValue;
        bool boolValue;
    };
};

using ScriptReturn = std::variant<ScriptValue, std::string>;

class Script final {
public:
    Script(const std::string& pModule, const std::string& pCode);
    Script(const Script&) = delete;
    Script(Script&&) = delete;
    void operator=(const Script&) = delete;
    void operator=(Script&&) = delete;
    ~Script();
    
    void setLastError(std::string pLastError);
    std::future<ScriptReturn> execute(const std::string& pFunctionName, std::function<void(WrenVM*)> pParamSetter);
private:
    std::string popLastError();
    std::string mModuleName;
    WrenVM *mVM;
    WrenHandle *mInstance;
    std::string mLastError;
    std::map<std::string, WrenHandle*> mFunctions;
    
    std::atomic<bool> mShouldRun = true;
    std::atomic<int> mIdCounter = 0;
    std::mutex mQueueMutex;
    std::optional<std::thread> mExecuteThread;
    
    std::queue<std::pair<int, std::function<ScriptReturn()>>> mWorkQueue;
    std::list<std::pair<int, ScriptReturn>> mResultList;
};
