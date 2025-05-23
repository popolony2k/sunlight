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

project (tilemaprenderer_test)

include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(FetchContent)
include(CTest)

if(APPLE)
    #
    # Setup CLang to C++17 (minimum for working with some raylib defines when using APPLE)
    #
    set(CMAKE_CXX_STANDARD 17)
endif()

file(GLOB_RECURSE TEST_APP_SOURCES ./*.cpp)
file(GLOB TEST_APP_HEADERS ./*.h)

add_executable(tilemaprenderer_test ${TEST_APP_SOURCES} ${TEST_APP_HEADERS})
add_dependencies(tilemaprenderer_test sunlight)
target_link_libraries(tilemaprenderer_test PRIVATE sunlight)

add_custom_target(tilemaprenderer_test_copy_binaries
COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LibXml2> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raylib> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:tmx> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sunlight> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMENT "Copying TileMaprenderer test sample dependency binaries" VERBATIM)
add_dependencies(tilemaprenderer_test tilemaprenderer_test_copy_binaries)

