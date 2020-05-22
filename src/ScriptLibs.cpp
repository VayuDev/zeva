#include "ScriptLibs.hpp"
#include <cstring>
#include <stdexcept>
#include "Logger.hpp"
#include "PostgreSQLDatabase.hpp"
#include <cassert>
#include <TcpClient.hpp>
#include "Util.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

WrenForeignClassMethods bindForeignClass(WrenVM*, const char* module, const char* classname) {
    (void) module;
    if(strcmp(classname, "Database") == 0) {
        WrenForeignClassMethods methods = {
            .allocate = [] (WrenVM* pVM) {
                if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
                    wrenSetSlotNull(pVM, 0);
                    return;
                }
                void *data = wrenSetSlotNewForeign(pVM, 0, 0, sizeof(PostgreSQLDatabase));
                const char *dbname = wrenGetSlotString(pVM, 1);
                if(wrenGetSlotType(pVM, 2) == WREN_TYPE_STRING) {
                    new(data)PostgreSQLDatabase(dbname,
                            wrenGetSlotString(pVM, 2),
                            wrenGetSlotString(pVM, 3),
                            wrenGetSlotString(pVM, 4),
                            wrenGetSlotDouble(pVM, 5));
                } else {
                    new(data)PostgreSQLDatabase(dbname);
                }

            },
            .finalize = [] (void *data) {
                auto* db = (PostgreSQLDatabase*)data;
                db->~PostgreSQLDatabase();
            },
        };
        return methods;
    } else if(strcmp(classname, "TcpClient") == 0) {
        WrenForeignClassMethods methods = {
                .allocate = [] (WrenVM* pVM) {
                    wrenEnsureSlots(pVM, 3);
                    if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING && wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM) {
                        wrenSetSlotNull(pVM, 0);
                        return;
                    }
                    auto hostname = wrenGetSlotString(pVM, 1);
                    auto port = wrenGetSlotDouble(pVM, 2);
                    void *data = wrenSetSlotNewForeign(pVM, 0, 0, sizeof(TcpClient));
                    if(port <= 0 || port > std::numeric_limits<uint16_t>::max()) {
                        wrenSetSlotNull(pVM, 0);
                        return;
                    }
                    try {
                        new(data)TcpClient(hostname, port);
                    } catch(std::exception& e) {
                        log().error("Failed to construct a TcpClient: %s", e.what());
                        wrenSetSlotNull(pVM, 0);
                    }
                },
                .finalize = [] (void *data) {
                    auto* socket = (TcpClient*)data;
                    socket->~TcpClient();
                },
        };
        return methods;
    }
    WrenForeignClassMethods methods = {.allocate = nullptr, .finalize = nullptr};
    return methods;
}

static void databaseQuerySync(WrenVM *pVM) {
    if(wrenGetSlotType(pVM, 1) == WREN_TYPE_STRING) {
        auto* db = (PostgreSQLDatabase*) wrenGetSlotForeign(pVM, 0);
        const char *text = wrenGetSlotString(pVM, 1);
        wrenEnsureSlots(pVM, 2);
        try {
            auto ret = db->query(text);
            //LOAD VALUES
            wrenSetSlotString(pVM, 1, "ok");
            wrenEnsureSlots(pVM, 4);
            wrenSetSlotNewList(pVM, 2);
            for(size_t r = 0; r < ret->getRowCount(); ++r) {
                wrenSetSlotNewList(pVM, 3);
                for(size_t c = 0; c < ret->getColumnCount(); ++c) {
                    const auto& sv = ret->getValue(r, c);
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
                            wrenSetSlotDouble(pVM, 4, sv.timeValue.tv_sec + sv.timeValue.tv_usec / 1'000'000.0);
                            break;
                        default:
                            assert(false);
                    }
                    wrenInsertInList(pVM, 3, c, 4);
                }
                wrenInsertInList(pVM, 2, r, 3);

            }
        } catch (std::exception& e) {
            wrenSetSlotString(pVM, 1, "error");
            wrenSetSlotString(pVM, 2, e.what());
        }
        wrenSetSlotNewList(pVM, 0);
        wrenInsertInList(pVM, 0, 0, 1);
        wrenInsertInList(pVM, 0, 1, 2);
    } else {
        wrenSetSlotNull(pVM, 0);
    }
}

static void passToVM(WrenVM* pVM, int pSlot, const char* pType, const char *pMsg) {
    wrenEnsureSlots(pVM, pSlot + 3);
    wrenSetSlotNewList(pVM, 0);
    wrenSetSlotString(pVM, 1, pType);
    wrenSetSlotString(pVM, 2, pMsg);
    wrenInsertInList(pVM, 0, 0, 1);
    wrenInsertInList(pVM, 0, 1, 2);
}

static void tcpClientIsConnected(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    wrenSetSlotBool(pVM, 0, socket->isConnected());
}

static void tcpClientSendByte(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
        passToVM(pVM, 0, "error", "Pass a string");
        return;
    }
    int length;
    const auto* bytesPointer = (const uint8_t*) wrenGetSlotBytes(pVM, 1, &length);
    if(length != 1) {
        passToVM(pVM, 0, "error", "Pass only one character");
        return;
    }
    uint8_t byte = *bytesPointer;
    try {
        socket->sendByte(byte);
        wrenSetSlotString(pVM, 0, "ok");
    } catch(std::exception& e) {
        passToVM(pVM, 0, "error", e.what());
    }
}

static void tcpClientRecvByte(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    try {
        auto byte = socket->recvByte();
        std::string byteString{static_cast<char>(byte), 1};
        passToVM(pVM, 0, "ok", byteString.c_str());
    } catch(std::exception& e) {
        passToVM(pVM, 0, "error", e.what());
    }
}

static void tcpClientSendString(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    if(wrenGetSlotType(pVM, 1) != WREN_TYPE_STRING) {
        passToVM(pVM, 0, "error", "Pass a string");
        return;
    }

    const char* str = wrenGetSlotString(pVM, 1);
    if(strlen(str) <= 0) {
        passToVM(pVM, 0, "error", "Pass at least character");
        return;
    }
    try {
        socket->sendString(str);
        wrenSetSlotString(pVM, 0, "ok");
    } catch(std::exception& e) {
        passToVM(pVM, 0, "error", e.what());
    }
}

static void tcpClientRecvString(WrenVM* pVM) {
    auto* socket = (TcpClient*) wrenGetSlotForeign(pVM, 0);
    try {
        auto bytes = socket->recvString();
        passToVM(pVM, 0, "ok", bytes.c_str());
    } catch(std::exception& e) {
        passToVM(pVM, 0, "error", e.what());
    }
}

static void exportImage(WrenVM* pVM) {
    if(wrenGetSlotType(pVM, 1) != WREN_TYPE_LIST || wrenGetSlotType(pVM, 2) != WREN_TYPE_NUM || wrenGetSlotType(pVM, 3) != WREN_TYPE_NUM) {
        wrenSetSlotNull(pVM, 0);
        return;
    }
    auto width = (int)wrenGetSlotDouble(pVM, 2);
    auto height = (int)wrenGetSlotDouble(pVM, 3);
    if(width > 10000 || height > 10000 || width <= 0 || height <= 0) {
        wrenSetSlotNull(pVM, 0);
        return;
    }
    wrenEnsureSlots(pVM, 5);
    auto *data = (uint8_t*) malloc(width * height * 3);
    for(size_t i = 0; i < width * height; ++i) {
        wrenGetListElement(pVM, 1, i, 2);
        wrenGetListElement(pVM, 2, 0, 3);
        wrenGetListElement(pVM, 2, 1, 4);
        wrenGetListElement(pVM, 2, 2, 5);
        data[i*3 + 0] = (uint8_t) wrenGetSlotDouble(pVM, 3);
        data[i*3 + 1] = (uint8_t) wrenGetSlotDouble(pVM, 4);
        data[i*3 + 2] = (uint8_t) wrenGetSlotDouble(pVM, 5);
    }
    int imageLen;
    auto image = stbi_write_png_to_mem(data, width * 3, width, height, 3, &imageLen);
    free(data);
    /*auto file = fopen("/tmp/test", "wb");
    fwrite(image, 1, imageLen, file);
    fclose(file);*/
    assert(image);
    wrenSetSlotBytes(pVM, 0, reinterpret_cast<const char *>(image), imageLen);
    free(image);
}

WrenForeignMethodFn bindForeignMethod( 
    WrenVM* vm, 
    const char* module, 
    const char* className, 
    bool isStatic, 
    const char* signature) 
{
    (void)vm;
    (void)module;
    if(strcmp(className, "Database") == 0) {
        if(!isStatic && strcmp(signature, "query(_)") == 0) {
            return databaseQuerySync;
        }
    } else if(strcmp(className, "TcpClient") == 0 && !isStatic) {
        if(strcmp("isConnected()", signature) == 0) {
            return tcpClientIsConnected;
        } else if(strcmp("sendByte(_)", signature) == 0) {
            return tcpClientSendByte;
        } else if(strcmp("recvByte()", signature) == 0) {
            return tcpClientRecvByte;
        } else if(strcmp("sendString(_)", signature) == 0) {
            return tcpClientSendString;
        } else if(strcmp("recvString()", signature) == 0) {
            return tcpClientRecvString;
        }
    } else if(strcmp(className, "Image") == 0 && !isStatic) {
        if(strcmp("exportInternal(_,_,_)", signature) == 0) {
            return exportImage;
        }
    }
    assert(false);
}

const char *foreignClassesString() {
    return R"--(
foreign class Database {
    construct new(dbname) {}
    construct new(dbname, username, password, hostname, port) {}
    foreign query(request)
}

foreign class TcpClient {
    construct new(ip, port) {}
    foreign isConnected()
    foreign sendByte(byte)
    foreign recvByte()
    foreign sendString(str)
    foreign recvString()
}

class Image {
    construct new(width, height) {
        _width = width
        _height = height
        _pixels = List.filled(width * height, [0, 1, 1])
    }
    setPixel(x, y, color) {
        _pixels[y * _width + x] = color
    }
    foreign exportInternal(pixels, width, height)
    export() {
        return exportInternal(_pixels, _width, _height)
    }
}
)--";
}