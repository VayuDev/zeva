#pragma once
#include <string>
#include <optional>
#include <list>
#include <vector>
#include <variant>
#include <memory>

enum class QueryValueType {
    INTEGER,
    DOUBLE,
    BOOL,
    STRING,
    TNULL,
    UNKNOWN
};

class QueryValue {
public:
    union {
        int64_t intValue;
        double doubleValue;
    };
    std::string stringValue;
    QueryValueType type = QueryValueType::TNULL;
    static inline QueryValue makeInt(int64_t pInt) {
        QueryValue ret;
        ret.type = QueryValueType::INTEGER;
        ret.intValue = pInt;
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
};