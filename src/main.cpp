#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "ScriptManager.hpp"

int main() {
    ScriptManager manager;
    manager.compile("main", 
    "class ScriptModule {\n"
    "  construct new() {"
    "    System.print(5*444)"
    "  }"
    "}");
}