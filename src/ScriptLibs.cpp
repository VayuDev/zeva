#include "ScriptLibs.hpp"
#include <cstring>
#include <stdexcept>
#include "Logger.hpp"
#include "PostgreSQLDatabase.hpp"
#include <cassert>
#include <TcpClient.hpp>

WrenForeignClassMethods bindForeignClass(WrenVM*, const char* module, const char* classname) {
    if(strcmp(classname, "Database") == 0) {
        WrenForeignClassMethods methods = {
            .allocate = [] (WrenVM* pVM) {
                if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
                    wrenSetSlotNull(pVM, 0);
                    return;
                }
                void *data = wrenSetSlotNewForeign(pVM, 0, 0, sizeof(PostgreSQLDatabase));
                const char *dbname = wrenGetSlotString(pVM, 1);
                if(wrenGetSlotType(pVM, 2) == WREN_TYPE_STRING) {
                    new(data)PostgreSQLDatabase(dbname,
                            wrenGetSlotString(pVM, 2),
                            wrenGetSlotString(pVM, 3),
                            wrenGetSlotString(pVM, 4),
                            wrenGetSlotDouble(pVM, 5));
                } else {
                    new(data)PostgreSQLDatabase(dbname);
                }

            },
            .finalize = [] (void *data) {
                auto* db = (PostgreSQLDatabase*)data;
                db->~PostgreSQLDatabase();
            },
        };
        return methods;
    } else if(strcmp(classname, "TcpClient") == 0) {
        WrenForeignClassMethods methods = {
                .allocate = [] (WrenVM* pVM) {
                    if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING && wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM) {
                        wrenSetSlotNull(pVM, 0);
                        return;
                    }
                    auto hostname = wrenGetSlotString(pVM, 1);
                    auto port = wrenGetSlotDouble(pVM, 2);
                    void *data = wrenSetSlotNewForeign(pVM, 0, 0, sizeof(TcpClient));
                    if(port <= 0 || port > std::numeric_limits<uint16_t>::max()) {
                        wrenSetSlotNull(pVM, 0);
                        return;
                    }
                    new(data)TcpClient(hostname, port);
                },
                .finalize = [] (void *data) {
                    auto* socket = (TcpClient*)data;
                    socket->~TcpClient();
                },
        };
        return methods;
    }
    WrenForeignClassMethods methods = {.allocate = nullptr, .finalize = nullptr};
    return methods;
}

static void databaseQuerySync(WrenVM *pVM) {
    if(wrenGetSlotType(pVM, 1) == WREN_TYPE_STRING) {
        auto* db = (PostgreSQLDatabase*) wrenGetSlotForeign(pVM, 0);
        const char *text = wrenGetSlotString(pVM, 1);
        wrenEnsureSlots(pVM, 2);
        try {
            auto ret = db->query(text);
            //LOAD VALUES
            wrenSetSlotString(pVM, 1, "Success!");
            wrenEnsureSlots(pVM, 4);
            wrenSetSlotNewList(pVM, 2);
            for(size_t r = 0; r < ret->getRowCount(); ++r) {
                wrenSetSlotNewList(pVM, 3);
                for(size_t c = 0; c < ret->getColumnCount(); ++c) {
                    const auto& sv = ret->getValue(r, c);
                    switch (sv.type) {
                        case QueryValueType::INTEGER:
                            wrenSetSlotDouble(pVM, 4, sv.intValue);
                            break;
                        case QueryValueType::STRING:
                            wrenSetSlotString(pVM, 4, sv.stringValue.c_str());
                            break;
                        case QueryValueType::DOUBLE:
                            wrenSetSlotDouble(pVM, 4, sv.doubleValue);
                            break;
                        case QueryValueType::TNULL:
                            wrenSetSlotNull(pVM, 4);
                            break;
                        case QueryValueType::TIME:
                            wrenSetSlotDouble(pVM, 4, sv.timeValue.tv_sec + sv.timeValue.tv_usec / 1'000'000.0);
                            break;
                        default:
                            assert(false);
                    }
                    wrenInsertInList(pVM, 3, c, 4);
                }
                wrenInsertInList(pVM, 2, r, 3);

            }
        } catch (std::exception& e) {
            wrenSetSlotString(pVM, 1, "Error");
            wrenSetSlotString(pVM, 2, e.what());
        }
        wrenSetSlotNewList(pVM, 0);
        wrenInsertInList(pVM, 0, 0, 1);
        wrenInsertInList(pVM, 0, 1, 2);
    } else {
        wrenSetSlotNull(pVM, 0);
    }
}

static void tcpClientIsConnected(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    wrenSetSlotBool(pVM, 0, socket->isConnected());
}

static void tcpClientSendByte(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
        wrenSetSlotString(pVM, 0, "Error: Pass a string");
        return;
    }
    int length;
    const auto* bytesPointer = (const uint8_t*) wrenGetSlotBytes(pVM, 1, &length);
    if(length != 1) {
        wrenSetSlotString(pVM, 0, "Error: Pass only one character");
        return;
    }
    uint8_t byte = *bytesPointer;
    try {
        socket->sendByte(byte);
        wrenSetSlotNull(pVM, 0);
    } catch(std::exception& e) {
        std::string err = std::string{"Error: "} + e.what();
        wrenSetSlotString(pVM, 0, err.c_str());
    }

}

static void tcpClientRecvByte(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    try {
        auto byte = socket->recvByte();
        wrenSetSlotBytes(pVM, 0, (char*) &byte, 1);
    } catch(std::exception& e) {
        std::string err = std::string{"Error: "} + e.what();
        wrenSetSlotString(pVM, 0, err.c_str());
    }
}

WrenForeignMethodFn bindForeignMethod( 
    WrenVM* vm, 
    const char* module, 
    const char* className, 
    bool isStatic, 
    const char* signature) 
{
    if(strcmp(className, "Database") == 0) {
        if(!isStatic && strcmp(signature, "query(_)") == 0) {
            return databaseQuerySync;
        }
    } else if(strcmp(className, "TcpClient") == 0 && !isStatic) {
        if(strcmp("isConnected()", signature) == 0) {
            return tcpClientIsConnected;
        } else if(strcmp("sendByte(_)", signature) == 0) {
            return tcpClientSendByte;
        } else if(strcmp("recvByte()", signature) == 0) {
            return tcpClientRecvByte;
        }

    }
    assert(false);
}

const char *foreignClassesString() {
    return R"--(
foreign class Database {
    construct new(dbname) {}
    construct new(dbname, username, password, hostname, port) {}
    foreign query(request)
}

foreign class TcpClient {
    construct new(ip, port) {}
    foreign isConnected()
    foreign sendByte(byte)
    foreign recvByte()
}
)--";
}
/*

    foreign sendString(str)
    foreign recvString()
    */