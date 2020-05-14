#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "Script.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"

int main() {
    ScriptManager manager;
    manager.addScript("test", 
    "class ScriptModule is Script {\n"
    "  construct new() {\n"
    "    _b = 0\n"
    "    System.print(\"Constructed!\")\n"
    "    System.print(\"_b is %(_b)\")\n"
    "  }\n"
    "  \n"
    "  onRunOnce(a) {\n"
    "    _b = _b + a\n"
    "    return _b\n"
    "  }\n"
    "}\n");
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
    default:
        break;
    }
}
