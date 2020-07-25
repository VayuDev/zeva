#include "Util.hpp"
#include <cassert>
#include <fstream>



Json::Value scriptValueToJson(const ScriptValue &pVal) {
  switch (pVal.type) {
    case WREN_TYPE_STRING:
      if (!isValidAscii(
          reinterpret_cast<const signed char *>(pVal.stringValue.c_str()),
          pVal.stringValue.size()))
        throw std::runtime_error("Wren value contains invalid ascii!");
      return pVal.stringValue;
    case WREN_TYPE_NUM:
      return pVal.doubleValue;
    case WREN_TYPE_FOREIGN:
    case WREN_TYPE_NULL:
      return Json::Value::null;
    case WREN_TYPE_BOOL:
      return pVal.boolValue;
    case WREN_TYPE_LIST: {
      Json::Value list;
      for (auto &i : pVal.listValue) {
        list.append(scriptValueToJson(i));
      }
      return list;
    }
    case WREN_TYPE_UNKNOWN:
      return "(unknown)";
    default:
      assert(false);
  }
}


std::string readWholeFile(const std::filesystem::path &pPath) {
  std::ifstream t(pPath);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  return str;
}

bool isValidAscii(const signed char *c, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (c[i] < 0)
      return false;
  }
  return true;
}
