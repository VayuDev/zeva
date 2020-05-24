#pragma once
#include "DatabaseWrapper.hpp"
#include <memory>
#include <map>
#include <pqxx/pqxx>
#include <filesystem>
#include <json/value.h>
#include <repl.h>

class PostgreSQLQueryResult : public QueryResult {
public:
    virtual size_t getRowCount() const override ;
    virtual size_t getColumnCount() const override ;
    virtual const QueryValue& getValue(size_t pRow, size_t pColumn) const override ;
    virtual const std::vector<std::string>& getColumnNames() const override;
    virtual void log() const override;
private:
    friend class PostgreSQLDatabase;
    std::vector<std::vector<QueryValue>> mData;
    std::vector<std::string> mColumns;
};

class PostgreSQLDatabase : public DatabaseWrapper {
public:
    explicit PostgreSQLDatabase(const std::filesystem::path& pConfigFile, bool connect = true);
    PostgreSQLDatabase(std::string pDbName, std::string pUserName, std::string pPassword, std::string pHost, uint16_t pPort);
    ~PostgreSQLDatabase();
    virtual std::unique_ptr<QueryResult> query(std::string pQuery, std::vector<QueryValue> pPlaceholders = {}) override;
    std::string performCopyToStdout(const std::string& pQuery);
    void awaitNotifications(int millis) override;
private:
    friend WrenForeignClassMethods bindForeignClass(WrenVM*, const char*, const char*);
    void init();
    std::string mConnectString;
    std::unique_ptr<pqxx::connection> mConnection;
    std::map<pqxx::oid, QueryValueType> mTypes;

class NotificationReceiver : public pqxx::notification_receiver {
public:
    explicit NotificationReceiver(PostgreSQLDatabase& pDb);
private:
    void operator()(std::string const &payload, int backend_pid) override;
    PostgreSQLDatabase& mDb;
};
    std::optional<NotificationReceiver> mNotificationReceiver;

};