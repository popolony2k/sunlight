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
#include <filesystem>
#include <fstream>
#include "backends/physfs/physfsfilesystem.h"

using namespace SunLight :: FileSystem :: PhysFs;

namespace  {

    /**
     * @brief Creates a real, throwaway directory (with one real
     * subdirectory + text file inside it) on disk for each test case to
     * mount, and removes it again on scope exit - the one exception to
     * this test suite's own "no real window/display" scope (see this
     * project's own CLAUDE.md), since PhysFsFileSystem's whole job is
     * real filesystem I/O, not window/render state.
     */
    struct ScratchDirFixture  {

        std :: filesystem :: path  root;

        ScratchDirFixture( void )  {
            root = std :: filesystem :: temp_directory_path() / "sunlight_physfs_test";

            std :: filesystem :: remove_all( root );
            std :: filesystem :: create_directories( root / "subdir" );

            std :: ofstream  file( root / "subdir" / "greeting.txt", std :: ios :: binary );
            file << "hello from physfs";
        }

        ~ScratchDirFixture( void )  {
            std :: filesystem :: remove_all( root );
        }
    };
}

TEST_SUITE( "backends/physfs/PhysFsFileSystem" )  {

    TEST_CASE( "Mount over a loose directory makes it's contents readable" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        CHECK( fs.Exists( "/subdir/greeting.txt" ) );
        CHECK_FALSE( fs.Exists( "/subdir/does_not_exist.txt" ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( data.size() == 17 );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }

    TEST_CASE( "Mount over a real zip archive reads identically to a loose directory" )  {

        ScratchDirFixture  fixture;

        // Build a real .zip from the same fixture directory - PHYSFS_mount
        // must not need to know or care which one it's given.
        std :: filesystem :: path  zipPath = fixture.root.parent_path() / "sunlight_physfs_test.zip";

        std :: filesystem :: remove( zipPath );

        std :: string  strZipCmd = "cd \"" + fixture.root.string() + "\" && zip -q -r \"" + zipPath.string() + "\" .";

        REQUIRE( std :: system( strZipCmd.c_str() ) == 0 );

        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( zipPath.string(), "/", true ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();

        std :: filesystem :: remove( zipPath );
    }

    TEST_CASE( "ReadFile/Exists on an unmounted path fail cleanly, not crash" )  {

        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );

        CHECK_FALSE( fs.Exists( "/nothing/mounted/here.txt" ) );

        std :: vector<unsigned char>  data;

        CHECK_FALSE( fs.ReadFile( "/nothing/mounted/here.txt", data ) );

        fs.Shutdown();
    }

    TEST_CASE( "Init/Shutdown are safe to call more than once" )  {

        PhysFsFileSystem  fs;

        CHECK( fs.Init( "sunlight_tests" ) );
        CHECK( fs.Init( "sunlight_tests" ) );  // second call, already initialized

        fs.Shutdown();
        fs.Shutdown();  // second call, already shut down
    }
}
