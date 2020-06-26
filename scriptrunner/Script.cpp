#include "Script.hpp"
#include "ScriptLibs.hpp"
#include "wren.hpp"
#include <PostgreSQLDatabase.hpp>
#include <Util.hpp>
#include <cassert>
#include <cstring>
#include <iostream>
#include <zconf.h>

constexpr const char *PARENT_SCRIPT = R"(
class Script {
    construct new() {}
    onRunOnce(a) {}
    onTableChanged(a, b) {}
    onAudio(a, b, c) {}
    drawImage(width, height) {}
}
)";

static std::string toFunctionSignature(const std::string &pName,
                                       size_t pArity) {
  bool first = true;
  std::string ret = pName + "(";
  for (size_t i = 0; i < pArity; ++i) {
    if (first) {
      first = false;
    } else {
      ret += ",";
    }
    ret += "_";
  }
  ret += ")";
  return ret;
}

static char *loadModule(WrenVM *pVM, const char *name) {
  // try to load from a file
  auto self = (Script *)wrenGetUserData(pVM);
  std::string path = "assets/wren_libs/";
  path.append(name);
  path.append(".wren");
  self->log(LEVEL_INFO, "Loading module: " + path);
  auto data = readWholeFileCString(path.c_str(), false);
  if (data)
    return data;

  // try to load from another script
  auto db = self->getDbConnection();
  auto ret = db->query("SELECT code FROM scripts WHERE name=$1",
                       {QueryValue::makeString(name)});
  if (!ret || ret->getRowCount() == 0) {
    return nullptr;
  }
  auto code = ret->getValue(0, 0).stringValue;
  char *codeCpy = (char *)malloc(strlen(PARENT_SCRIPT) + code.size() + 1);
  strcpy(codeCpy, PARENT_SCRIPT);
  strcpy(codeCpy + strlen(PARENT_SCRIPT), code.c_str());
  return codeCpy;
}

void Script::create(const std::string &pModule, const std::string &pCode) {
  mModuleName = pModule;
  mCode = pCode;
  WrenConfiguration config;
  wrenInitConfiguration(&config);
  config.errorFn = [](WrenVM *pVM, WrenErrorType, const char *module, int line,
                      const char *message) {
    auto *self = (Script *)wrenGetUserData(pVM);
    std::string msg = self->mModuleName + ": " + (module ? module : "(null)") +
                      std::string{" ("} + std::to_string(line) +
                      "): " + message;
    if (msg.at(msg.size() - 1) == '\n') {
      msg.at(msg.size() - 1) = ' ';
    }
    self->log(LEVEL_ERROR, msg);
    self->setLastError(std::move(msg));
  };
  config.writeFn = [](WrenVM *pVM, const char *text) {
    std::string str{text};
    auto *self = (Script *)wrenGetUserData(pVM);
    if (!str.empty() && str != "\n") {
      if (str.at(str.size() - 1) == '\n') {
        str.at(str.size() - 1) = ' ';
      }
      self->log(LEVEL_INFO, self->mModuleName + ": " + str);
    }
  };
  config.bindForeignClassFn = bindForeignClass;
  config.bindForeignMethodFn = bindForeignMethod;
  config.loadModuleFn = loadModule;
  config.userData = this;
  mVM = wrenNewVM(&config);

  // setup functions
  auto append = [this](std::string pFuncName, size_t pArity) {
    auto sign = toFunctionSignature(pFuncName, pArity);
    mFunctions[pFuncName] = wrenMakeCallHandle(mVM, sign.c_str());
  };
  append("onTableChanged", 2);
  append("onRunOnce", 1); // TODO rename
  append("onAudio", 3);
  append("drawImage", 2);
  append("new", 0);
  append("gc", 0);

  // compile parent script
  auto compileRes = wrenInterpret(mVM, pModule.c_str(), PARENT_SCRIPT);

  if (compileRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
    throw std::runtime_error("Base Script compilation failed: " +
                             popLastError());
  }

  // compile module
  compileRes = wrenInterpret(mVM, pModule.c_str(), pCode.c_str());
  if (compileRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
    throw std::runtime_error("Compilation failed: " + popLastError());
  }
  auto scriptClassName = getScriptClassNameFromScriptName(mModuleName);
  wrenEnsureSlots(mVM, 1);
  wrenGetVariable(mVM, pModule.c_str(), scriptClassName.c_str(), 0);
  auto interpretRes = wrenCall(mVM, mFunctions["new"]);
  if (interpretRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
    throw std::runtime_error("Instantiation failed: " + popLastError());
  }
  mInstance = wrenGetSlotHandle(mVM, 0);
}
Script::Script(const std::string &pModule, const std::string &pCode)
    : mDbConnection(std::make_shared<PostgreSQLDatabase>()) {
  create(pModule, pCode);
}

Script::~Script() {
  for (auto &func : mFunctions) {
    wrenReleaseHandle(mVM, func.second);
  }
  wrenReleaseHandle(mVM, mInstance);

  wrenFreeVM(mVM);
}

void Script::setLastError(std::string pLastError) {
  if (!mLastError.empty())
    mLastError += "\n";
  mLastError += pLastError;
}

std::string Script::popLastError() {
  std::string ret = std::move(mLastError);
  mLastError.clear();
  return ret;
}

ScriptBindingsReturn Script::execute(const std::string &pFunctionName,
                                     const Json::Value &pParamSetter) {
  wrenEnsureSlots(mVM, pParamSetter.size() + 1);
  wrenSetSlotHandle(mVM, 0, mInstance);
  size_t i = 0;
  for (const auto &val : pParamSetter) {
    switch (val.type()) {
    case Json::stringValue:
      wrenSetSlotString(mVM, i + 1, val.asCString());
      break;
    case Json::uintValue:
    case Json::intValue:
      wrenSetSlotDouble(mVM, i + 1, val.asInt64());
      break;
    case Json::realValue:
      wrenSetSlotDouble(mVM, i + 1, val.asDouble());
      break;
    case Json::nullValue:
      wrenSetSlotNull(mVM, i + 1);
      break;
    default:
      std::cerr << "Unknown value type: " << val.type() << "\n";
      assert(false);
    }
    ++i;
  }
  auto interpretResult = wrenCall(mVM, mFunctions.at(pFunctionName));
  if (interpretResult != WrenInterpretResult::WREN_RESULT_SUCCESS) {
    throw std::runtime_error("Running script failed: " + popLastError());
  }

  ScriptBindingsReturn ret;
  if (wrenGetSlotType(mVM, 0) == WREN_TYPE_STRING) {
    int length;
    auto str = wrenGetSlotBytes(mVM, 0, &length);
    ret = std::string{str, (size_t)length};
  } else {
    ret = wrenValueToJsonValue(mVM, 0);
  }
  // perform gc
  wrenGetVariable(mVM, mModuleName.c_str(), "System", 0);
  auto interpretRes = wrenCall(mVM, mFunctions["gc"]);
  if (interpretRes != WrenInterpretResult::WREN_RESULT_SUCCESS) {
    throw std::runtime_error("GC failed: " + popLastError());
  }

  return ret;
}

void Script::log(int pErr, const std::string &pMsg) const {
  static pid_t sPid = getpid();

  std::string toLog;
  auto epoch = time(0);
  struct tm tm_time;
  gmtime_r(&epoch, &tm_time);
  char buf[132] = {0};
  snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d UTC %04d",
           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, 0,
           static_cast<int>(sPid));
  toLog = buf;
  switch (pErr) {
  case LEVEL_INFO:
    toLog += " INFO [ScriptLog] ";
    break;
  case LEVEL_WARNING:
    toLog += " WARN [ScriptLog] ";
    break;
  case LEVEL_ERROR:
    toLog += " ERROR [ScriptLog] ";
    break;
  default:
    assert(false);
  }
  toLog += pMsg;
  if (pErr >= LEVEL_WARNING) {
    std::cerr << toLog << std::endl;
  } else {
    std::cout << toLog << std::endl;
  };
  mDbConnection->query(
      "INSERT INTO log (level, created, msg) VALUES ($1, "
      "CURRENT_TIMESTAMP, $2)",
      {QueryValue::makeInt(pErr), QueryValue::makeString(toLog)});
}
