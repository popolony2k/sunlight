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
#include "filesystem/filepath.h"

using namespace SunLight :: FileSystem;

TEST_SUITE( "filesystem/FilePath::ToVirtualPath" )  {

    TEST_CASE( "Strips a Windows drive letter and normalizes backslashes" )  {

        CHECK( FilePath :: ToVirtualPath( "D:\\Projects\\game-engine\\build\\games\\caravellius.zip" )
               == "/Projects/game-engine/build/games/caravellius.zip" );
    }

    TEST_CASE( "Strips a lowercase drive letter just as well" )  {

        CHECK( FilePath :: ToVirtualPath( "c:\\Users\\dev" ) == "/Users/dev" );
    }

    TEST_CASE( "Normalizes backslashes even without a drive letter" )  {

        // eg. a UNC-style or already-rooted path with no drive letter -
        // strVirtual[1] isn't ':', so the drive-letter strip is skipped,
        // but every '\' still becomes '/'.
        CHECK( FilePath :: ToVirtualPath( "\\\\server\\share\\file.txt" ) == "//server/share/file.txt" );
    }

    TEST_CASE( "Leaves a POSIX absolute path untouched" )  {

        CHECK( FilePath :: ToVirtualPath( "/home/user/game" ) == "/home/user/game" );
    }

    TEST_CASE( "Leaves a relative path untouched" )  {

        CHECK( FilePath :: ToVirtualPath( "samples/orthogonal/" ) == "samples/orthogonal/" );
    }

    TEST_CASE( "Is idempotent - safe to call on it's own prior output" )  {

        std :: string  strOnce = FilePath :: ToVirtualPath( "D:\\Projects\\game" );
        std :: string  strTwice = FilePath :: ToVirtualPath( strOnce );

        CHECK( strOnce == strTwice );
        CHECK( strTwice == "/Projects/game" );
    }

    TEST_CASE( "Handles empty and single-character strings without crashing" )  {

        CHECK( FilePath :: ToVirtualPath( "" ) == "" );
        CHECK( FilePath :: ToVirtualPath( "/" ) == "/" );
        CHECK( FilePath :: ToVirtualPath( "D" ) == "D" );
    }

    TEST_CASE( "Strips a leading './' - the exact shape raylib's own GetDirectoryPath() produces" )  {

        // eg. GetDirectoryPath("resources/fonts/foo.fnt") -> "./resources/fonts",
        // then TextFormat("%s/%s", ..., "foo.png") -> "./resources/fonts/foo.png" -
        // a leading "./" is a "." path segment, which PhysFS's own path
        // sanitizer rejects outright (not just a whole-string special case).
        CHECK( FilePath :: ToVirtualPath( "./resources/fonts/caravellius8x8.png" )
               == "resources/fonts/caravellius8x8.png" );
    }

    TEST_CASE( "Only strips a leading './' - not a bare '.' elsewhere in the path" )  {

        // A dotfile-style name ('.hidden') must survive untouched - only
        // the specific two-character './' prefix is stripped.
        CHECK( FilePath :: ToVirtualPath( ".hidden" ) == ".hidden" );
        CHECK( FilePath :: ToVirtualPath( "resources/./fonts/foo.png" ) == "resources/./fonts/foo.png" );
    }

    TEST_CASE( "'./' stripping composes with backslash normalization and is idempotent" )  {

        CHECK( FilePath :: ToVirtualPath( ".\\resources\\fonts\\foo.png" ) == "resources/fonts/foo.png" );

        std :: string  strOnce = FilePath :: ToVirtualPath( "./resources/fonts/foo.png" );
        std :: string  strTwice = FilePath :: ToVirtualPath( strOnce );

        CHECK( strOnce == strTwice );
        CHECK( strTwice == "resources/fonts/foo.png" );
    }
}
