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

project (sunlight_tests)

include(FetchContent)

if(APPLE)
    set(CMAKE_CXX_STANDARD 17)
endif()

####################################################################
# doctest
####################################################################
FetchContent_Declare(doctest
                     GIT_REPOSITORY https://github.com/doctest/doctest.git
                     GIT_TAG v2.4.11)
FetchContent_MakeAvailable(doctest)

file(GLOB_RECURSE SUNLIGHT_TEST_SOURCES ./*.cpp)

add_executable(sunlight_tests ${SUNLIGHT_TEST_SOURCES})
add_dependencies(sunlight_tests sunlight)
target_link_libraries(sunlight_tests PRIVATE sunlight doctest::doctest)

add_custom_target(sunlight_tests_copy_binaries
COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LibXml2> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raylib> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:tmx> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sunlight> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
COMMENT "Copying sunlight_tests dependency binaries" VERBATIM)
add_dependencies(sunlight_tests sunlight_tests_copy_binaries)

add_test(NAME sunlight_tests COMMAND sunlight_tests)
