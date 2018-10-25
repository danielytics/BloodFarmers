cmake_minimum_required(VERSION 3.12)

# Project Configuration
project(BloodFarm VERSION 1.0.0 LANGUAGES CXX)

# Detect compiler
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  # using Clang
  set(USING_CLANG 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(USING_GCC 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  set(USING_ICC 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  set(USING_MSVC 1)
endif()

# Dependencies that get included into projects sources directly
set(DEPENDENCY_INCLUDED_SOURCES

)

add_library(FastNoiseSIMD OBJECT
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
)

# Sources
set(SOURCES
    ${DEPENDENCY_INCLUDED_SOURCES}
    src/main.cpp
    src/shader.cpp
    src/logging.cpp
)

add_executable(BloodFarm ${SOURCES} $<TARGET_OBJECTS:FastNoiseSIMD>)

# Package dependencies

find_package (SDL2)
if (SDL2_FOUND)
  include_directories(${SDL2_INCLUDE_DIRS})
  target_link_libraries(BloodFarm ${SDL2_LIBRARIES})
endif()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries(BloodFarm ${GLEW_LIBRARIES})
endif()

add_subdirectory(deps/spdlog)
target_include_directories (spdlog INTERFACE $<BUILD_INTERFACE:${SPDLOG_SOURCE_DIR}/include>)
target_link_libraries(BloodFarm spdlog)


# Included dependencies


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(SETTINGS_ORGANIZATION "Dan Kersten")
set(SETTINGS_APPLICATION ${PROJECT_NAME})
set(PROJECT_AUTHOR "Dan Kersten")
set(PROJECT_AUTHOR_EMAIL "projects@dan.kersten.me")

message("*")
message("* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message("* Copyright (c) 2018 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")
message("*")

option(USE_ASAN "Use address sanitizer by adding -fsanitize=address -fno-omit-frame-pointer flags" OFF)

target_include_directories(BloodFarm
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/deps/FastNoiseSIMD/FastNoiseSIMD
        ${PROJECT_SOURCE_DIR}/deps/glm-0.9.7.4/include
        ${PROJECT_SOURCE_DIR}/deps/cpptoml/include
)

if(USE_ASAN)
    target_compile_options(BloodFarm PUBLIC $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fsanitize=address -fno-omit-frame-pointer>)
    target_link_libraries(BloodFarm PUBLIC $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fsanitize=address -fno-omit-frame-pointer>)
endif()

target_compile_options(
    BloodFarm
    PUBLIC $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O0 -g>
    # it seems that -O3 ruins a bit the performance when using clang ...
    PUBLIC $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:Clang>>:-O2>
    # ... on the other side, GCC is incredibly comfortable with it.
    PUBLIC $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU>>:-O3>
)

# Platform specific compile options
if(USING_MSVC)
    target_compile_options(BloodFarm PUBLIC /arch:SSE4.1 /fp:fast)
    target_compile_options(FastNoiseSIMD PUBLIC /arch:SSE4.1 /arch:AVX2 /fp:fast)
else()
    target_compile_options(BloodFarm PUBLIC -msse4.1 -mfma)
    target_compile_options(FastNoiseSIMD PUBLIC -msse4.1 -mavx2 -mfma)
    if(USING_CLANG)
       target_compile_options(BloodFarm PUBLIC -ffp-contract=fast)
       target_compile_options(FastNoiseSIMD PUBLIC -ffp-contract=fast)
    endif()
endif()

target_compile_options(BloodFarm PUBLIC -Werror)
target_compile_features(BloodFarm PRIVATE cxx_std_17)

target_compile_features(FastNoiseSIMD PRIVATE cxx_std_11)

# Place compiled binary into project root directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

if (NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS OFF CACHE BOOL "Build Tests ?")
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()