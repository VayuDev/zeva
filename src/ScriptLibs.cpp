#include "ScriptLibs.hpp"
#include <cstring>
#include <stdexcept>

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
            VMReturn ret = db->querySync(text);
            if(ret.errorMsg) {
                wrenSetSlotString(pVM, 1, "Error");
                wrenSetSlotString(pVM, 2, ret.errorMsg);
            } else {
                //LOAD VALUES
                wrenSetSlotString(pVM, 1, "Success!");
                wrenSetSlotString(pVM, 2, "Here comes the data...");
            }
            vm_return_content_free(&ret);
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