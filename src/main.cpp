#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "ScriptManager.hpp"
#include "Logger.hpp"

int main() {
    Script manager("test", 
    "class ScriptModule {\n"
    "  construct new() {\n"
    "    _b = 0\n"
    "    System.print(\"Constructed!\")\n"
    "    System.print(\"_b is %(_b)\")\n"
    "  }\n"
    "  \n"
    "  onRunOnce(a) {\n"
    "    _b = _b + 3\n"
    "    return _b + a\n"
    "  }\n"
    "}\n");
    manager.execute("onRunOnce", [](WrenVM* vm) {
        wrenSetSlotDouble(vm, 1, 1.2);
    });
    auto ret = manager.execute("onRunOnce", [](WrenVM* vm) {
        wrenSetSlotDouble(vm, 1, 1.2);
    });
    switch(ret.type) {
    case WrenType::WREN_TYPE_NUM:
        log() << std::to_string(ret.doubleValue);
    default:
        break;
    }
}