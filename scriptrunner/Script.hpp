#pragma once
#include "ScriptValue.hpp"
#include <filesystem>
#include <functional>
#include <future>
#include <json/json.h>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <variant>
#include <wren.hpp>

class Script final {
public:
  Script(const std::string &pModule, const std::string &pCode);
  Script(const Script &) = delete;
  Script(Script &&) = delete;
  void operator=(const Script &) = delete;
  void operator=(Script &&) = delete;
  ~Script();

  void setLastError(std::string pLastError);
  ScriptBindingsReturn execute(const std::string &pFunctionName,
                               const Json::Value &);

  inline const std::string &getCode() { return mCode; }

private:
  void create(const std::string &pModule, const std::string &pCode);

  std::string popLastError();
  std::string mModuleName;
  WrenVM *mVM;
  WrenHandle *mInstance;
  std::string mLastError;
  std::map<std::string, WrenHandle *> mFunctions;
  std::string mCode;
};
