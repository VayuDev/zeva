#include "ScriptLibs.hpp"
#include "PostgreSQLDatabase.hpp"
#include "Script.hpp"
#include "TcpClient.hpp"
#include "Util.hpp"
#include "httplib.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

template <typename T> struct ScriptStorage {
  T value;
  bool inited = false;
};

using DatabaseStorage = ScriptStorage<PostgreSQLDatabase>;
using TcpClientStorage = ScriptStorage<TcpClient>;

WrenForeignClassMethods bindForeignClass(WrenVM *, const char *module,
                                         const char *classname) {
  (void)module;
  if (strcmp(classname, "Database") == 0) {
    WrenForeignClassMethods methods = {
        .allocate =
            [](WrenVM *pVM) {
              auto *db = (DatabaseStorage *)wrenSetSlotNewForeign(
                  pVM, 0, 0, sizeof(DatabaseStorage));
              db->inited = false;
            },
        .finalize =
            [](void *data) {
              auto *db = reinterpret_cast<DatabaseStorage *>(data);
              if (db && db->inited) {
                db->value.~PostgreSQLDatabase();
              }
              db->inited = false;
            },
    };
    return methods;
  } else if (strcmp(classname, "TcpClient") == 0) {
    WrenForeignClassMethods methods = {
        .allocate =
            [](WrenVM *pVM) {
              auto *self = (Script *)wrenGetUserData(pVM);
              wrenEnsureSlots(pVM, 3);
              auto *data = (TcpClientStorage *)wrenSetSlotNewForeign(
                  pVM, 0, 0, sizeof(TcpClientStorage));
              data->inited = false;
              if (wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING &&
                  wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM) {
                self->log(LEVEL_ERROR,
                          "ScriptLibs TcpClient: Invalid parameter types!");
                return;
              }
              auto hostname = wrenGetSlotString(pVM, 1);
              auto port = wrenGetSlotDouble(pVM, 2);

              if (port <= 0 || port > std::numeric_limits<uint16_t>::max()) {
                self->log(LEVEL_ERROR,
                          std::string{"ScriptLibs TcpClient: Invalid port: "} +
                              std::to_string(port));
                return;
              }
              try {
                new (&data->value) TcpClient(hostname, port);
                data->inited = true;
              } catch (std::exception &e) {
                self->log(
                    LEVEL_ERROR,
                    std::string{"ScriptLibs TcpClient instantiation failed: "} +
                        e.what());
                return;
              }
            },
        .finalize =
            [](void *data) {
              auto *socket = (TcpClientStorage *)data;
              if (socket->inited) {
                socket->value.~TcpClient();
              }
            },
    };
    return methods;
  }
  WrenForeignClassMethods methods = {.allocate = nullptr, .finalize = nullptr};
  return methods;
}

static void passToVM(WrenVM *pVM, int pSlot, const char *pType,
                     const char *pMsg) {
  wrenEnsureSlots(pVM, pSlot + 3);
  wrenSetSlotNewList(pVM, pSlot);
  wrenSetSlotString(pVM, pSlot + 1, pType);
  wrenSetSlotString(pVM, pSlot + 2, pMsg);
  wrenInsertInList(pVM, pSlot, 0, pSlot + 1);
  wrenInsertInList(pVM, pSlot, 1, pSlot + 2);
}

static void databaseCreate(WrenVM *pVM) {
  auto db = (DatabaseStorage *)wrenGetSlotForeign(pVM, 0);
  auto *self = (Script *)wrenGetUserData(pVM);
  try {
    new (&db->value) PostgreSQLDatabase();
    db->inited = true;
  } catch (std::exception &e) {
    self->log(LEVEL_ERROR, std::string{"Script failed to connect to db: "} +
                               e.what() + "\n");
    db->inited = false;
  }
}

void databaseCreateWithParams(WrenVM *pVM) {
  auto db = (DatabaseStorage *)wrenGetSlotForeign(pVM, 0);
  auto *self = (Script *)wrenGetUserData(pVM);
  try {
    if (wrenGetSlotType(pVM, 1) == WREN_TYPE_STRING &&
        wrenGetSlotType(pVM, 2) == WREN_TYPE_STRING &&
        wrenGetSlotType(pVM, 3) == WREN_TYPE_STRING &&
        wrenGetSlotType(pVM, 4) == WREN_TYPE_STRING &&
        wrenGetSlotType(pVM, 5) == WREN_TYPE_NUM) {
      new (&db->value) PostgreSQLDatabase(
          wrenGetSlotString(pVM, 1), wrenGetSlotString(pVM, 2),
          wrenGetSlotString(pVM, 3), wrenGetSlotString(pVM, 4),
          wrenGetSlotDouble(pVM, 5));
      db->inited = true;
    } else {
      self->log(LEVEL_ERROR,
                "Script failed to connect to db: invalid parameters");
      db->inited = false;
    }
  } catch (std::exception &e) {
    self->log(LEVEL_ERROR, std::string{"Script failed to connect to db: "} +
                               e.what() + "\n");
    db->inited = false;
  }
}

static void databaseQuerySync(WrenVM *pVM) {
  if (wrenGetSlotType(pVM, 1) == WREN_TYPE_STRING) {
    auto *db = (DatabaseStorage *)wrenGetSlotForeign(pVM, 0);
    if (!db->inited) {
      wrenSetSlotString(pVM, 1, "Database didn't connect!");
      wrenAbortFiber(pVM, 1);
    } else {
      const char *text = wrenGetSlotString(pVM, 1);
      wrenEnsureSlots(pVM, 5);
      try {
        auto ret = db->value.query(text);
        // LOAD VALUES
        wrenSetSlotString(pVM, 1, "ok");
        wrenSetSlotNewList(pVM, 2);
        if (ret) {
          for (size_t r = 0; r < ret->getRowCount(); ++r) {
            wrenSetSlotNewList(pVM, 3);
            for (size_t c = 0; c < ret->getColumnCount(); ++c) {
              const auto &sv = ret->getValue(r, c);
              switch (sv.type) {
              case QueryValueType::INTEGER:
                wrenSetSlotDouble(pVM, 4, sv.intValue);
                break;
              case QueryValueType::STRING:
                wrenSetSlotString(pVM, 4, sv.stringValue.c_str());
                break;
              case QueryValueType::DOUBLE:
                wrenSetSlotDouble(pVM, 4, sv.doubleValue);
                break;
              case QueryValueType::TNULL:
                wrenSetSlotNull(pVM, 4);
                break;
              case QueryValueType::TIME:
                wrenSetSlotDouble(pVM, 4,
                                  sv.timeValue.tv_sec +
                                      sv.timeValue.tv_usec / 1'000'000.0);
                break;
              default:
                std::cerr << "[SCRIPT] Invalid query value!" << std::endl;
                assert(false);
              }
              wrenInsertInList(pVM, 3, c, 4);
            }
            wrenInsertInList(pVM, 2, r, 3);
          }
          // COLUMNS
          wrenSetSlotNewList(pVM, 3);
          for (size_t c = 0; c < ret->getColumnCount(); ++c) {
            wrenSetSlotString(pVM, 4, ret->getColumnNames().at(c).c_str());
            wrenInsertInList(pVM, 3, c, 4);
          }
        }

      } catch (std::exception &e) {
        wrenSetSlotString(pVM, 1, e.what());
        wrenAbortFiber(pVM, 1);
      }
    }
  } else {
    wrenSetSlotString(pVM, 1, "Please pass a string");
    wrenAbortFiber(pVM, 1);
  }
  wrenSetSlotNewList(pVM, 0);
  wrenInsertInList(pVM, 0, 0, 1);
  wrenInsertInList(pVM, 0, 1, 2);
  wrenInsertInList(pVM, 0, 2, 3);
}

static void tcpClientIsConnected(WrenVM *pVM) {
  auto *socket = (TcpClientStorage *)wrenGetSlotForeign(pVM, 0);
  wrenSetSlotBool(pVM, 0, socket->inited && socket->value.isConnected());
}

static void tcpClientSendByte(WrenVM *pVM) {
  auto *socket = (TcpClientStorage *)wrenGetSlotForeign(pVM, 0);
  if (!socket->inited) {
    passToVM(pVM, 0, "error", "TcpClient not initialized!");
    return;
  }
  if (wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
    passToVM(pVM, 0, "error", "Pass a string");
    return;
  }
  int length;
  const auto *bytesPointer = (const uint8_t *)wrenGetSlotBytes(pVM, 1, &length);
  if (length != 1) {
    passToVM(pVM, 0, "error", "Pass only one character");
    return;
  }
  uint8_t byte = *bytesPointer;
  try {
    socket->value.sendByte(byte);
    wrenSetSlotString(pVM, 0, "ok");
  } catch (std::exception &e) {
    passToVM(pVM, 0, "error", e.what());
  }
}

static void tcpClientRecvByte(WrenVM *pVM) {
  auto *socket = (TcpClientStorage *)wrenGetSlotForeign(pVM, 0);
  if (!socket->inited) {
    passToVM(pVM, 0, "error", "TcpClient not initialized!");
    return;
  }
  try {
    auto byte = socket->value.recvByte();
    std::string byteString{static_cast<char>(byte), 1};
    passToVM(pVM, 0, "ok", byteString.c_str());
  } catch (std::exception &e) {
    passToVM(pVM, 0, "error", e.what());
  }
}

static void tcpClientSendString(WrenVM *pVM) {
  auto *socket = (TcpClientStorage *)wrenGetSlotForeign(pVM, 0);
  if (!socket->inited) {
    passToVM(pVM, 0, "error", "TcpClient not initialized!");
    return;
  }
  if (wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
    passToVM(pVM, 0, "error", "Pass a string");
    return;
  }

  const char *str = wrenGetSlotString(pVM, 1);
  if (strlen(str) <= 0) {
    passToVM(pVM, 0, "error", "Pass at least character");
    return;
  }
  try {
    socket->value.sendString(str);
    wrenSetSlotString(pVM, 0, "ok");
  } catch (std::exception &e) {
    passToVM(pVM, 0, "error", e.what());
  }
}

static void tcpClientRecvString(WrenVM *pVM) {
  auto *socket = (TcpClientStorage *)wrenGetSlotForeign(pVM, 0);
  if (!socket->inited) {
    passToVM(pVM, 0, "error", "TcpClient not initialized!");
    return;
  }
  try {
    auto bytes = socket->value.recvString();
    passToVM(pVM, 0, "ok", bytes.c_str());
  } catch (std::exception &e) {
    passToVM(pVM, 0, "error", e.what());
  }
}

static void tcpClientClose(WrenVM *pVM) {
  auto *socket = (TcpClientStorage *)wrenGetSlotForeign(pVM, 0);
  if (!socket->inited) {
    passToVM(pVM, 0, "error", "TcpClient not initialized!");
    return;
  }
  try {
    socket->value.~TcpClient();
    socket->inited = false;
  } catch (std::exception &e) {
    passToVM(pVM, 0, "error", e.what());
  }
}

static void hsvToRgb(WrenVM *pVM) {
  auto *self = (Script *)wrenGetUserData(pVM);
  if (wrenGetSlotType(pVM, 1) != WREN_TYPE_NUM ||
      wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM ||
      wrenGetSlotType(pVM, 3) != WREN_TYPE_NUM) {
    wrenSetSlotNull(pVM, 0);
    self->log(LEVEL_ERROR, "ScriptLibs: hsvToRgb invalid parameters");
    return;
  }
  auto h = wrenGetSlotDouble(pVM, 1);
  auto s = wrenGetSlotDouble(pVM, 2);
  auto v = wrenGetSlotDouble(pVM, 3);
  double p, q, t, ff;
  long i;
  double r, g, b;
  while (h >= 360.0)
    h -= 360.0;
  while (h < 0)
    h += 360.0;
  h /= 60.0;
  i = (long)h;
  ff = h - i;
  p = v * (1.0 - s);
  q = v * (1.0 - (s * ff));
  t = v * (1.0 - (s * (1.0 - ff)));

  switch (i) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  case 5:
  default:
    r = v;
    g = p;
    b = q;
    break;
  }
  wrenSetSlotNewList(pVM, 0);
  wrenSetSlotDouble(pVM, 1, r * 255);
  wrenInsertInList(pVM, 0, 0, 1);
  wrenSetSlotDouble(pVM, 1, g * 255);
  wrenInsertInList(pVM, 0, 1, 1);
  wrenSetSlotDouble(pVM, 1, b * 255);
  wrenInsertInList(pVM, 0, 2, 1);
}

static void exportImage(WrenVM *pVM) {
  if (wrenGetSlotType(pVM, 1) != WREN_TYPE_LIST ||
      wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM ||
      wrenGetSlotType(pVM, 3) != WREN_TYPE_NUM) {
    wrenSetSlotNull(pVM, 0);
    return;
  }
  auto width = (int)wrenGetSlotDouble(pVM, 2);
  auto height = (int)wrenGetSlotDouble(pVM, 3);
  if (width > 10000 || height > 10000 || width <= 0 || height <= 0) {
    wrenSetSlotNull(pVM, 0);
    return;
  }
  auto clamp = [](auto pNum) {
    if (pNum > 255)
      return 255.0;
    else if (pNum < 0)
      return 0.0;
    return pNum;
  };

  wrenEnsureSlots(pVM, 5);
  auto *data = (uint8_t *)malloc(width * height * 3);
  for (size_t i = 0; i < static_cast<size_t>(width * height); ++i) {
    wrenGetListElement(pVM, 1, i, 2);
    wrenGetListElement(pVM, 2, 0, 3);
    wrenGetListElement(pVM, 2, 1, 4);
    wrenGetListElement(pVM, 2, 2, 5);
    data[i * 3 + 0] = (uint8_t)clamp(wrenGetSlotDouble(pVM, 3));
    data[i * 3 + 1] = (uint8_t)clamp(wrenGetSlotDouble(pVM, 4));
    data[i * 3 + 2] = (uint8_t)clamp(wrenGetSlotDouble(pVM, 5));
  }
  int imageLen;
  auto image =
      stbi_write_png_to_mem(data, width * 3, width, height, 3, &imageLen);
  free(data);
  /*auto file = fopen("/tmp/test", "wb");
  fwrite(image, 1, imageLen, file);
  fclose(file);*/
  assert(image);
  wrenSetSlotBytes(pVM, 0, reinterpret_cast<const char *>(image), imageLen);
  free(image);
}

static void genRandomDouble(WrenVM *pVM) {
  wrenSetSlotDouble(pVM, 0, (rand() % 1'000'000) / 1'000'000.0);
}

static void getConfigString(WrenVM *pVM) {
  char *data = readWholeFileCString(getConfigFileLocation(), false);
  if (data) {
    wrenSetSlotString(pVM, 0, data);
    free(data);
  } else {
    wrenSetSlotNull(pVM, 0);
  }
}

static void httpClientSend(WrenVM *pVM) {
  if (wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING ||
      wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM ||
      wrenGetSlotType(pVM, 3) != WREN_TYPE_STRING ||
      wrenGetSlotType(pVM, 4) != WREN_TYPE_STRING ||
      wrenGetSlotType(pVM, 5) != WREN_TYPE_STRING) {
    wrenSetSlotString(pVM, 0, "Invalid parameters types");
    wrenAbortFiber(pVM, 0);
  }
  auto hostname = wrenGetSlotString(pVM, 1);
  auto port = (int64_t)wrenGetSlotDouble(pVM, 2);
  auto url = wrenGetSlotString(pVM, 3);
  std::string type = wrenGetSlotString(pVM, 4);
  auto body = wrenGetSlotString(pVM, 5);

  try {
    std::unique_ptr<httplib::Client> httpClient;
    if (port == 443)
      httpClient.reset(new httplib::SSLClient(hostname, port));
    else
      httpClient.reset(new httplib::Client(hostname, port));
    std::shared_ptr<httplib::Response> resp;
    if (type == "GET") {
      resp = httpClient->Get(url);
    } else if (type == "POST") {
      resp = httpClient->Post(url, body, "application/x-www-form-urlencoded");
    } else {
      throw std::runtime_error("Unknown request type");
    }
    if (!resp) {
      throw std::runtime_error("Unable to make request");
    }
    wrenEnsureSlots(pVM, 2);
    wrenSetSlotNewList(pVM, 0);
    wrenSetSlotDouble(pVM, 1, resp->status);
    wrenInsertInList(pVM, 0, 0, 1);
    wrenSetSlotString(pVM, 1, resp->body.c_str());
    wrenInsertInList(pVM, 0, 1, 1);
  } catch (std::exception &e) {
    wrenSetSlotString(pVM, 0, e.what());
    wrenAbortFiber(pVM, 0);
  }
}

static void stringsJoin(WrenVM* pVM) {
  if(wrenGetSlotType(pVM, 1) != WREN_TYPE_LIST || wrenGetSlotType(pVM, 2) != WREN_TYPE_STRING) {
    wrenSetSlotString(pVM, 0, "Please pass an array of strings and a string");
    wrenAbortFiber(pVM, 0);
    return;
  }
  int seperatorLength;
  const char* seperator = wrenGetSlotBytes(pVM, 2, &seperatorLength);

  const auto elementCount = wrenGetListCount(pVM, 1);
  size_t totalLength = 0;
  for(int i = 0; i < elementCount; ++i) {
    wrenGetListElement(pVM, 1, i, 2);
    int length;
    wrenGetSlotBytes(pVM, 2, &length);
    totalLength += length;
  }
  size_t allocedBytes = totalLength + seperatorLength * (elementCount - 1);
  char* returnString = (char*)malloc(allocedBytes);
  size_t written = 0;
  for(int i = 0; i < elementCount; ++i) {
    wrenGetListElement(pVM, 1, i, 2);
    int length;
    auto bytes = wrenGetSlotBytes(pVM, 2, &length);
    memcpy(returnString + written, bytes, length);
    written += length;
    if(i != elementCount - 1) {
      memcpy(returnString + written, seperator, seperatorLength);
      written += seperatorLength;
    }
  }
  wrenSetSlotBytes(pVM, 0, returnString, allocedBytes);
  free(returnString);
}

WrenForeignMethodFn bindForeignMethod(WrenVM *vm, const char *module,
                                      const char *className, bool isStatic,
                                      const char *signature) {
  (void)vm;
  (void)module;
  if (strcmp(className, "Database") == 0) {
    if (!isStatic && strcmp(signature, "queryInternal(_)") == 0) {
      return databaseQuerySync;
    } else if (strcmp(signature, "init new()") == 0) {
      return databaseCreate;
    } else if (strcmp(signature, "init new(_,_,_,_,_)") == 0) {
      return databaseCreateWithParams;
    }
  } else if (strcmp(className, "TcpClient") == 0 && !isStatic) {
    if (strcmp("isConnected()", signature) == 0) {
      return tcpClientIsConnected;
    } else if (strcmp("sendByte(_)", signature) == 0) {
      return tcpClientSendByte;
    } else if (strcmp("recvByte()", signature) == 0) {
      return tcpClientRecvByte;
    } else if (strcmp("sendString(_)", signature) == 0) {
      return tcpClientSendString;
    } else if (strcmp("recvString()", signature) == 0) {
      return tcpClientRecvString;
    } else if (strcmp("close()", signature) == 0) {
      return tcpClientClose;
    }
  } else if (strcmp(className, "Image") == 0 && !isStatic) {
    if (strcmp("exportInternal(_,_,_)", signature) == 0) {
      return exportImage;
    }
  } else if (strcmp(className, "Image") == 0 && isStatic) {
    if (strcmp("hsvToRgb(_,_,_)", signature) == 0) {
      return hsvToRgb;
    }
  } else if (strcmp(className, "Math") == 0 && isStatic) {
    if (strcmp("random()", signature) == 0) {
      return genRandomDouble;
    }
  } else if (strcmp(className, "Config") == 0 && isStatic) {
    if (strcmp("getConfigStringInternal()", signature) == 0) {
      return getConfigString;
    }
  } else if (strcmp(className, "HttpClient") == 0 && isStatic) {
    if (strcmp("sendInternal(_,_,_,_,_)", signature) == 0) {
      return httpClientSend;
    }
  } else if (strcmp(className, "Util") == 0 && isStatic) {
    if (strcmp("join(_,_)", signature) == 0) {
      return stringsJoin;
    }
  }
  std::cout << "Unknown class " << className << " and signature " << signature
            << "\n";
  assert(false);
}