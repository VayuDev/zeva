#include "ScriptValue.hpp"

Json::Value wrenValueToJsonValue(struct WrenVM *pVM, int pSlot);
ScriptValue wrenValueToScriptValue(struct WrenVM *pVM, int pSlot);