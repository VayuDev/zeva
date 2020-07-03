#include "PostgreSQLDatabase.hpp"
#include "libpq-fe.h"
#include <Util.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

std::unique_ptr<QueryResult>
PostgreSQLDatabase::query(std::string pQuery,
                          std::vector<QueryValue> pPlaceholders) {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  assert(mCConnection);
  auto oids = genOidVector(pPlaceholders);
  std::vector<char *> values;
  for (const auto &val : pPlaceholders) {
    auto str = val.toString();
    char *data = new char[str.size() + 1];
    strcpy(data, str.c_str());
    values.emplace_back(data);
  }

  auto result = PQexecParams(mCConnection, pQuery.c_str(), pPlaceholders.size(),
                             oids.data(), values.data(), nullptr, nullptr, 0);
  for (char *c : values) {
    delete[] c;
  }
  checkForNewNotifications();
  if (PQresultStatus(result) != PGRES_TUPLES_OK) {
    if (PQresultStatus(result) == PGRES_COMMAND_OK) {
      PQclear(result);
      return nullptr;
    } else {
      PQclear(result);
      throw std::runtime_error("PostgreSQL error: " +
                               std::string{PQerrorMessage(mCConnection)});
    }
  }
  const auto rows = PQntuples(result);
  const auto columns = PQnfields(result);
  auto res = std::make_unique<PostgreSQLQueryResult>();
  // get column names
  for (int i = 0; i < columns; ++i) {
    try {
      res->mColumns.emplace_back(PQfname(result, i));
    } catch (...) {
      res->mColumns.emplace_back("");
    }
  }
  res->mData.reserve(rows);
  for (int r = 0; r < rows; ++r) {
    std::vector<QueryValue> row;
    for (int c = 0; c < columns; ++c) {
      QueryValue ret;
      if (PQgetisnull(result, r, c)) {
        ret.type = QueryValueType::TNULL;
      } else {
        char *val = PQgetvalue(result, r, c);
        auto length = PQgetlength(result, r, c);
        auto type = mOidToType.at(PQftype(result, c));
        ret.type = type;
        switch (type) {
        case QueryValueType::STRING:
          ret.stringValue.append(val, length);
          break;
        case QueryValueType::INTEGER:
          ret.intValue = std::stoll(val);
          break;
        case QueryValueType::BOOL:
          assert(length > 0);
          ret.boolValue = val[0] == 't';
          break;
        case QueryValueType::TIME: {
          struct tm tm;
          strptime(val, "%Y-%m-%d %H:%M:%S", &tm);
          ret.timeValue = timeval{.tv_sec = mktime(&tm), .tv_usec = 0};
          break;
        case QueryValueType::DOUBLE:
          ret.doubleValue = std::stod(val);
          break;
        default:
          std::cerr << "Unknown OID " << PQftype(result, c) << "\n";
          assert(false);
        }
        }
      }
      row.emplace_back(std::move(ret));
    }
    res->mData.emplace_back(std::move(row));
  }
  PQclear(result);
  return res;
}

void PostgreSQLQueryResult::log() const {
  std::string columnsStr;
  for (size_t i = 0; i < getColumnNames().size(); ++i) {
    columnsStr += getColumnNames().at(i) + ", ";
  }
  for (size_t r = 0; r < getRowCount(); ++r) {
    std::string rowStr = "";
    for (size_t c = 0; c < getColumnCount(); ++c) {
      const auto &val = getValue(r, c);
      switch (val.type) {
      case QueryValueType::INTEGER:
        rowStr += std::to_string(val.intValue) + ", ";
        break;
      case QueryValueType::STRING:
        rowStr += "'" + val.stringValue + "', ";
        break;
      default:
        assert(false);
      }
    }
  }
}

PostgreSQLDatabase::PostgreSQLDatabase(std::filesystem::path pConfigFile) {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  std::ifstream instream{pConfigFile};
  if (!instream)
    throw std::runtime_error(std::string{"Unable to read "} +
                             pConfigFile.string());
  Json::Value config;
  instream >> config;
  const auto dbconfig = config["db_clients"][0];
  mConnectString = "dbname = " + dbconfig["dbname"].asString() +
                   " user = " + dbconfig["user"].asString() +
                   " password = " + dbconfig["passwd"].asString() +
                   " hostaddr = " + dbconfig["host"].asString() +
                   " port = " + std::to_string(5432);

  mCConnection = PQconnectdb(mConnectString.c_str());
  init();
}

PostgreSQLDatabase::PostgreSQLDatabase(std::string pDbName,
                                       std::string pUserName,
                                       std::string pPassword, std::string pHost,
                                       uint16_t pPort)
    : mConnectString("dbname = " + pDbName + " user = " + pUserName +
                     " password = " + pPassword + " hostaddr = " + pHost +
                     " port = " + std::to_string(pPort)) {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  mCConnection = PQconnectdb(mConnectString.c_str());
  init();
}

void PostgreSQLDatabase::init() {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  auto res = PQexec(mCConnection, "SELECT typname, oid FROM pg_type;");
  checkForNewNotifications();
  for (int r = 0; r < PQntuples(res); ++r) {
    std::string name = PQgetvalue(res, r, 0);
    auto oid = (Oid)std::stol(PQgetvalue(res, r, 1));
    auto type = QueryValueType::UNKNOWN;
    bool writeToReverseMap = true;
    if (name.find("int") == 0) {
      type = QueryValueType::INTEGER;
      if (name != "int8") {
        writeToReverseMap = false;
      }
    }

    if (name == "name" || name == "varchar" || name == "text") {
      type = QueryValueType::STRING;
    }
    if (name.find("timestamp") == 0) {
      type = QueryValueType::TIME;
    }
    if (name == "bool") {
      type = QueryValueType::BOOL;
    }
    if (name.find("float") == 0) {
      type = QueryValueType::DOUBLE;
    }
    if (name == "numeric") {
      type = QueryValueType::DOUBLE;
      writeToReverseMap = false;
    }
    mOidToType[oid] = type;
    if (writeToReverseMap) {
      mTypeToOid[type] = oid;
    }
  }
  PQclear(res);

  PQsetNoticeReceiver(
      mCConnection, [](void *, const PGresult *) {}, nullptr);
}

PostgreSQLDatabase::~PostgreSQLDatabase() {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  if (mCConnection) {
    PQfinish(mCConnection);
  }
}

std::string PostgreSQLDatabase::performCopyToStdout(const std::string &pQuery) {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  auto res = PQexec(mCConnection, pQuery.c_str());
  checkForNewNotifications();
  char *buff;
  std::string output;
  int ret;
  while (true) {
    ret = PQgetCopyData(mCConnection, &buff, 0);
    if (ret < 0) {
      break;
    }
    if (buff) {
      output.append(buff, ret);
      PQfreemem(buff);
    }
  }
  if (ret == -2) {
    throw std::runtime_error(std::string{"COPY data transfer failed "} +
                             PQerrorMessage(mCConnection));
  }
  PQclear(res);
  res = PQgetResult(mCConnection);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    throw std::runtime_error("PostgreSQL returned not ok!");
  }
  PQclear(res);

  return output;
}

void PostgreSQLDatabase::awaitNotifications(int millis) {
  std::this_thread::sleep_for(std::chrono::milliseconds(millis));
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  PQconsumeInput(mCConnection);
  checkForNewNotifications();
}

std::vector<Oid>
PostgreSQLDatabase::genOidVector(const std::vector<QueryValue> &pValues) const {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  std::vector<Oid> ret(pValues.size());
  size_t i = 0;
  for (auto val : pValues) {
    ret.at(i++) = mTypeToOid.at(val.type);
  }
  return ret;
}

void PostgreSQLDatabase::checkForNewNotifications() {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  pgNotify *notification;
  do {
    notification = PQnotifies(mCConnection);
    if (notification) {
      if (notification->extra) {
        onNotification(notification->relname, notification->extra);
      }
      PQfreemem(notification);
    }
  } while (notification != nullptr);
}

void PostgreSQLDatabase::listenTo(const std::string &pChannel) {
  std::unique_lock<std::recursive_mutex> lock(mMutex);
  auto query = "LISTEN " + pChannel;
  auto listenResp = PQexec(mCConnection, query.c_str());
  checkForNewNotifications();
  if (PQresultStatus(listenResp) != PGRES_COMMAND_OK) {
    PQclear(listenResp);
    throw std::runtime_error("Failed to listen for table change");
  }
  PQclear(listenResp);
}

size_t PostgreSQLQueryResult::getRowCount() const { return mData.size(); }

size_t PostgreSQLQueryResult::getColumnCount() const {
  return mData.size() > 0 ? mData.at(0).size() : 0;
}

const QueryValue &PostgreSQLQueryResult::getValue(size_t pRow,
                                                  size_t pColumn) const {
  return mData.at(pRow).at(pColumn);
}

const std::vector<std::string> &PostgreSQLQueryResult::getColumnNames() const {
  return mColumns;
}