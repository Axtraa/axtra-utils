cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(RemoveEffect VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

# Bindings come from Geode's own CPM clone of geode-sdk/bindings@main.
# Do not try to pin them by fetching a bare SHA: GitHub rejects fetches of
# unadvertised objects, so such a step silently falls back to the default.
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Upstream iOS codegen bug: the FMOD free functions are declared with `ios 0x...`
# addresses in FMOD.bro, so codegen emits their definitions into
# GeneratedSource.cpp — but that file never includes the FMOD headers, and the
# build dies on "unknown type name 'FMOD_RESULT'". Force-include them for the
# bindings target (no-op on the platforms that do not generate that code).
if (("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS) AND TARGET GeodeBindings)
    target_compile_options(GeodeBindings PRIVATE "-include" "fmod.hpp")
endif()

setup_geode_mod(${PROJECT_NAME})
