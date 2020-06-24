#pragma once
#include "ScriptValue.hpp"
#include <filesystem>
#include <json/json.h>
#include <optional>
#include <string.h>

Json::Value scriptValueToJson(const ScriptValue &pVal);
Json::Value wrenValueToJsonValue(struct WrenVM *pVM, int pSlot);

std::string readWholeFile(const std::filesystem::path &pPath);
ScriptValue wrenValueToScriptValue(struct WrenVM *pVM, int pSlot);

bool isValidAscii(const signed char *c, size_t len);

inline void throwError(const char *msg) {
  char buff[1024];
  strerror_r(errno, buff, sizeof(buff));
  std::string err{msg};
  err += ": ";
  err += buff;
  throw std::runtime_error(err);
}

template <typename T> class Badge {
private:
  Badge() = default;
  friend T;
};

class DestructWrapper {
public:
  DestructWrapper(std::function<void()> &&pFunc) : mFunc(std::move(pFunc)) {}
  ~DestructWrapper() { mFunc(); }
  // TODO copy etc.
private:
  std::function<void()> mFunc;
};

inline const char *getConfigFileLocation() {
  if (std::filesystem::exists("assets/local.json"))
    return "assets/local.json";
  return "assets/config.json";
}

inline bool isMusicFile(const std::string &pPath) {
  static constexpr std::array<const char *, 3> extensions = {".mp3", ".ogg",
                                                             ".wav"};
  auto extension = std::filesystem::path(pPath).extension();
  return std::find(extensions.cbegin(), extensions.cend(), extension) !=
         extensions.cend();
}

inline char *readWholeFileCString(const char *pPath, bool pAllowZeroes = true) {
  FILE *file = fopen(pPath, "r");
  if (!file) {
    return nullptr;
  }
  fseek(file, 0, SEEK_END);
  size_t length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *data = (char *)malloc(length + 1);
  fread(data, 1, length, file);

  if (!pAllowZeroes && memmem(data, length, "\0", 1) != nullptr) {
    free(data);
    return nullptr;
  }
  data[length] = '\0';
  fclose(file);
  return data;
}

inline std::string
getScriptClassNameFromScriptName(const std::string &pScriptName) {
  std::stringstream ret;
  bool printedChar = false;
  for (auto c : pScriptName) {
    if (isalnum(c)) {
      ret << c;
      printedChar = true;
    } else if (printedChar) {
      ret << '_';
    }
  }
  return ret.str();
}