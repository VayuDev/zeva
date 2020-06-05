#pragma once
#include <string>
#include <optional>
#include <list>
#include <vector>
#include <variant>
#include <functional>
#include <memory>
#include <ctime>
#include <cassert>
#include <tuple>
#include <thread>
#include <condition_variable>
#include <set>
#include <queue>
#include <atomic>

enum class QueryValueType {
    INTEGER,
    DOUBLE,
    BOOL,
    STRING,
    TNULL,
    TIME,
    UNKNOWN
};

class QueryValue {
public:
    union {
        int64_t intValue;
        double doubleValue;
        timeval timeValue;
        bool boolValue;
    };
    std::string stringValue;
    QueryValueType type = QueryValueType::TNULL;
    static inline QueryValue makeInt(int64_t pInt) {
        QueryValue ret;
        ret.type = QueryValueType::INTEGER;
        ret.intValue = pInt;
        return ret;
    }
    static inline QueryValue makeString(std::string&& pStr) {
        QueryValue ret;
        ret.type = QueryValueType::STRING;
        ret.stringValue = std::move(pStr);
        return ret;
    }
    static inline QueryValue makeString(const std::string& pStr) {
        QueryValue ret;
        ret.type = QueryValueType::STRING;
        ret.stringValue = pStr;
        return ret;
    }
    [[nodiscard]] std::string toString() const {
        switch(type) {
            case QueryValueType::INTEGER:
                return std::to_string(intValue);
            case QueryValueType::DOUBLE:
                return std::to_string(doubleValue);
            case QueryValueType::STRING:
                return stringValue;
            case QueryValueType::BOOL:
                return std::to_string(boolValue);
            case QueryValueType::TIME: {
                char out[512];
                struct tm tm;
                localtime_r(&timeValue.tv_sec, &tm);
                strftime(out, 512, "%Y-%m-%d %H:%M:%S", &tm);
                return out;
            }
            default:
                assert(false);
        }
    }
};

class QueryResult {
public:
    virtual ~QueryResult() = default;
    virtual size_t getRowCount() const = 0;
    virtual size_t getColumnCount() const = 0;
    virtual const QueryValue& getValue(size_t pRow, size_t pColumn) const = 0;
    virtual const std::vector<std::string>& getColumnNames() const = 0;
    virtual void log() const {};
};

class DatabaseWrapper {
public:
    DatabaseWrapper();
    virtual ~DatabaseWrapper();
    virtual std::unique_ptr<QueryResult> query(std::string pQuery, std::vector<QueryValue> pPlaceholders = {}) = 0;
    void addListener(const std::string& pChannel, std::function<void(const std::string&)> pListener);
    virtual void awaitNotifications(int millis) {(void)millis;};
protected:
    virtual void listenTo(const std::string& pChannel) = 0;
    virtual void onNotification(const std::string& pChannel, const std::string& pPayload);
private:
    std::list<std::pair<std::string, std::function<void(const std::string&)>>> mListeners;
    std::set<std::string> mListeningTo;

    std::mutex mNotificationMutex;
    std::queue<std::pair<std::string, std::string>> mNotificationQueue;
    std::atomic<bool> mShouldRun = true;
    std::condition_variable mNotifier;
    std::thread mNotificationThread;
};
