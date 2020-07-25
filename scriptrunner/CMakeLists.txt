cmake_minimum_required(VERSION 2.8.2)
project(ZeVaScript)
set (CMAKE_CXX_STANDARD 20)

include_directories("${PROJECT_SOURCE_DIR}/../wren/src/include")
include_directories("${PROJECT_SOURCE_DIR}/../wren/src/module")
include_directories("${PROJECT_SOURCE_DIR}/../wren/src/cli")
include_directories("${PROJECT_SOURCE_DIR}/../wren/src/optional")
include_directories("${PROJECT_SOURCE_DIR}/../wren/src/vm")
include_directories("${PROJECT_SOURCE_DIR}/../include")
include_directories("${PROJECT_SOURCE_DIR}/../include_thirdparty")

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -mtune=native -fno-strict-aliasing")

file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src_thirdparty/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/Util.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/PostgreSQLDatabase.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/DatabaseWrapper.cpp")
file(GLOB_RECURSE WREN_SOURCES
        "${PROJECT_SOURCE_DIR}/../wren/src/vm/*.c"
        "${PROJECT_SOURCE_DIR}/../wren/src/optional/*.c")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../drogon/cmake_modules")
find_package(Jsoncpp REQUIRED)
include_directories(${JSONCPP_INCLUDE_DIRS})

add_executable(ZeVaScript ${SOURCES} ${WREN_SOURCES})

target_link_libraries(ZeVaScript pthread pq m stdc++ stdc++fs Jsoncpp_lib ssl crypto)

add_custom_command(TARGET ZeVaScript
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ZeVaScript> ${PROJECT_SOURCE_DIR}/../assets/)
