#
# CMake file for building sunlight game engine library.
# Copyright (c) since 2021 by PopolonY2k
#

cmake_minimum_required (VERSION 3.14)

project (sunlight)

include(FetchContent)

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
                     GIT_TAG master)
FetchContent_MakeAvailable(raylib)

###################################################################
# Mandatory libraries
###################################################################

# Add all include files
include_directories("src/include")
include_directories("src/include/base")
include_directories("src/include/input")
include_directories("src/include/canvas")
include_directories("src/include/sprite")
include_directories("src/include/collision")
include_directories("src/include/renderer")
include_directories("src/include/tilemap")
include_directories("src/include/scripting")
include_directories("src/include/engines")
include_directories("src/include/sound")
include_directories("src/include/concurrent")


# Add all source files
file( GLOB_RECURSE APPLICATION_SOURCES "src/source" *.cpp )
file( GLOB APPLICATION_HEADERS src/include/*.h )

file(GLOB_RECURSE REMOVE_CMAKE_FILES CMakeFiles/*)
file(GLOB_RECURSE REMOVE_BUILD_FILES build/*)
#list(REMOVE_ITEM APPLICATION_SOURCES ${REMOVE_BUILD_FILES} ${REMOVE_CMAKE_FILES})


if(APPLE)
  #
  # Setup CLang to C++17 (minimum for working with some raylib defines when using APPLE)
  #
  set(CMAKE_CXX_STANDARD 17)
endif()

if(UNIX) # AND NOT APPLE)
    #
    # Debug information (comment to turn off)
    #
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g2")

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

#
# Setup default engine (DEFAULT_ENGINE preprocessor)
# 1 - RAYLIB
#
add_definitions(-DDEFAULT_ENGINE=1)

add_library(sunlight ${APPLICATION_SOURCES} ${APPLICATION_HEADERS})

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
