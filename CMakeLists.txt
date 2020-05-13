cmake_minimum_required(VERSION 2.8.2)
project(ZeVa2_0)
set (CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories("${PROJECT_SOURCE_DIR}/wren/src/include")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/module")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/cli")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/optional")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/vm")
include_directories("${PROJECT_SOURCE_DIR}/include")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse3 -fno-strict-aliasing")

file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE WREN_SOURCES 
  "${PROJECT_SOURCE_DIR}/wren/src/vm/*.c"
  "${PROJECT_SOURCE_DIR}/wren/src/optional/*.c")

#list(REMOVE_ITEM ADD_ON_SOURCES "${PROJECT_SOURCE_DIR}/angelscript/add_on/autowrapper/generator/generateheader.cpp")

add_definitions(-D_LINUX_)

add_executable(ZeVa2_0 ${SOURCES} ${HEADERS} ${WREN_SOURCES})

target_link_libraries(ZeVa2_0 pthread)