#include "WrenUtil.hpp"
#include <cassert>

Json::Value wrenValueToJsonValue(struct WrenVM *pVM, int pSlot) {
  switch (wrenGetSlotType(pVM, pSlot)) {
    case WrenType::WREN_TYPE_BOOL:
      return wrenGetSlotBool(pVM, pSlot);
    case WrenType::WREN_TYPE_NUM:
      return wrenGetSlotDouble(pVM, pSlot);
    case WrenType::WREN_TYPE_STRING: {
      int length;
      const char *bytes = wrenGetSlotBytes(pVM, pSlot, &length);
      std::string str(bytes, length);
      return str;
    }
      // call toString
    case WrenType::WREN_TYPE_UNKNOWN:
    case WrenType::WREN_TYPE_FOREIGN:
    case WrenType::WREN_TYPE_MAP: {
      auto handle = wrenMakeCallHandle(pVM, "toString");
      wrenCall(pVM, handle);
      wrenReleaseHandle(pVM, handle);
      return wrenValueToJsonValue(pVM, pSlot);
    }
    case WrenType::WREN_TYPE_NULL:
      return Json::nullValue;
    case WrenType::WREN_TYPE_LIST: {
      const size_t count = wrenGetListCount(pVM, pSlot);
      wrenEnsureSlots(pVM, pSlot + 2);
      Json::Value list;
      for (size_t i = 0; i < count; ++i) {
        wrenGetListElement(pVM, pSlot, i, pSlot + 1);
        list.append(wrenValueToJsonValue(pVM, pSlot + 1));
      }
      return list;
    }
  }
  assert(false);
}

ScriptValue wrenValueToScriptValue(struct WrenVM *pVM, int pSlot) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  ScriptValue ret = {.type = wrenGetSlotType(pVM, pSlot)};
#pragma GCC diagnostic pop
  switch (ret.type) {
    case WrenType::WREN_TYPE_BOOL:
      ret.boolValue = wrenGetSlotBool(pVM, pSlot);
      break;
    case WrenType::WREN_TYPE_NUM:
      ret.doubleValue = wrenGetSlotDouble(pVM, pSlot);
      break;
    case WrenType::WREN_TYPE_STRING: {
      int length;
      const char *bytes = wrenGetSlotBytes(pVM, pSlot, &length);
      ret.stringValue.append(bytes, length);
      break;
    }
    case WrenType::WREN_TYPE_FOREIGN:
    case WrenType::WREN_TYPE_NULL:
      break;
    case WrenType::WREN_TYPE_LIST: {
      const size_t count = wrenGetListCount(pVM, pSlot);
      wrenEnsureSlots(pVM, pSlot + 2);
      for (size_t i = 0; i < count; ++i) {
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
