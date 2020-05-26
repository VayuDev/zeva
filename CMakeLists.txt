cmake_minimum_required(VERSION 2.8.2)
project(ZeVa2_0)
set (CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)
include_directories(${PROJECT_SOURCE_DIR}/json/include)

add_subdirectory("drogon")
include_directories("${PROJECT_SOURCE_DIR}/drogon/lib/inc")


include_directories("${PROJECT_SOURCE_DIR}/controllers")

include_directories("${PROJECT_SOURCE_DIR}/wren/src/include")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/module")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/cli")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/optional")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/vm")
include_directories("${PROJECT_SOURCE_DIR}/include")


set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse3 -fno-strict-aliasing")

file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/controllers/*.cpp")
file(GLOB_RECURSE WREN_SOURCES 
  "${PROJECT_SOURCE_DIR}/wren/src/vm/*.c"
  "${PROJECT_SOURCE_DIR}/wren/src/optional/*.c")

add_definitions(-D_LINUX_)

find_package(PkgConfig REQUIRED)
pkg_check_modules(pqxx libpq REQUIRED)

add_executable(ZeVa2_0 ${SOURCES} ${WREN_SOURCES})

target_link_libraries(ZeVa2_0 pthread zstd drogon nlohmann_json::nlohmann_json pqxx pq)
