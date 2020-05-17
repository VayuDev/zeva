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

struct ScriptValue {
    WrenType type;
    union {
        double doubleValue;
        bool boolValue;
    };
    std::string stringValue;
};

using ScriptReturn = std::variant<ScriptValue, std::string>;

class Script final {
public:
    Script(const std::filesystem::path& pSourcePath);
    Script(const std::string& pModule, const std::string& pCode);
    Script(const Script&) = delete;
    Script(Script&&) = delete;
    void operator=(const Script&) = delete;
    void operator=(Script&&) = delete;
    ~Script();
    
    void setLastError(std::string pLastError);
    std::future<ScriptReturn> execute(const std::string& pFunctionName, std::function<void(WrenVM*)> pParamSetter);
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
    std::optional<std::thread> mExecuteThread;
    
    std::queue<std::pair<int, std::function<ScriptReturn()>>> mWorkQueue;
    std::list<std::pair<int, ScriptReturn>> mResultList;
};
