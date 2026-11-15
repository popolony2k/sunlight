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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <physfs.h>
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

    TEST_CASE( "Mount works as the very first call: no Init and no read before it" )  {

        ScratchDirFixture  fixture;

        // Start from a process where PhysFS was never set up - what a consumer that mounts first sees.
        if( ::PHYSFS_isInit() )
            ::PHYSFS_deinit();

        PhysFsFileSystem  fs;

        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }

    TEST_CASE( "Mount over a loose directory makes it's contents readable" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );

        CHECK( fs.Exists( "/subdir/greeting.txt" ) );
        CHECK( fs.Exists( "/subdir" ) );        // PHYSFS_exists answers for a folder too (see VirtualExists)
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

        fs.SetReadFilter( []( const std :: vector<unsigned char>& in, std :: vector<unsigned char>& out )  {
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

        fs.SetReadFilter( []( const std :: vector<unsigned char> &, std :: vector<unsigned char>& out )  {
            out = { 'X' };
            return true;
        } );

        fs.SetReadFilter( SunLight :: FileSystem :: IFileSystem :: ReadFilterCallback() );  // clear it

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "/subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
    }

    // I12 of the master plan: the write directory.

    TEST_CASE( "A virtual write needs a write directory" )  {

        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        CHECK_FALSE( fs.WriteFile( "settings/bindings.txt", { 'a' } ) );
        fs.Shutdown();
    }

    TEST_CASE( "SetWriteDirectory creates the folder; a virtual write lands there, folders and all, and reads back" )  {

        ScratchDirFixture           fixture;
        PhysFsFileSystem            fs;
        std :: filesystem :: path   writeDir = fixture.root / "user";     // does not exist yet

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.SetWriteDirectory( writeDir.string() ) );
        CHECK( std :: filesystem :: is_directory( writeDir ) );

        REQUIRE( fs.WriteFile( "settings/deep/bindings.txt", { 'h', 'i' } ) );

        std :: ifstream  file( writeDir / "settings" / "deep" / "bindings.txt", std :: ios :: binary );
        std :: string    strOnDisk( ( std :: istreambuf_iterator<char>( file ) ), std :: istreambuf_iterator<char>() );

        CHECK( strOnDisk == "hi" );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "settings/deep/bindings.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hi" );
        CHECK( fs.Exists( "vpath://settings/deep/bindings.txt" ) );

        fs.Shutdown();
    }

    TEST_CASE( "A file in the write directory wins over a mounted file of the same name" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );     // ships subdir/greeting.txt
        REQUIRE( fs.SetWriteDirectory( ( fixture.root.parent_path() / "sunlight_physfs_user" ).string() ) );
        REQUIRE( fs.WriteFile( "subdir/greeting.txt", { 'u', 's', 'e', 'r' } ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "user" );

        fs.Shutdown();
        std :: filesystem :: remove_all( fixture.root.parent_path() / "sunlight_physfs_user" );
    }

    TEST_CASE( "A write directory is not a mount: a consumer that never mounts still gets the working directory" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.SetWriteDirectory( ( fixture.root.parent_path() / "sunlight_physfs_user" ).string() ) );

        std :: filesystem :: path  previousCwd = std :: filesystem :: current_path();

        std :: filesystem :: current_path( fixture.root );

        std :: vector<unsigned char>  data;
        bool                          bRead = fs.ReadFile( "subdir/greeting.txt", data );

        std :: filesystem :: current_path( previousCwd );

        REQUIRE( bRead );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
        std :: filesystem :: remove_all( fixture.root.parent_path() / "sunlight_physfs_user" );
    }

    // I14 of the master plan: the virtual write is atomic, and RemoveFile.

    TEST_CASE( "A virtual write replaces the old file atomically, reads back at once, and leaves no temporary file" )  {

        ScratchDirFixture           fixture;
        PhysFsFileSystem            fs;
        std :: filesystem :: path   writeDir = fixture.root / "user";

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.SetWriteDirectory( writeDir.string() ) );
        REQUIRE( fs.WriteFile( "saves/slot1.json", { 'o', 'l', 'd' } ) );
        REQUIRE( fs.WriteFile( "saves/slot1.json", { 'n', 'e', 'w', '!' } ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "saves/slot1.json", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "new!" );
        CHECK_FALSE( std :: filesystem :: exists( writeDir / "saves" / "slot1.json.tmp" ) );

        fs.Shutdown();
    }

    TEST_CASE( "A failed virtual write leaves the old file as it was" )  {

        ScratchDirFixture           fixture;
        PhysFsFileSystem            fs;
        std :: filesystem :: path   writeDir = fixture.root / "user";

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.SetWriteDirectory( writeDir.string() ) );
        REQUIRE( fs.WriteFile( "slot1.json", { 'o', 'l', 'd' } ) );
        std :: filesystem :: create_directories( writeDir / "slot1.json.tmp" );    // the temporary name is taken

        CHECK_FALSE( fs.WriteFile( "slot1.json", { 'n', 'e', 'w' } ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "slot1.json", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "old" );

        fs.Shutdown();
    }

    TEST_CASE( "A virtual write or remove never leaves the write directory" )  {

        ScratchDirFixture           fixture;
        PhysFsFileSystem            fs;
        std :: filesystem :: path   writeDir = fixture.root / "user";

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.SetWriteDirectory( writeDir.string() ) );

        CHECK_FALSE( fs.WriteFile( "../escaped.txt", { 'x' } ) );
        CHECK_FALSE( fs.WriteFile( "saves/../../escaped.txt", { 'x' } ) );
        CHECK_FALSE( fs.WriteFile( "./x.txt", { 'x' } ) );
        CHECK_FALSE( fs.WriteFile( "C:/x.txt", { 'x' } ) );
        CHECK_FALSE( fs.WriteFile( "", { 'x' } ) );
        CHECK_FALSE( std :: filesystem :: exists( fixture.root / "escaped.txt" ) );
        CHECK_FALSE( fs.RemoveFile( "../subdir/greeting.txt" ) );
        CHECK( std :: filesystem :: exists( fixture.root / "subdir" / "greeting.txt" ) );

        fs.Shutdown();
    }

    TEST_CASE( "RemoveFile removes a saved file only: a shipped file of the same name stays, and reads again" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/", true ) );                    // ships subdir/greeting.txt
        CHECK_FALSE( fs.RemoveFile( "subdir/greeting.txt" ) );                       // no write directory

        REQUIRE( fs.SetWriteDirectory( ( fixture.root.parent_path() / "sunlight_physfs_user" ).string() ) );
        CHECK_FALSE( fs.RemoveFile( "subdir/greeting.txt" ) );                       // only the mount has it

        REQUIRE( fs.WriteFile( "subdir/greeting.txt", { 'u', 's', 'e', 'r' } ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "user" );

        CHECK( fs.RemoveFile( "subdir/greeting.txt" ) );
        REQUIRE( fs.ReadFile( "subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );   // the shipped one again
        CHECK( std :: filesystem :: exists( fixture.root / "subdir" / "greeting.txt" ) );
        CHECK_FALSE( fs.RemoveFile( "subdir/greeting.txt" ) );                       // the saved one is gone

        fs.Shutdown();
        std :: filesystem :: remove_all( fixture.root.parent_path() / "sunlight_physfs_user" );
    }

    TEST_CASE( "RemoveFile never removes a file of a mounted .zip" )  {

        ScratchDirFixture           fixture;
        std :: filesystem :: path   zipPath = fixture.root.parent_path() / "sunlight_physfs_remove.zip";

        std :: filesystem :: remove( zipPath );

        std :: string  strZipCmd = "cmake -E chdir \"" + fixture.root.string() + "\" cmake -E tar cf \"" + zipPath.string() + "\" --format=zip .";

        REQUIRE( std :: system( strZipCmd.c_str() ) == 0 );

        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( zipPath.string(), "/", true ) );
        REQUIRE( fs.SetWriteDirectory( ( fixture.root.parent_path() / "sunlight_physfs_user" ).string() ) );
        CHECK_FALSE( fs.RemoveFile( "subdir/greeting.txt" ) );

        std :: vector<unsigned char>  data;

        REQUIRE( fs.ReadFile( "subdir/greeting.txt", data ) );
        CHECK( std :: string( data.begin(), data.end() ) == "hello from physfs" );

        fs.Shutdown();
        std :: filesystem :: remove( zipPath );
        std :: filesystem :: remove_all( fixture.root.parent_path() / "sunlight_physfs_user" );
    }
    // I22 of the master plan: the same folder or archive at several mount points, and Unmount.

    namespace  {

        // Two real folders, each with a file "f" whose contents name its folder.
        struct TwoFolders  {

            std :: filesystem :: path  one, two;

            TwoFolders( void )  {
                one = std :: filesystem :: temp_directory_path() / "sunlight_physfs_one";
                two = std :: filesystem :: temp_directory_path() / "sunlight_physfs_two";

                for( const std :: filesystem :: path& dir : { one, two } )  {
                    std :: filesystem :: remove_all( dir );
                    std :: filesystem :: create_directories( dir );

                    std :: ofstream  file( dir / "f", std :: ios :: binary );

                    file << dir.filename().string();
                }
            }

            ~TwoFolders( void )  {
                std :: filesystem :: remove_all( one );
                std :: filesystem :: remove_all( two );
            }
        };

        std :: string Read( PhysFsFileSystem& fs, const char* szPath )  {
            std :: vector<unsigned char>  data;

            return fs.ReadFile( szPath, data ) ? std :: string( data.begin(), data.end() ) : std :: string( "<none>" );
        }
    }

    TEST_CASE( "I22: one folder mounted at two mount points is read at both" )  {

        ScratchDirFixture  fixture;
        PhysFsFileSystem   fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/a", true ) );
        REQUIRE( fs.Mount( fixture.root.string(), "/b", true ) );        // PhysFS alone ignored this one

        CHECK( Read( fs, "/a/subdir/greeting.txt" ) == "hello from physfs" );
        CHECK( Read( fs, "/b/subdir/greeting.txt" ) == "hello from physfs" );

        fs.Shutdown();
    }

    TEST_CASE( "I22: one .zip mounted at two mount points is read at both" )  {

        ScratchDirFixture           fixture;
        std :: filesystem :: path  zipPath = fixture.root.parent_path() / "sunlight_physfs_twice.zip";
        std :: string              strZipCmd = "cmake -E chdir \"" + fixture.root.string() +
                                               "\" cmake -E tar cf \"" + zipPath.string() + "\" --format=zip .";

        std :: filesystem :: remove( zipPath );
        REQUIRE( std :: system( strZipCmd.c_str() ) == 0 );

        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( zipPath.string(), "/app", true ) );
        REQUIRE( fs.Mount( zipPath.string(), "/", true ) );

        CHECK( Read( fs, "/app/subdir/greeting.txt" ) == "hello from physfs" );
        CHECK( Read( fs, "/subdir/greeting.txt" ) == "hello from physfs" );

        fs.Shutdown();
        std :: filesystem :: remove( zipPath );
    }

    TEST_CASE( "I22: each mount keeps its own place in the search order" )  {

        TwoFolders        folders;
        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( folders.one.string(), "/", true ) );          // root: one, then two
        REQUIRE( fs.Mount( folders.two.string(), "/", true ) );
        REQUIRE( fs.Mount( folders.one.string(), "/m", true ) );         // "/m": two (in front), then one
        REQUIRE( fs.Mount( folders.two.string(), "/m", false ) );

        CHECK( Read( fs, "/f" ) == "sunlight_physfs_one" );
        CHECK( Read( fs, "/m/f" ) == "sunlight_physfs_two" );

        fs.Shutdown();
    }

    TEST_CASE( "I22: Unmount removes that one mount; the same pair mounted twice is one mount" )  {

        TwoFolders        folders;
        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( folders.one.string(), "/a", true ) );
        REQUIRE( fs.Mount( folders.one.string(), "/b", true ) );
        REQUIRE( fs.Mount( folders.one.string(), "/b/", true ) );        // the same pair: nothing new

        CHECK( fs.Unmount( folders.one.string(), "/a" ) == true );
        CHECK( Read( fs, "/a/f" ) == "<none>" );
        CHECK( Read( fs, "/b/f" ) == "sunlight_physfs_one" );           // the other mount stays

        CHECK( fs.Unmount( folders.one.string(), "b" ) == true );        // "/b", "/b/" and "b" are one mount point
        CHECK( fs.Unmount( folders.one.string(), "/b" ) == false );       // gone: there was one mount, not two
        CHECK( fs.Unmount( folders.two.string(), "/a" ) == false );       // never mounted
        CHECK( Read( fs, "/b/f" ) == "<none>" );

        // Mounted again after being removed: back.
        REQUIRE( fs.Mount( folders.one.string(), "/a", true ) );
        CHECK( Read( fs, "/a/f" ) == "sunlight_physfs_one" );

        fs.Shutdown();
    }

    TEST_CASE( "I22: a folder mounted by the game and then made the write directory keeps both mounts, whatever the write directory becomes" )  {

        TwoFolders        folders;
        PhysFsFileSystem  fs;

        REQUIRE( fs.Init( "sunlight_tests" ) );
        REQUIRE( fs.Mount( folders.one.string(), "/saves", true ) );
        REQUIRE( fs.SetWriteDirectory( folders.one.string() ) );

        CHECK( Read( fs, "/f" ) == "sunlight_physfs_one" );             // the write directory, at the root
        CHECK( Read( fs, "/saves/f" ) == "sunlight_physfs_one" );

        // Another write directory: the old one leaves the root, the game's own mount stays.
        REQUIRE( fs.SetWriteDirectory( folders.two.string() ) );
        CHECK( Read( fs, "/f" ) == "sunlight_physfs_two" );
        CHECK( Read( fs, "/saves/f" ) == "sunlight_physfs_one" );

        // And Unmount never touches the write directory's mount.
        CHECK( fs.Unmount( folders.two.string(), "/" ) == false );
        CHECK( Read( fs, "/f" ) == "sunlight_physfs_two" );

        fs.Shutdown();
    }

}
