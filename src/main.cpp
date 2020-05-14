#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "Script.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"
#include "zegdb.hpp"
#include "common.hpp"
#include <cstring>
#include <cstdlib>

int main() {
    /*DatabaseNetworkConnection conn{"localhost", 5120};
    conn.querySync("CREATE TABLE IF NOT EXISTS scripts (id INTEGER, name TEXT, code TEXT)");
    conn.querySync("SELECT * FROM sample_min_csv");

    using namespace std::chrono_literals;

    std::this_thread::sleep_for(2s);

    conn.querySync("SELECT * FROM sample_min_csv");*/

    ScriptManager manager;
    

    auto[data, len] = readWholeFile("../assets/samples/test.wren");
    char cpy[len + 1];
    memcpy(cpy, data, len);
    cpy[len] = '\0';
    free(data);
    manager.addScript("test", cpy);
    manager.executeScript("test", "onRunOnce", [](WrenVM* vm) {
        wrenSetSlotDouble(vm, 1, 1.2);
    });
    auto ret = manager.executeScript("test", "onRunOnce", [](WrenVM* vm) {
        wrenSetSlotDouble(vm, 1, 1.2);
    }).get();
    
    ScriptValue& sRet = std::get<ScriptValue>(ret);
    switch(sRet.type) {
    case WrenType::WREN_TYPE_NUM:
        log() << std::to_string(sRet.doubleValue);
        break;
    case WrenType::WREN_TYPE_STRING: 
        log() << std::string{sRet.stringValue};
        free(sRet.stringValue);
        break;
    default:
        break;
    }
}
