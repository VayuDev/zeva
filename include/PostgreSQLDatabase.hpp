#pragma once
#include "DatabaseWrapper.hpp"
#include <memory>
#include <map>
#include <pqxx/pqxx>
#include "Logger.hpp"

class PostgreSQLQueryResult : public QueryResult {
public:
    virtual size_t getRowCount() const override ;
    virtual size_t getColumnCount() const override ;
    virtual const QueryValue& getValue(size_t pRow, size_t pColumn) const override ;
    virtual const std::vector<std::string>& getColumnNames() const override ;
    virtual void log() const override;
private:
    friend class PostgreSQLDatabase;
    std::vector<std::vector<QueryValue>> mData;
    std::vector<std::string> mColumns;
};

class PostgreSQLDatabase : public DatabaseWrapper {
public:
    PostgreSQLDatabase(std::string pDbName, std::string pUserName = "postgres", std::string pPassword = "postgres", std::string pHost = "127.0.0.1", uint16_t pPort = 5432);
    ~PostgreSQLDatabase();
    virtual std::unique_ptr<QueryResult> query(std::string pQuery, std::vector<QueryValue> pPlaceholders = {}) override;

private:
    std::unique_ptr<pqxx::connection> mConnection;
    std::map<pqxx::oid, QueryValueType> mTypes;
};
