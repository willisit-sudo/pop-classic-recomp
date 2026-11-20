# pop_classic - ReXGlue Recompiled Project
#
# The SDK package provides rexglue_project and the rexglue_setup_* functions.

cmake_minimum_required(VERSION 3.25)
project(pop_classic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (REXSDK_DIR)
  add_subdirectory("${REXSDK_DIR}" rexglue-sdk)
else ()
  find_package(rexglue 0.11.0 REQUIRED CONFIG)
endif ()
rexglue_project(pop_classic OUTPUT_DIR generated/default)

# Host sources. src/main.cpp stays on the executable.
set(POPCLASSIC_HOST_SOURCES src/pop_classic_hooks.cpp)

# OBJECT and never STATIC, an archive drops the REX_EXPORT symbols.
add_library(pop_classic_common OBJECT ${POPCLASSIC_HOST_SOURCES})
rexglue_setup_host_library(pop_classic_common)

if (WIN32)
  add_executable(pop_classic WIN32 src/main.cpp)
else ()
  add_executable(pop_classic src/main.cpp)
endif ()

rexglue_setup_target(pop_classic)
target_link_libraries(pop_classic PRIVATE pop_classic_common)
