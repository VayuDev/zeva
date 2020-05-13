#pragma once
#include "wren.hpp"
#include <map>

class Script {
public:
    Script(WrenHandle* pModuleName);
private:
    WrenHandle *mInstance;
};