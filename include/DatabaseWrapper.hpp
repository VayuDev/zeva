#pragma once
#include <string>
#include <optional>
#include <list>
#include <vector>
#include <variant>
#include <functional>
#include <memory>
#include <ctime>

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
    virtual ~DatabaseWrapper() = default;
    virtual std::unique_ptr<QueryResult> query(std::string pQuery, std::vector<QueryValue> pPlaceholders = {}) = 0;
    inline void addListener(std::function<void(const std::string&)> pListener) {
        mListeners.emplace_back(std::move(pListener));
    }
    virtual void awaitNotifications(int millis) {(void)millis;};
protected:
    void onNotification(const std::string& pPayload);
private:
    std::list<std::function<void(const std::string& pPayload)>> mListeners;
};