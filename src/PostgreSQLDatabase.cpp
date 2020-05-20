#include "PostgreSQLDatabase.hpp"
#include <memory>
#include <pqxx/pqxx>
#include <cassert>
#include <pqxx/util.hxx>
#include <pqxx/tablereader.hxx>
#include <pqxx/strconv.hxx>
#include "libpq-fe.h"

namespace pqxx {
    template<> struct PQXX_LIBEXPORT string_traits<std::optional<timeval>>
    {
        static constexpr const char *name() noexcept { return "std::optional<timeval>"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(std::optional<timeval> t) { return  !t.has_value(); }
        static std::optional<timeval> null() { return {}; }
        static void from_string(const char Str[], std::optional<timeval> &Obj) {
            struct tm tm;
            strptime(Str, "%Y-%m-%d %H:%M:%S", &tm);
            Obj = timeval{.tv_sec = mktime(&tm), .tv_usec = 0};
        }
    };
}


std::unique_ptr<QueryResult> PostgreSQLDatabase::query(std::string pQuery, std::vector<QueryValue> pPlaceholders) {

    mConnection->prepare(pQuery, pQuery);
    pqxx::work w{*mConnection};
    pqxx::result r;
    if(pPlaceholders.empty()) {
        r = w.exec(pQuery);
    } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto inv = w.prepared(pQuery);
#pragma GCC diagnostic pop
        for(const auto& param: pPlaceholders) {
            switch(param.type) {
                case QueryValueType::INTEGER:
                    inv(param.intValue);
                    break;
                case QueryValueType::STRING:
                    inv(param.stringValue);
                    break;
                default:
                    assert(false);
            }
        }

        r = inv.exec();
    }
    w.commit();

    auto res = std::make_unique<PostgreSQLQueryResult>();
    for(size_t i = 0; i < r.columns(); ++i) {
        try {
            res->mColumns.emplace_back(r.column_name(i));
        } catch(...) {
            res->mColumns.emplace_back("");
        }

    }
    for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
        res->mData.emplace_back();
        for(size_t c = 0; c < row.size(); ++c) {
            auto type = mTypes[row[(int)c].type()];

            QueryValue value;
            if(!row[(int)c].is_null()) {
                switch(type) {
                    case QueryValueType::INTEGER:
                        value.type = QueryValueType::INTEGER;
                        value.intValue = row[(int)c].as<int64_t>();
                        break;
                    case QueryValueType::STRING:
                        value.type = QueryValueType::STRING;
                        value.stringValue = row[(int)c].as<std::string>();
                        break;
                    case QueryValueType::TIME:
                        value.type = QueryValueType::TIME;
                        value.timeValue = *row[(int)c].as<std::optional<timeval>>();
                        break;
                    case QueryValueType::BOOL:
                        value.type = QueryValueType::BOOL;
                        value.boolValue = row[(int)c].as<bool>();
                        break;
                    default:
                        log().error("Unknown type %i", (int)row[(int)c].type());
                        assert(false);
                }
            }

            res->mData.at(res->mData.size() - 1).push_back(std::move(value));
        }
    }

    return res;
}

void PostgreSQLQueryResult::log() const {
    ::log().info("Rows: %i", (int)getRowCount());
    ::log().info("Columns: %i", (int)getColumnCount());
    std::string columnsStr;
    for(size_t i = 0; i < getColumnNames().size(); ++i) {
        columnsStr += getColumnNames().at(i) + ", ";
    }
    ::log().info(columnsStr.c_str());
    for(size_t r = 0; r < getRowCount(); ++r) {
        std::string rowStr = "";
        for(size_t c = 0; c < getColumnCount(); ++c) {
            const auto& val = getValue(r, c);
            switch(val.type) {
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
        ::log().info(rowStr.c_str());
    }
}

PostgreSQLDatabase::PostgreSQLDatabase(std::string pDbName, std::string pUserName, std::string pPassword,
                                       std::string pHost, uint16_t pPort)
                                       : mConnectString("dbname = " + pDbName +
                                                        " user = " + pUserName +
                                                        " password = " + pPassword +
                                                        " hostaddr = " + pHost +
                                                        " port = " + std::to_string(pPort)),
                                         mConnection(std::make_unique<pqxx::connection>(mConnectString)),
                                         mNotificationReceiver(*this) {

    pqxx::nontransaction n{*mConnection};
    pqxx::result r{n.exec("select typname, oid from pg_type;")};

    for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
        auto name = row[0].as<std::string>();
        auto oid = row[1].as<pqxx::oid>();
        auto type = QueryValueType::UNKNOWN;
        if(name.find("int") == 0) {
            type = QueryValueType::INTEGER;
        }
        if(name == "name" || name == "varchar" || name == "text") {
            type = QueryValueType::STRING;
        }
        if(name.find("timestamp") == 0) {
            type = QueryValueType::TIME;
        }
        if(name == "bool") {
            type = QueryValueType::BOOL;
        }
        mTypes[oid] = type;
    }
}

PostgreSQLDatabase::~PostgreSQLDatabase() {
    mConnection->disconnect();
}

std::string PostgreSQLDatabase::performCopyToStdout(const std::string& pQuery) {
    auto conn = PQconnectdb(mConnectString.c_str());
    auto res = PQexec(conn, pQuery.c_str());
    char *buff;
    std::string output;
    int ret;
    while(true) {
        ret = PQgetCopyData(conn, &buff, 0);
        if(ret <  0){
            break;
        }
        if(buff) {
            output.append(buff, ret);
            PQfreemem(buff);
        }
    }
    if(ret == -2) {
        throw std::runtime_error(std::string{"COPY data transfer failed "} + PQerrorMessage(conn));
    }
    PQclear(res);
    res = PQgetResult(conn);
    if(PQresultStatus(res) != PGRES_COMMAND_OK) {
        throw std::runtime_error("PostgreSQL returned not ok!");
    }
    PQclear(res);
    PQfinish(conn);

    return output;
}

void PostgreSQLDatabase::awaitNotifications(int millis) {
    mConnection->await_notification(0, millis * 1000);
}

size_t PostgreSQLQueryResult::getRowCount() const {
    return mData.size();
}

size_t PostgreSQLQueryResult::getColumnCount() const {
    return mData.size() > 0 ? mData.at(0).size() : 0;
}

const QueryValue &PostgreSQLQueryResult::getValue(size_t pRow, size_t pColumn) const {
    return mData.at(pRow).at(pColumn);
}

const std::vector<std::string> &PostgreSQLQueryResult::getColumnNames() const {
    return mColumns;
}

void PostgreSQLDatabase::NotificationReceiver::operator()(const std::string &payload, int) {
    mDb.onNotification(payload);
}

PostgreSQLDatabase::NotificationReceiver::NotificationReceiver(PostgreSQLDatabase &pDb)
: pqxx::notification_receiver(*pDb.mConnection, "tableChange"), mDb(pDb) {
}
