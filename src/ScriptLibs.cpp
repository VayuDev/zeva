#include "ScriptLibs.hpp"
#include <cstring>
#include <stdexcept>
#include "Logger.hpp"
#include "PostgreSQLDatabase.hpp"
#include <cassert>

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
    }
    WrenForeignClassMethods methods = {.allocate = nullptr, .finalize = nullptr};
    return methods;
}

static void databaseQuerySync(WrenVM *pVM) {
    if(wrenGetSlotType(pVM, 1) == WREN_TYPE_STRING) {
        PostgreSQLDatabase* db = (PostgreSQLDatabase*) wrenGetSlotForeign(pVM, 0);
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
)--";
}