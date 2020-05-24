#include <fstream>
#include "Util.hpp"
#include "Script.hpp"
#include "Script.hpp"
#include <unicode/ucnv.h>

Json::Value scriptValueToJson(ScriptValue&& pVal) {
    switch(pVal.type) {
        case WREN_TYPE_STRING:
            return std::move(pVal.stringValue);
        case WREN_TYPE_NUM:
            return pVal.doubleValue;
        case WREN_TYPE_FOREIGN:
        case WREN_TYPE_NULL:
            return Json::Value::null;
        case WREN_TYPE_BOOL:
            return pVal.boolValue;
        case WREN_TYPE_LIST: {
            Json::Value list;
            for(auto & i : pVal.listValue) {
                list.append(scriptValueToJson(std::move(i)));
            }
            return list;
        }
        case WREN_TYPE_UNKNOWN:
            return "(unknown)";
        default:
            assert(false);
    }
}

std::string readWholeFile(const std::filesystem::path& pPath) {
    std::ifstream t(pPath);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    return str;
}

ScriptValue wrenValueToScriptValue(struct WrenVM *pVM, int pSlot) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    ScriptValue ret = {.type = wrenGetSlotType(pVM, pSlot) };
#pragma GCC diagnostic pop
    switch(ret.type) {
        case WrenType::WREN_TYPE_BOOL:
            ret.boolValue = wrenGetSlotBool(pVM, pSlot);
            break;
        case WrenType::WREN_TYPE_NUM:
            ret.doubleValue = wrenGetSlotDouble(pVM, pSlot);
            break;
        case WrenType::WREN_TYPE_STRING: {
            int length;
            const char* bytes = wrenGetSlotBytes(pVM, pSlot, &length);
            ret.stringValue.append(bytes, length);
            break;
        }
        case WrenType::WREN_TYPE_FOREIGN:
        case WrenType::WREN_TYPE_NULL:
            break;
        case WrenType::WREN_TYPE_LIST: {
            const size_t count = wrenGetListCount(pVM, pSlot);
            wrenEnsureSlots(pVM, pSlot + 2);
            for(size_t i = 0; i < count; ++i) {
                wrenGetListElement(pVM, pSlot, i, pSlot + 1);
                ret.listValue.push_back(wrenValueToScriptValue(pVM, pSlot + 1));
            }
            break;
        }
        default:
            assert(false);
            break;
    }
    return ret;
}

bool isValidAscii(const signed char *c, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if(c[i] < 0) return false;
    }
    return true;
}
