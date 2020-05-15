#include "ScriptLibs.hpp"
#include <cstring>
#include <stdexcept>
#include "Logger.hpp"

WrenForeignClassMethods bindForeignClass(WrenVM*, const char* module, const char* classname) {
    if(strcmp(classname, "Database") == 0) {
        WrenForeignClassMethods methods = {
            .allocate = [] (WrenVM* pVM) {
                void *data = wrenSetSlotNewForeign(pVM, 0, 0, sizeof(DatabaseNetworkConnection));
                const char *hostname = wrenGetSlotString(pVM, 1);
                int port = (int) wrenGetSlotDouble(pVM, 2);
                new(data)DatabaseNetworkConnection(hostname, port);
            },
            .finalize = [] (void *data) {
                DatabaseNetworkConnection* db = (DatabaseNetworkConnection*)data;
                db->~DatabaseNetworkConnection();
            },
        };
        return methods;
    }
    WrenForeignClassMethods methods = {.allocate = nullptr, .finalize = nullptr};
    return methods;
}

static void databaseQuerySync(WrenVM *pVM) {
    if(wrenGetSlotType(pVM, 1) == WREN_TYPE_STRING) {
        DatabaseNetworkConnection* db = (DatabaseNetworkConnection*) wrenGetSlotForeign(pVM, 0);
        const char *text = wrenGetSlotString(pVM, 1);
        wrenEnsureSlots(pVM, 2);
        try {
            QueryResult ret = db->querySync(text);
            if(ret.isError()) {
                wrenSetSlotString(pVM, 1, "Error");
                wrenSetSlotString(pVM, 2, ret.getError()->c_str());
            } else {
                //LOAD VALUES
                wrenSetSlotString(pVM, 1, "Success!");
                const Value *v = ret.getValue();
                wrenEnsureSlots(pVM, 4);
                wrenSetSlotNewList(pVM, 2);
                for(size_t r = 0; r < v->rows; ++r) {
                    wrenSetSlotNewList(pVM, 3);
                    for(size_t c = 0; c < v->columns; ++c) {
                        SingleValue *sv = value_get(v, r, c);
                        switch (sv->valueType) {
                            case VALUE_INT:
                                wrenSetSlotDouble(pVM, 4, sv->intValue);
                                break;
                            case VALUE_DOUBLE:
                                wrenSetSlotDouble(pVM, 4, sv->doubleValue);
                                break;
                            case VALUE_NULL:
                                wrenSetSlotNull(pVM, 4);
                                break;
                            case VALUE_STRING:
                                wrenSetSlotString(pVM, 4, sv->stringValue);
                                break;
                            case VALUE_BOOL:
                                wrenSetSlotBool(pVM, 4, sv->boolValue);
                                break;
                            case VALUE_INTERVAL:
                            case VALUE_TIME:
                                time_normalize(&sv->timeValue);
                                wrenSetSlotDouble(pVM, 4, sv->timeValue.tv_sec + sv->timeValue.tv_nsec / 1'000'000'000.0);
                                break;
                            default:
                                ASSERT_NOT_REACHED();
                        }
                        wrenInsertInList(pVM, 3, c, 4);
                    }
                    wrenInsertInList(pVM, 2, r, 3);
                }
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
    ASSERT_NOT_REACHED();
}

const char *foreignClassesString() {
    return R"--(
foreign class Database {
    construct new(hostname, port) {}
    foreign query(request)
})--";
}