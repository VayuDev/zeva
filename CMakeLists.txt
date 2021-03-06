cmake_minimum_required(VERSION 2.8.2)
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(ZeVa2_0)
set (CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory("drogon")
include_directories("${PROJECT_SOURCE_DIR}/drogon/lib/inc")


include_directories("${PROJECT_SOURCE_DIR}/controllers")
include_directories("${PROJECT_SOURCE_DIR}/include_thirdparty")

include_directories("${PROJECT_SOURCE_DIR}/wren/src/include")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/module")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/cli")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/optional")
include_directories("${PROJECT_SOURCE_DIR}/wren/src/vm")
include_directories("${PROJECT_SOURCE_DIR}/include")


set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -mtune=native -fno-strict-aliasing")

file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/controllers/*.cpp")
add_definitions(-D_LINUX_)

find_package(PkgConfig REQUIRED)

find_package(PostgreSQL REQUIRED)

pkg_check_modules(PQ REQUIRED libpq)
include_directories(${PostgreSQL_INCLUDE_DIRS})

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})

pkg_check_modules(SSH2 REQUIRED libssh2)
include_directories(${SSH2_INCLUDE_DIRS})

add_executable(ZeVa2_0 ${SOURCES})

target_link_libraries(ZeVa2_0 pthread zstd drogon m stdc++ pq stdc++fs ${GSTREAMER_LIBRARIES} ${SSH2_LIBRARIES})

add_subdirectory(${PROJECT_SOURCE_DIR}/scriptrunner)