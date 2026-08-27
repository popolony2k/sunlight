/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <doctest/doctest.h>
#include "filesystem/ifilesystem.h"

using namespace SunLight :: FileSystem;

TEST_SUITE( "filesystem/IFileSystem::ToVirtualPath" )  {

    TEST_CASE( "Strips a Windows drive letter and normalizes backslashes" )  {

        CHECK( IFileSystem :: ToVirtualPath( "D:\\Projects\\game-engine\\build\\games\\caravellius.zip" )
               == "/Projects/game-engine/build/games/caravellius.zip" );
    }

    TEST_CASE( "Strips a lowercase drive letter just as well" )  {

        CHECK( IFileSystem :: ToVirtualPath( "c:\\Users\\dev" ) == "/Users/dev" );
    }

    TEST_CASE( "Normalizes backslashes even without a drive letter" )  {

        // eg. a UNC-style or already-rooted path with no drive letter -
        // strVirtual[1] isn't ':', so the drive-letter strip is skipped,
        // but every '\' still becomes '/'.
        CHECK( IFileSystem :: ToVirtualPath( "\\\\server\\share\\file.txt" ) == "//server/share/file.txt" );
    }

    TEST_CASE( "Leaves a POSIX absolute path untouched" )  {

        CHECK( IFileSystem :: ToVirtualPath( "/home/user/game" ) == "/home/user/game" );
    }

    TEST_CASE( "Leaves a relative path untouched" )  {

        CHECK( IFileSystem :: ToVirtualPath( "samples/tilemaprenderer/" ) == "samples/tilemaprenderer/" );
    }

    TEST_CASE( "Is idempotent - safe to call on it's own prior output" )  {

        std :: string  strOnce = IFileSystem :: ToVirtualPath( "D:\\Projects\\game" );
        std :: string  strTwice = IFileSystem :: ToVirtualPath( strOnce );

        CHECK( strOnce == strTwice );
        CHECK( strTwice == "/Projects/game" );
    }

    TEST_CASE( "Handles empty and single-character strings without crashing" )  {

        CHECK( IFileSystem :: ToVirtualPath( "" ) == "" );
        CHECK( IFileSystem :: ToVirtualPath( "/" ) == "/" );
        CHECK( IFileSystem :: ToVirtualPath( "D" ) == "D" );
    }
}
