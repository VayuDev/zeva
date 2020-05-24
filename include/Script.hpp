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

class ScriptValue {
public:
    WrenType type = WREN_TYPE_NULL;
    union {
        double doubleValue;
        bool boolValue;
    };
    std::vector<ScriptValue> listValue;
    std::string stringValue;
    static ScriptValue makeString(const std::string& pStr) {
        ScriptValue ret;
        ret.type = WREN_TYPE_STRING;
        ret.stringValue = pStr;
        return ret;
    }
    static ScriptValue makeInt(int64_t pInt) {
        ScriptValue ret;
        ret.type = WREN_TYPE_NUM;
        ret.doubleValue = pInt;
        return ret;
    }
    static ScriptValue makeDouble(double pDouble) {
        ScriptValue ret;
        ret.type = WREN_TYPE_NUM;
        ret.doubleValue = pDouble;
        return ret;
    }
    static ScriptValue makeNull() {
        return ScriptValue{};
    }
};

class ScriptReturn {
public:
    time_t timestamp = -1;
    std::variant<ScriptValue, std::string> value;
    ScriptReturn() = default;
    explicit ScriptReturn(std::string pError)
    : value(std::move(pError)) {

    }
};

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

    void executeScriptWithCallback(const std::string &pName, const std::string &pFunction,
                                   const std::vector<ScriptValue> &pParamSetter,
                                   std::function<void(const ScriptReturn &)> &&pCallback);
};
