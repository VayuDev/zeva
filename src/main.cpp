#include "wren.hpp"
#include <cassert>
#include <iostream>

void onError(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message) {
    std::cout << (module ? module : "(null)") << " (" << std::to_string(line) << "): "<< message << "\n";
}

void onWrite(WrenVM* vm, const char* text) {
    std::cout << text;
}

int main() {
    WrenConfiguration config; 
    wrenInitConfiguration(&config);
    config.errorFn = onError;
    config.writeFn = onWrite;
    WrenVM* vm = wrenNewVM(&config);
    WrenInterpretResult result = wrenInterpret( 
        vm, 
        "main", 
        "class MyClass {\n"
        "  static add(a, b) {\n"
        "    return a * b\n"
        "  }\n"
        "}\n");
    assert(result == WrenInterpretResult::WREN_RESULT_SUCCESS);

    wrenEnsureSlots(vm, 1);
    wrenGetVariable(vm, "main", "MyClass", 0);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 0);
    wrenSetSlotHandle(vm, 0, receiver);
    WrenHandle* func = wrenMakeCallHandle(vm, "add(_,_)");
    wrenEnsureSlots(vm, 3);
    wrenSetSlotHandle(vm, 0, receiver);
    wrenSetSlotDouble(vm, 1, 4.3);
    wrenSetSlotDouble(vm, 2, 323.3);
    auto ret = wrenCall(vm, func);
    assert(ret == WREN_RESULT_SUCCESS);
    
    std::cout << wrenGetSlotDouble(vm, 0) << "\n";

    wrenReleaseHandle(vm, func);
    wrenReleaseHandle(vm, receiver);
    
    wrenFreeVM(vm);

}