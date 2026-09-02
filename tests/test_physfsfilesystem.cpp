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
#include <algorithm>
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
        // must not need to know or care which one it's given. Built via
        // `cmake -E tar ... --format=zip` (wrapped in `cmake -E chdir` to
        // set the working directory) rather than shelling out to the
        // `zip` CLI tool directly - `zip` isn't installed on GitHub's
        // windows-latest runners by default (confirmed live: this test
        // failed there with "'zip' is not recognized..."), while `cmake`
        // itself is guaranteed to be on PATH in this exact context (the
        // same toolchain that built and is now running this test binary).
        std :: filesystem :: path  zipPath = fixture.root.parent_path() / "sunlight_physfs_test.zip";

        std :: filesystem :: remove( zipPath );

        std :: string  strZipCmd = "cmake -E chdir \"" + fixture.root.string() +
                                    "\" cmake -E tar cf \"" + zipPath.string() + "\" --format=zip .";

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

    TEST_CASE( "A consumer that never calls Mount still gets plain CWD-relative access" )  {

        // Simulates a consumer that wires up SunLight::FileSystem's
        // Init/read calls (eg. via RaylibEngine's texture-load callback or
        // RayLibSound::Load) but never explicitly calls Mount() itself -
        // this used to mean every such read failed outright (the exact
        // gap that broke every one of sunlight's own bundled samples the
        // first time this abstraction shipped). PhysFsFileSystem's own
        // lazy EnsureReady() must fall back to mounting the process's
        // current working directory automatically, so a virtual path
        // relative to CWD still resolves - exactly like a plain
        // fopen()/ifstream always did before this abstraction existed.
        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );

        std :: filesystem :: path  previousCwd = std :: filesystem :: current_path();

        std :: filesystem :: current_path( fixture.root );

        std :: vector<unsigned char>  data;
        bool                          bRead = fs.ReadFile( "subdir/greeting.txt", data );

        std :: filesystem :: current_path( previousCwd );

        REQUIRE( bRead );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }

    TEST_CASE( "ReadFile/Exists on a path that doesn't exist anywhere fail cleanly, not crash" )  {

        // A genuinely bogus virtual path must still fail cleanly even
        // after EnsureReady()'s own default CWD fallback silently mounts
        // something (see the previous test case) - the fallback only
        // means SOME real directory is always searchable, not that every
        // arbitrary path suddenly resolves.
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

    TEST_CASE( "ReadFile with no filter set returns the unchanged raw bytes" )  {

        // Baseline - SetReadFilter is never called here, so ReadFile must
        // behave exactly as it always did before this hook existed.
        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }

    TEST_CASE( "A filter that returns true substitutes the read bytes" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        fs.SetReadFilter( []( const std :: vector<unsigned char> &in, std :: vector<unsigned char> &out )  {
            out.assign( in.rbegin(), in.rend() );  // trivial, verifiable transform: reverse the bytes
            return true;
        } );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );

        std :: string  strExpected = "hello from physfs";
        std :: reverse( strExpected.begin(), strExpected.end() );

        CHECK( std :: string( data.begin(), data.end() ) == strExpected );

        fs.Shutdown();
    }

    TEST_CASE( "A filter that returns false leaves the original bytes untouched" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        fs.SetReadFilter( []( const std :: vector<unsigned char> &, std :: vector<unsigned char> & )  {
            return false;  // declines to substitute anything
        } );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }

    TEST_CASE( "A registered filter is never invoked for a failed raw read" )  {

        PhysFsFileSystem  fs;
        bool               bFilterCalled = false;

        REQUIRE( fs.Init( "sunlight_tests" ) );

        fs.SetReadFilter( [&bFilterCalled]( const std :: vector<unsigned char> &, std :: vector<unsigned char> & )  {
            bFilterCalled = true;
            return false;
        } );

        std :: vector<unsigned char>  data;

        CHECK_FALSE( fs.ReadFile( "/nothing/mounted/here.txt", data ) );
        CHECK_FALSE( bFilterCalled );

        fs.Shutdown();
    }

    TEST_CASE( "SetReadFilter with an empty callback clears a previously-set one" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        fs.SetReadFilter( []( const std :: vector<unsigned char> &, std :: vector<unsigned char> &out )  {
            out = { 'X' };
            return true;
        } );

        fs.SetReadFilter( SunLight :: FileSystem :: IFileSystem :: ReadFilterCallback() );  // clear it

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }
}
