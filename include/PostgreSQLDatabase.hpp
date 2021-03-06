#pragma once
#include "DatabaseWrapper.hpp"
#include "Util.hpp"
#include <filesystem>
#include <json/value.h>
#include <libpq-fe.h>
#include <map>
#include <memory>
#include <mutex>

class PostgreSQLQueryResult : public QueryResult {
public:
  virtual size_t getRowCount() const override;
  virtual size_t getColumnCount() const override;
  virtual const QueryValue &getValue(size_t pRow,
                                     size_t pColumn) const override;
  virtual const std::vector<std::string> &getColumnNames() const override;
  virtual void log() const override;

private:
  friend class PostgreSQLDatabase;
  std::vector<std::vector<QueryValue>> mData;
  std::vector<std::string> mColumns;
};

class PostgreSQLDatabase : public DatabaseWrapper {
public:
  explicit PostgreSQLDatabase(
      std::filesystem::path pConfigFile = getConfigFileLocation());
  PostgreSQLDatabase(std::string pDbName, std::string pUserName,
                     std::string pPassword, std::string pHost, uint16_t pPort);
  ~PostgreSQLDatabase() override;
  virtual std::unique_ptr<QueryResult>
  query(std::string pQuery,
        std::vector<QueryValue> pPlaceholders = {}) override;
  std::string performCopyToStdout(const std::string &pQuery);
  void awaitNotifications(int millis) override;

private:
  void init();
  std::string mConnectString;
  pg_conn *mCConnection = nullptr;
  std::map<Oid, QueryValueType> mOidToType;
  std::map<QueryValueType, Oid> mTypeToOid;
  mutable std::recursive_mutex mMutex;

  void checkForNewNotifications();
  void listenTo(const std::string &pChannel) override;

  [[nodiscard]] std::vector<Oid>
  genOidVector(const std::vector<QueryValue> &pValues) const;
};