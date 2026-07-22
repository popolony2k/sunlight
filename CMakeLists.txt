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

project (sunlight_all)

option(BUILD_LIBRARY_SAMPLES "Build SunLight library samples" OFF)
option(BUILD_LIBRARY_TESTS "Build SunLight library unit tests" OFF)

# Core library
add_subdirectory(src)

# Samples
if(BUILD_LIBRARY_SAMPLES)
    add_subdirectory(samples/tilemaprenderer)
    add_subdirectory(samples/sprite)
    add_subdirectory(samples/collision)
    add_subdirectory(samples/gamepad)
    add_subdirectory(samples/scriptprocessor)
endif(BUILD_LIBRARY_SAMPLES)

# Unit tests
if(BUILD_LIBRARY_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif(BUILD_LIBRARY_TESTS)
