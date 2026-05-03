# Copyright 2026 m4jr0. All Rights Reserved.
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.21)

################################################################################
#
# Kettle
#
################################################################################

project(kettle
  VERSION 0.1.0
  DESCRIPTION "Tiny blueprint-style graph runtime and VM"
  LANGUAGES CXX
)

# CMake setup ##################################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Names ########################################################################

set(KETTLE_LIBRARY_NAME "kettle")
set(KETTLE_CLI_NAME "kettle_cli")
set(MESSAGE_PREFIX "[Kettle] ")

message(STATUS "${MESSAGE_PREFIX}Build type: ${CMAKE_BUILD_TYPE}")

# Options ######################################################################

option(KETTLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(KETTLE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(KETTLE_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Multi-config output ###########################################################

if(CMAKE_CONFIGURATION_TYPES)
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
  endforeach()
endif()

# Library ######################################################################

option(KETTLE_STATIC_MSVC_RUNTIME "Link MSVC runtime statically" ON)

if(MSVC AND KETTLE_STATIC_MSVC_RUNTIME)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(${KETTLE_LIBRARY_NAME}
  src/kettle/bytecode.cpp
  src/kettle/compiler.cpp
  src/kettle/format.h
  src/kettle/graph_view.cpp
  src/kettle/hash.h
  src/kettle/natives.cpp
  src/kettle/nodes.cpp
  src/kettle/value_utils.h
)

target_include_directories(${KETTLE_LIBRARY_NAME}
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(${KETTLE_LIBRARY_NAME}
  PUBLIC
    KETTLE_NAME="${PROJECT_NAME}"
    KETTLE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    KETTLE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    KETTLE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Executable ###################################################################

add_executable(${KETTLE_CLI_NAME}
  src/kettle_cli/main.cpp
)

target_link_libraries(${KETTLE_CLI_NAME}
  PRIVATE
    ${KETTLE_LIBRARY_NAME}
)

# Compiler settings ############################################################

if(MSVC)
  target_compile_options(${KETTLE_LIBRARY_NAME}
    PRIVATE
      /permissive-
      /Zc:preprocessor
      /W4
      /MP
  )

  target_compile_options(${KETTLE_CLI_NAME}
    PRIVATE
      /permissive-
      /Zc:preprocessor
      /W4
      /MP
  )

  if(KETTLE_WARNINGS_AS_ERRORS)
    target_compile_options(${KETTLE_LIBRARY_NAME} PRIVATE /WX)
    target_compile_options(${KETTLE_CLI_NAME} PRIVATE /WX)
  endif()

  if(KETTLE_ENABLE_ASAN)
    target_compile_options(${KETTLE_LIBRARY_NAME} PRIVATE /fsanitize=address)
    target_compile_options(${KETTLE_CLI_NAME} PRIVATE /fsanitize=address)
    target_link_options(${KETTLE_CLI_NAME} PRIVATE /fsanitize=address)
  endif()

  if(KETTLE_ENABLE_TSAN)
    message(FATAL_ERROR "${MESSAGE_PREFIX}TSan is not supported with MSVC here.")
  endif()
else()
  target_compile_options(${KETTLE_LIBRARY_NAME}
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
  )

  target_compile_options(${KETTLE_CLI_NAME}
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
  )

  if(KETTLE_WARNINGS_AS_ERRORS)
    target_compile_options(${KETTLE_LIBRARY_NAME} PRIVATE -Werror)
    target_compile_options(${KETTLE_CLI_NAME} PRIVATE -Werror)
  endif()

  if(KETTLE_ENABLE_ASAN)
    target_compile_options(${KETTLE_LIBRARY_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_compile_options(${KETTLE_CLI_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${KETTLE_CLI_NAME} PRIVATE -fsanitize=address)
  endif()

  if(KETTLE_ENABLE_TSAN)
    target_compile_options(${KETTLE_LIBRARY_NAME} PRIVATE -fsanitize=thread)
    target_compile_options(${KETTLE_CLI_NAME} PRIVATE -fsanitize=thread)
    target_link_options(${KETTLE_CLI_NAME} PRIVATE -fsanitize=thread)
  endif()
endif()

message(STATUS "${MESSAGE_PREFIX}Done.")