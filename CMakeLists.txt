cmake_minimum_required(VERSION 3.12)

# Project Configuration
project(BloodFarm VERSION 1.0.0 LANGUAGES CXX C)

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
add_library(FastNoiseSIMD OBJECT
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
    deps/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
)

set(PHYSICSFS_SOURCES
    deps/physfs/src/physfs.c
    deps/physfs/src/physfs_archiver_7z.c
    deps/physfs/src/physfs_archiver_dir.c
    deps/physfs/src/physfs_archiver_grp.c
    deps/physfs/src/physfs_archiver_hog.c
    deps/physfs/src/physfs_archiver_iso9660.c
    deps/physfs/src/physfs_archiver_mvl.c
    deps/physfs/src/physfs_archiver_qpak.c
    deps/physfs/src/physfs_archiver_slb.c
    deps/physfs/src/physfs_archiver_unpacked.c
    deps/physfs/src/physfs_archiver_vdf.c
    deps/physfs/src/physfs_archiver_wad.c
    deps/physfs/src/physfs_archiver_zip.c
    deps/physfs/src/physfs_byteorder.c
    deps/physfs/src/physfs_miniz.h
    deps/physfs/src/physfs_platform_apple.m
    deps/physfs/src/physfs_platform_haiku.cpp
    deps/physfs/src/physfs_platform_os2.c
    deps/physfs/src/physfs_platform_posix.c
    deps/physfs/src/physfs_platform_qnx.c
    deps/physfs/src/physfs_platform_unix.c
    deps/physfs/src/physfs_platform_windows.c
    deps/physfs/src/physfs_platform_winrt.cpp
    deps/physfs/src/physfs_unicode.c
)

# Sources
set(SOURCES
    ${PHYSICSFS_SOURCES}
    src/main.cpp
    src/graphics/shader.cpp
    src/graphics/textures.cpp
    src/graphics/imagesets.cpp
    src/graphics/spritepool.cpp
    src/util/logging.cpp
    src/util/helpers.cpp
    src/services/core/resources.cpp
    src/services/core/physics.cpp
)

add_executable(BloodFarm ${SOURCES} $<TARGET_OBJECTS:FastNoiseSIMD>)

# Packaged dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

find_package(SDL2 2.0.9)
if (SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIR})
    target_link_libraries(BloodFarm ${SDL2_LIBRARY})
endif()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries(BloodFarm ${GLEW_LIBRARIES})
endif()
find_package(BULLET REQUIRED)
if (BULLET_FOUND)
    include_directories(${BULLET_INCLUDE_DIRS})
    target_link_libraries(BloodFarm ${BULLET_LIBRARIES})
endif()
# Included dependencies

add_subdirectory(deps/cpptoml)
target_include_directories (cpptoml INTERFACE $<BUILD_INTERFACE:${CPPTOML_SOURCE_DIR}/include>)
target_link_libraries(BloodFarm cpptoml)

target_include_directories(BloodFarm
    PRIVATE 
        # Project include files
        ${PROJECT_SOURCE_DIR}/include
        # Source dependencies' includes
        ${PROJECT_SOURCE_DIR}/deps/FastNoiseSIMD/FastNoiseSIMD
        ${PROJECT_SOURCE_DIR}/deps/physfs/src # Includes the header file...

        # Include header-only dependencies
        ${PROJECT_SOURCE_DIR}/deps/stb
        ${PROJECT_SOURCE_DIR}/deps/spdlog/include
        ${PROJECT_SOURCE_DIR}/deps/EnTT/src
        ${PROJECT_SOURCE_DIR}/deps/glm
        ${PROJECT_SOURCE_DIR}/deps/semimap
        ${PROJECT_SOURCE_DIR}/deps/physfs-hpp/include 
        ${PROJECT_SOURCE_DIR}/deps/cxxopts/include
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
target_compile_definitions(BloodFarm PUBLIC $<$<CONFIG:DEBUG>:DEBUG_BUILD>)
target_compile_definitions(BloodFarm PUBLIC $<$<CONFIG:DEBUG>:SPDLOG_DEBUG_ON>)
target_compile_definitions(BloodFarm PUBLIC $<$<CONFIG:DEBUG>:SPDLOG_TRACE_ON>)


set(SETTINGS_ORGANIZATION "Dan Kersten")
set(SETTINGS_APPLICATION ${PROJECT_NAME})
set(PROJECT_AUTHOR "Dan Kersten")
set(PROJECT_AUTHOR_EMAIL "projects@dan.kersten.me")

message("*")
message("* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message("* Copyright (c) 2018 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")
message("* Building with ${CMAKE_CXX_COMPILER_ID}")
message("*")

option(USE_ASAN "Use address sanitizer by adding -fsanitize=address -fno-omit-frame-pointer flags" OFF)

if(USE_ASAN)
    target_compile_options(BloodFarm PUBLIC $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fsanitize=address -fno-omit-frame-pointer>)
    target_link_libraries(BloodFarm PUBLIC $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fsanitize=address -fno-omit-frame-pointer>)
endif()

target_compile_options(
    BloodFarm
    PUBLIC $<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:USING_MSVC>>>:-O0 -g>
    # it seems that -O3 ruins a bit the performance when using clang ...
    PUBLIC $<$<AND:$<CONFIG:Release>,$<BOOL:USING_CLANG>>:-O2 -flto=thin>
    # ... on the other side, GCC is incredibly comfortable with it.
    PUBLIC $<$<AND:$<CONFIG:Release>,$<BOOL:USING_GCC>>:-O3>
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
if(APPLE)
    # Foundation and IOKit are needed by PhysicsFS on OSX
    target_link_libraries(BloodFarm
        "-framework Foundation"
        "-framework IOKit"
        "-framework OpenGL"
    )
endif()

#target_compile_options(BloodFarm PUBLIC -Werror)
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
