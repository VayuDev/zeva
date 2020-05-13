#pragma once
#include "wren.hpp"
#include <map>

class Script {
public:
    Script(WrenHandle* pModuleName);
    Script(const Script&) = delete;
    Script(Script&&) = delete;
    void operator=(const Script&) = delete;
    void operator=(Script&&) = delete;
private:
    friend class ScriptManager;
    WrenHandle *mInstance;
};