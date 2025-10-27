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

project (sunlight)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FetchContent)

option(SUNLIGHT_DEBUG_SYMBOLS_UNIX "Build SunLight with debug symbols on Unixes (Mac and Linux)" ON)

#
# Setup default engine (DEFAULT_ENGINE preprocessor)
# 1 - RAYLIB
#
set(SUNLIGHT_DEFAULT_BACKEND_ENGINE 1)

if(NOT USE_LOCAL_LIBXML2)
    ####################################################################
    # LibXml2 (TMX dependency)
    ####################################################################
    if(APPLE)
        add_definitions(-DHAVE_SYS_RANDOM_H=1)
    endif()

    set(LIBXML2_WITH_PYTHON OFF) # Turn off Python bind build dependency
    set(LIBXML2_WITH_TESTS OFF)  # Turn off tests build dependency

    FetchContent_Declare(libxml2 OVERRIDE_FIND_PACKAGE
                         GIT_REPOSITORY https://github.com/GNOME/libxml2.git
                         GIT_TAG master)
    FetchContent_MakeAvailable(libxml2)
endif()

####################################################################
# TMX
####################################################################
FetchContent_Declare(tmx 
                     GIT_REPOSITORY https://github.com/baylej/tmx.git
                     GIT_TAG master)
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
include_directories("concurrent")
include_directories("general")

# Add all source files
file( GLOB_RECURSE LIBRARY_SOURCES "./" *.cpp )
file( GLOB LIBRARY_HEADERS "./" *.h )

file(GLOB_RECURSE REMOVE_CMAKE_FILES CMakeFiles/*)
file(GLOB_RECURSE REMOVE_BUILD_FILES build/*)
#list(REMOVE_ITEM LIBRARY_SOURCES ${REMOVE_BUILD_FILES} ${REMOVE_CMAKE_FILES})


if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(APPLE)
  #
  # Setup CLang to C++17 (minimum for working with some raylib defines when using APPLE)
  #
  set(CMAKE_CXX_STANDARD 17)
endif()

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

target_link_libraries(sunlight ${CMAKE_THREAD_LIBS_INIT} raylib tmx LibXml2::LibXml2 ZLIB::ZLIB ${CMAKE_DL_LIBS})

# Installation

target_include_directories(sunlight INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}> $<INSTALL_INTERFACE:include>)

install(FILES "${CMAKE_BINARY_DIR}/sunlightConfig.cmake" "${CMAKE_BINARY_DIR}/sunlightConfigVersion.cmake"
        DESTINATION "lib/cmake/sunlight")
