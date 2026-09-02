#  Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
# 
#  This software is provided 'as-is', without any express or implied
#  warranty. In no event will the authors be held liable for any damages
#  arising from the use of this software.
#
#  Permission is granted to anyone to use this software for any purpose,
#  including commercial applications, and to alter it and redistribute it
#  freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not
#  claim that you wrote the original software. If you use this software
#  in a product, an acknowledgment in the product documentation would be
#  appreciated but is not required.
#  2. Altered source versions must be plainly marked as such, and must not be
#  misrepresented as being the original software.
#  3. This notice may not be removed or altered from any source distribution.

cmake_minimum_required (VERSION 3.24)

project (sunlight VERSION 0.18.0)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FetchContent)

option(SUNLIGHT_DEBUG_SYMBOLS_UNIX "Build SunLight with debug symbols on Unixes (Mac and Linux)" ON)

#
# Setup default engine (DEFAULT_ENGINE preprocessor)
# 1 - RAYLIB
#
set(SUNLIGHT_DEFAULT_BACKEND_ENGINE 1)

#
# Setup default resource filesystem (DEFAULT_FILESYSTEM preprocessor)
# 1 - PHYSFS
#
set(SUNLIGHT_DEFAULT_BACKEND_FILESYSTEM 1)

if(NOT USE_LOCAL_LIBXML2)
    ####################################################################
    # LibXml2 (TMX dependency)
    ####################################################################
    if(APPLE)
        add_definitions(-DHAVE_SYS_RANDOM_H=1)
    endif()

    set(LIBXML2_WITH_PYTHON OFF)   # Turn off Python bind build dependency
    set(LIBXML2_WITH_TESTS OFF)    # Turn off tests build dependency
    set(LIBXML2_WITH_PROGRAMS OFF) # Turn off xmllint/xmlcatalog CLI tools - sunlight only needs the library

    FetchContent_Declare(libxml2 OVERRIDE_FIND_PACKAGE
                         GIT_REPOSITORY https://github.com/GNOME/libxml2.git
                         GIT_TAG v2.15.3)
    FetchContent_MakeAvailable(libxml2)
endif()

####################################################################
# TMX
####################################################################
FetchContent_Declare(tmx
                     GIT_REPOSITORY https://github.com/baylej/tmx.git
                     GIT_TAG tmx_1.10.1)
FetchContent_MakeAvailable(tmx)

####################################################################
# RayLib
####################################################################

# Comment the line below to compile raylib using newer OpenGL (eg. 3.3)
set(GRAPHICS "GRAPHICS_API_OPENGL_21")

FetchContent_Declare(raylib
                     GIT_REPOSITORY https://github.com/raysan5/raylib.git
                     GIT_TAG 5.5)
FetchContent_MakeAvailable(raylib)

####################################################################
# PhysFS (Phase 12 prototype - archive/loose-folder resource
# abstraction, mount-based so a real .zip and a loose directory read
# identically to client code)
####################################################################
set(PHYSFS_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# raylib's own CMakeLists.txt already defines a conventional "uninstall"
# custom target - PhysFS's own would collide with it verbatim
# (CMP0002, "target already exists") since both are pulled in via
# FetchContent into the same top-level build. PhysFS exposes this exact
# name as a cache variable specifically for this kind of collision -
# rename it rather than patching PhysFS's own CMakeLists.txt.
# PhysFS is fetched as a static lib (see PHYSFS_BUILD_STATIC above),
# but it links straight into libsunlight, itself a SHARED library - on
# Linux, GCC only makes ordinary static-lib object code
# position-independent when this is set (unlike a shared lib target,
# which is always PIC by default), so leaving it off produces a
# real, CI-confirmed link failure: "relocation R_X86_64_PC32 against
# symbol ... can not be used when making a shared object; recompile
# with -fPIC". Setting this before physfs's own targets are created
# (FetchContent_MakeAvailable adds them via add_subdirectory, which
# inherits this like any other directory-scoped variable) fixes it
# without needing to patch PhysFS's own CMakeLists.txt.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PHYSFS_TARGETNAME_UNINSTALL "physfs_uninstall" CACHE STRING "" FORCE)

FetchContent_Declare(physfs
                     GIT_REPOSITORY https://github.com/icculus/physfs.git
                     GIT_TAG release-3.2.0)
FetchContent_MakeAvailable(physfs)

# PhysFS's own CMakeLists.txt unconditionally forces CMAKE_SKIP_RPATH ON
# (as a CACHE variable, with FORCE) on Clang/GCC "to avoid using -rpath" -
# since CACHE variables are global to the whole build, not scoped to the
# subdirectory that set them, this silently disabled RPATH generation for
# EVERY target in this entire project once PhysFS was added (raylib/tmx/
# sunlight/scarab alike), not just PhysFS's own - confirmed live: every
# one of those executables failed to launch afterward with "Library not
# loaded: @rpath/...", "no LC_RPATH's found". Reset back to CMake's normal
# default (RPATH generation enabled) rather than patching PhysFS's own
# vendored CMakeLists.txt.
set(CMAKE_SKIP_RPATH OFF CACHE BOOL "" FORCE)

###################################################################
# Mandatory libraries
###################################################################

# Add all include files
include_directories("./")
include_directories("base")
include_directories("input")
include_directories("canvas")
include_directories("sprite")
include_directories("collision")
include_directories("renderer")
include_directories("tilemap")
include_directories("scripting")
include_directories("engines")
include_directories("sound")
include_directories("filesystem")
include_directories("concurrent")
include_directories("general")
include_directories("backends")

# Add all source files
file( GLOB_RECURSE LIBRARY_SOURCES "./" *.cpp )
file( GLOB LIBRARY_HEADERS "./" *.h )

file(GLOB_RECURSE REMOVE_CMAKE_FILES CMakeFiles/*)
file(GLOB_RECURSE REMOVE_BUILD_FILES build/*)
#list(REMOVE_ITEM LIBRARY_SOURCES ${REMOVE_BUILD_FILES} ${REMOVE_CMAKE_FILES})


if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

#
# C++17 - originally only set for APPLE (needed for some raylib defines
# there), but this project's own resource-filesystem code
# (raylibsound.cpp) now uses std::filesystem::path directly, which MSVC
# only enables at C++17 or later (confirmed live: without this,
# raylibsound.cpp fails to compile on Windows - "The contents of
# <filesystem> are available only with C++17 or later" - while GCC's
# own default standard on Linux happened to already be recent enough to
# not need this set explicitly, and Clang/APPLE already had it). Setting
# this unconditionally, not just under if(APPLE), closes that gap for
# every platform rather than relying on each compiler's own default.
#
set(CMAKE_CXX_STANDARD 17)

if(UNIX) # AND NOT APPLE)
    #
    # Debug symbol information on Unixes
    #
    if(SUNLIGHT_DEBUG_SYMBOLS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g2")
    endif()

    #
    # pthread support
    #
    include(FindThreads)
    
    if(THREADS_FOUND AND NOT TARGET Threads::Threads)
    	add_library(Threads::Threads INTERFACE IMPORTED)

    	if(THREADS_HAVE_PTHREAD_ARG)
      		set_property(TARGET Threads::Threads PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
    	endif()

    	if(CMAKE_THREAD_LIBS_INIT)
      		set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
    	endif()
  	endif()
endif()

add_definitions(-DDEFAULT_ENGINE=${SUNLIGHT_DEFAULT_BACKEND_ENGINE})
add_definitions(-DDEFAULT_FILESYSTEM=${SUNLIGHT_DEFAULT_BACKEND_FILESYSTEM})

add_library(sunlight ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

#
# ZLIB
# In fact ZLIB is not required for Windows compilation, but for LINUX it is, so
# we will add it for both compilations.
#
include(FindZLIB)
find_package(ZLIB REQUIRED)

#
# LibXml2
#
include(FindLibXml2)
find_package(LibXml2 REQUIRED)

target_link_libraries(sunlight ${CMAKE_THREAD_LIBS_INIT} raylib tmx LibXml2::LibXml2 ZLIB::ZLIB ${CMAKE_DL_LIBS} PhysFS::PhysFS-static)

# Installation

target_include_directories(sunlight INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}> $<INSTALL_INTERFACE:include>)

# CMake's default INSTALL_RPATH is empty, so a plain install(TARGETS) below
# bakes in NO rpath at all into the installed libsunlight/libtmx (raylib
# needs no fix - nothing it depends on lives in another target's own build
# dir) - confirmed empirically: `otool -l` on a real `cmake --install` output
# shows zero LC_RPATH load commands, and actually dlopen()-ing the installed
# libsunlight.dylib fails immediately ("Library not loaded: @rpath/
# libraylib.550.dylib ... Reason: no LC_RPATH's found"), even though the
# exact same file works fine read straight out of the build tree (where
# CMake's default *build-tree* rpath - correct, but never meant to survive
# install - still applies). @loader_path/$ORIGIN tell the installed library
# to look for @rpath-referenced dependencies in its own install directory,
# which is exactly where install(TARGETS) above already puts all of them.
set_target_properties(sunlight tmx PROPERTIES
  INSTALL_RPATH "@loader_path;$ORIGIN")

# raylib/tmx are fetched from source, not find_package-imported. install(EXPORT)
# would require the whole transitive dependency graph (raylib -> glfw -> ...)
# to also be export-set-clean, which vendored FetchContent targets aren't
# designed for. Instead: install the binaries directly (no EXPORT, so no graph
# validation - this mirrors how samples/ already vendors these runtime deps
# next to each sample executable) and hand-write the imported target in
# sunlightConfig.cmake.in rather than generating one via install(EXPORT).
install(TARGETS sunlight raylib tmx
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/sunlightConfig.cmake.in
                               ${CMAKE_BINARY_DIR}/sunlightConfig.cmake
                               INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sunlight
                               PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_BINDIR)

write_basic_package_version_file(${CMAKE_BINARY_DIR}/sunlightConfigVersion.cmake
                                  VERSION ${PROJECT_VERSION}
                                  COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_BINARY_DIR}/sunlightConfig.cmake" "${CMAKE_BINARY_DIR}/sunlightConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sunlight)
