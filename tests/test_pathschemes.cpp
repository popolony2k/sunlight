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

/*
 * I12 of the master plan: IFileSystem's path schemes - "vpath://" / "virtualpath://" (and no
 * scheme) for a virtual path, "hostpath://" / "hpath://" for an OS path - its host-path reads and
 * writes, the switch that refuses host paths, and the read and write filters. All of it is
 * IFileSystem's own, the same for every backend; here over the in-memory one (no global PhysFS),
 * with real temp files for the host paths.
 */

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include "filesystem/filepath.h"
#include "mock_filesystem.h"

typedef SunLight :: FileSystem :: IFileSystem  IFS;
typedef SunLight :: FileSystem :: FilePath     FP;

namespace  {

    namespace fs = std :: filesystem;

    // A scratch folder for host files, removed on scope exit.
    struct HostScratch  {

        fs :: path  root;

        HostScratch( void )  {
            root = fs :: temp_directory_path() / "sunlight_pathschemes_test";
            fs :: remove_all( root );
            fs :: create_directories( root );
        }

        ~HostScratch( void )  {
            fs :: remove_all( root );
        }

        std :: string Host( const char* szName ) const  {
            return "hpath://" + ( root / szName ).string();
        }

        std :: string OnDisk( const char* szName ) const  {
            std :: ifstream  file( root / szName, std :: ios :: binary );

            return std :: string( ( std :: istreambuf_iterator<char>( file ) ), std :: istreambuf_iterator<char>() );
        }
    };

    Bytes ToBytes( const std :: string& str )  { return Bytes( str.begin(), str.end() ); }
    std :: string ToString( const Bytes& bytes )  { return std :: string( bytes.begin(), bytes.end() ); }

    // XOR 0x5A, its own inverse: the "encryption" of a pack and of a save.
    bool Scramble( const Bytes& in, Bytes& out )  {
        out = in;

        for( unsigned char& c : out )
            c = ( unsigned char ) ( c ^ 0x5A );

        return true;
    }
}

TEST_SUITE( "filesystem/path_schemes" )  {

    TEST_CASE( "SplitPath: no scheme, vpath:// and virtualpath:// are virtual; hostpath:// and hpath:// are host" )  {

        std :: string  strRest;

        CHECK( FP :: SplitPath( "maps/level.tmx", strRest ) == FP :: PATH_SCHEME_VIRTUAL );
        CHECK( strRest == "maps/level.tmx" );
        CHECK( FP :: SplitPath( "vpath://maps/level.tmx", strRest ) == FP :: PATH_SCHEME_VIRTUAL );
        CHECK( strRest == "maps/level.tmx" );
        CHECK( FP :: SplitPath( "virtualpath://maps/level.tmx", strRest ) == FP :: PATH_SCHEME_VIRTUAL );
        CHECK( strRest == "maps/level.tmx" );
        CHECK( FP :: SplitPath( "hostpath:///home/me/a.txt", strRest ) == FP :: PATH_SCHEME_HOST );
        CHECK( strRest == "/home/me/a.txt" );
        CHECK( FP :: SplitPath( "hpath://C:\\Users\\me\\a.txt", strRest ) == FP :: PATH_SCHEME_HOST );
        CHECK( strRest == "C:\\Users\\me\\a.txt" );
    }

    TEST_CASE( "SplitPath: any other scheme is unknown; '://' after a non-letter is not a scheme" )  {

        std :: string  strRest;

        CHECK( FP :: SplitPath( "http://example.com/a", strRest ) == FP :: PATH_SCHEME_UNKNOWN );
        CHECK( FP :: SplitPath( "VPATH://a", strRest ) == FP :: PATH_SCHEME_UNKNOWN );     // schemes are lowercase
        CHECK( FP :: SplitPath( "maps/a://b", strRest ) == FP :: PATH_SCHEME_VIRTUAL );
        CHECK( strRest == "maps/a://b" );
        CHECK( FP :: SplitPath( "://a", strRest ) == FP :: PATH_SCHEME_VIRTUAL );
    }

    TEST_CASE( "Every virtual spelling reads the same virtual file; an unknown scheme is refused everywhere" )  {

        MemoryFileSystem  memory;
        Bytes             data;

        memory.files["maps/level.tmx"] = ToBytes( "map" );
        memory.SetWriteDirectory( "unused" );

        for( const char* szPath : { "maps/level.tmx", "vpath://maps/level.tmx", "virtualpath://maps/level.tmx" } )  {
            INFO( szPath );
            CHECK( memory.Exists( szPath ) );
            REQUIRE( memory.ReadFile( szPath, data ) );
            CHECK( ToString( data ) == "map" );
        }

        CHECK_FALSE( memory.Exists( "http://maps/level.tmx" ) );
        CHECK_FALSE( memory.ReadFile( "http://maps/level.tmx", data ) );
        CHECK_FALSE( memory.WriteFile( "http://maps/level.tmx", ToBytes( "x" ) ) );
    }

    TEST_CASE( "A host path reads, writes (making its folders) and exists on the OS, with either spelling" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;
        Bytes             data;

        REQUIRE( memory.WriteFile( scratch.Host( "sub/dir/a.txt" ), ToBytes( "on disk" ) ) );
        CHECK( scratch.OnDisk( "sub/dir/a.txt" ) == "on disk" );
        CHECK( memory.Exists( scratch.Host( "sub/dir/a.txt" ) ) );
        CHECK( memory.Exists( "hostpath://" + ( scratch.root / "sub/dir/a.txt" ).string() ) );
        REQUIRE( memory.ReadFile( "hostpath://" + ( scratch.root / "sub/dir/a.txt" ).string(), data ) );
        CHECK( ToString( data ) == "on disk" );

        CHECK_FALSE( memory.Exists( scratch.Host( "missing.txt" ) ) );
        CHECK_FALSE( memory.ReadFile( scratch.Host( "missing.txt" ), data ) );
        CHECK_FALSE( memory.Exists( scratch.Host( "sub" ) ) );           // a folder is not a file
        CHECK( memory.files.empty() );                                    // nothing went to the virtual side
    }

    TEST_CASE( "With host paths refused, every host read, write and check fails; virtual paths still work" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;
        Bytes             data;

        memory.files["a.txt"] = ToBytes( "virtual" );
        REQUIRE( memory.WriteFile( scratch.Host( "a.txt" ), ToBytes( "host" ) ) );
        CHECK( memory.IsHostPathEnabled() );                              // allowed by default

        memory.SetHostPathEnabled( false );
        CHECK_FALSE( memory.IsHostPathEnabled() );
        CHECK_FALSE( memory.Exists( scratch.Host( "a.txt" ) ) );
        CHECK_FALSE( memory.ReadFile( scratch.Host( "a.txt" ), data ) );
        CHECK_FALSE( memory.WriteFile( scratch.Host( "b.txt" ), ToBytes( "x" ) ) );
        CHECK_FALSE( fs :: exists( scratch.root / "b.txt" ) );
        REQUIRE( memory.ReadFile( "a.txt", data ) );
        CHECK( ToString( data ) == "virtual" );

        memory.SetHostPathEnabled( true );
        REQUIRE( memory.ReadFile( scratch.Host( "a.txt" ), data ) );
        CHECK( ToString( data ) == "host" );
    }

    TEST_CASE( "The read filter applies to host reads as to virtual ones" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;
        Bytes             scrambled;
        Bytes             data;

        Scramble( ToBytes( "secret" ), scrambled );
        memory.files["pack/a.txt"] = scrambled;
        std :: ofstream( scratch.root / "a.txt", std :: ios :: binary ).write( ( const char* ) scrambled.data(), ( std :: streamsize ) scrambled.size() );

        memory.SetReadFilter( Scramble );

        REQUIRE( memory.ReadFile( "pack/a.txt", data ) );
        CHECK( ToString( data ) == "secret" );
        REQUIRE( memory.ReadFile( scratch.Host( "a.txt" ), data ) );
        CHECK( ToString( data ) == "secret" );
    }

    TEST_CASE( "The write filter changes what is written; with the matching read filter a save reads back as given" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;
        Bytes             data;

        memory.SetWriteDirectory( "user" );

        // No write filter: the bytes as given.
        REQUIRE( memory.WriteFile( scratch.Host( "plain.txt" ), ToBytes( "plain" ) ) );
        CHECK( scratch.OnDisk( "plain.txt" ) == "plain" );

        memory.SetWriteFilter( Scramble );
        memory.SetReadFilter( Scramble );

        REQUIRE( memory.WriteFile( scratch.Host( "save.txt" ), ToBytes( "settings" ) ) );
        REQUIRE( memory.WriteFile( "save.txt", ToBytes( "settings" ) ) );
        CHECK( scratch.OnDisk( "save.txt" ) != "settings" );               // stored scrambled
        CHECK( ToString( memory.files["save.txt"] ) != "settings" );

        REQUIRE( memory.ReadFile( scratch.Host( "save.txt" ), data ) );
        CHECK( ToString( data ) == "settings" );
        REQUIRE( memory.ReadFile( "save.txt", data ) );
        CHECK( ToString( data ) == "settings" );

        // A write filter that returns false leaves the bytes; an empty one clears it.
        memory.SetWriteFilter( []( const Bytes&, Bytes& ) { return false; } );
        REQUIRE( memory.WriteFile( scratch.Host( "kept.txt" ), ToBytes( "kept" ) ) );
        CHECK( scratch.OnDisk( "kept.txt" ) == "kept" );
        memory.SetWriteFilter( IFS :: WriteFilterCallback() );
        REQUIRE( memory.WriteFile( scratch.Host( "cleared.txt" ), ToBytes( "cleared" ) ) );
        CHECK( scratch.OnDisk( "cleared.txt" ) == "cleared" );
    }

    TEST_CASE( "A virtual write needs a write directory, in every backend" )  {

        MemoryFileSystem  memory;

        CHECK_FALSE( memory.WriteFile( "save.txt", ToBytes( "x" ) ) );
        memory.SetWriteDirectory( "user" );
        CHECK( memory.WriteFile( "vpath://save.txt", ToBytes( "x" ) ) );
        CHECK( ToString( memory.files["save.txt"] ) == "x" );
    }

    // I13 of the master plan: the path helpers, public on IFileSystem (moved from the map loader).

    TEST_CASE( "ParentPath and JoinPath keep a path's scheme and collapse only the rest" )  {

        CHECK( FP :: ParentPath( "vpath://maps/level.tmx" ) == "vpath://maps/" );
        CHECK( FP :: ParentPath( "vpath://level.tmx" ) == "vpath://" );
        CHECK( FP :: ParentPath( "hpath:///abs/x/level.tmx" ) == "hpath:///abs/x/" );
        CHECK( FP :: ParentPath( "maps/level.tmx" ) == "maps/" );           // plain: as before
        CHECK( FP :: ParentPath( "level.tmx" ) == "" );

        CHECK( FP :: JoinPath( "vpath://a/", "../c.tsx" ) == "vpath://c.tsx" );
        CHECK( FP :: JoinPath( "virtualpath://a/b/", "./c.tsx" ) == "virtualpath://a/b/c.tsx" );
        CHECK( FP :: JoinPath( "vpath://", "c.tsx" ) == "vpath://c.tsx" );
        CHECK( FP :: JoinPath( "vpath://a/", "../../c.tsx" ) == "" );               // above the root
        CHECK( FP :: JoinPath( "hpath:///abs/x/", "c.tsx" ) == "hpath:///abs/x/c.tsx" );
        CHECK( FP :: JoinPath( "hostpath:///abs/x/", "../y/c.tsx" ) == "hostpath:///abs/y/c.tsx" );
        CHECK( FP :: JoinPath( "hpath://C:\\games\\maps\\", "..\\tiles\\c.tsx" ) == "hpath://C:/games/tiles/c.tsx" );
        CHECK( FP :: JoinPath( "hpath://C:/maps/", "../../c.tsx" ) == "" );         // never above the drive
        CHECK( FP :: JoinPath( "maps/", "hpath:///abs/c.tsx" ) == "hpath:///abs/c.tsx" );   // its own scheme wins
        CHECK( FP :: JoinPath( "maps/sub/", "../../images/x.png" ) == "images/x.png" );    // plain: as before
        CHECK( FP :: JoinPath( "", "maps/sub/../../images/x.png" ) == "images/x.png" );
    }

    TEST_CASE( "ParentPath's result is what JoinPath expects: the trailing '/' is kept, and the pair builds a sibling's path" )  {

        CHECK( FP :: ParentPath( "scripts/x/game.project" ) == "scripts/x/" );
        CHECK( FP :: JoinPath( FP :: ParentPath( "scripts/x/game.project" ), "main.lua" ) == "scripts/x/main.lua" );
        CHECK( FP :: JoinPath( FP :: ParentPath( "vpath://scripts/x/game.project" ), "main.lua" ) == "vpath://scripts/x/main.lua" );
        CHECK( FP :: JoinPath( FP :: ParentPath( "game.project" ), "main.lua" ) == "main.lua" );
        CHECK( FP :: JoinPath( FP :: ParentPath( "scripts/x/game.project" ), "vpath://main.lua" ) == "vpath://main.lua" );   // its own scheme wins
    }

    TEST_CASE( "A relative path starting with '/' stays inside the folder; only an empty or absolute folder gives an absolute path" )  {

        CHECK( FP :: JoinPath( "app/", "/main.lua" ) == "app/main.lua" );
        CHECK( FP :: JoinPath( "vpath://app/", "/main.lua" ) == "vpath://app/main.lua" );
        CHECK( FP :: JoinPath( "", "/main.lua" ) == "/main.lua" );
        CHECK( FP :: JoinPath( "/app/", "main.lua" ) == "/app/main.lua" );
        CHECK( FP :: JoinPath( "hpath:///app/", "/main.lua" ) == "hpath:///app/main.lua" );
    }

    // I14 of the master plan: WriteFile is atomic (a temporary file beside the target, flushed, then
    // renamed over it), and RemoveFile.

    TEST_CASE( "An atomic host write replaces the old file, reads back at once, and leaves no temporary file" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;
        Bytes             data;

        REQUIRE( memory.WriteFile( scratch.Host( "save.json" ), ToBytes( "old" ) ) );
        REQUIRE( memory.WriteFile( scratch.Host( "save.json" ), ToBytes( "new and longer" ) ) );
        CHECK( scratch.OnDisk( "save.json" ) == "new and longer" );
        REQUIRE( memory.ReadFile( scratch.Host( "save.json" ), data ) );
        CHECK( ToString( data ) == "new and longer" );
        CHECK_FALSE( fs :: exists( scratch.root / "save.json.tmp" ) );

        REQUIRE( memory.WriteFile( scratch.Host( "save.json" ), ToBytes( "" ) ) );     // empty is a valid write
        CHECK( scratch.OnDisk( "save.json" ) == "" );
    }

    TEST_CASE( "A failed host write leaves the old file byte-identical and no temporary file of its own" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;

        REQUIRE( memory.WriteFile( scratch.Host( "save.json" ), ToBytes( "precious" ) ) );

        // The temporary name is taken by a folder: the write cannot start, and the folder is not touched.
        fs :: create_directories( scratch.root / "save.json.tmp" );
        CHECK_FALSE( memory.WriteFile( scratch.Host( "save.json" ), ToBytes( "lost?" ) ) );
        CHECK( scratch.OnDisk( "save.json" ) == "precious" );
        CHECK( fs :: is_directory( scratch.root / "save.json.tmp" ) );
        fs :: remove( scratch.root / "save.json.tmp" );

        // The target is a folder: the rename fails, the temporary file is removed, the folder stays.
        fs :: create_directories( scratch.root / "folder" );
        CHECK_FALSE( memory.WriteFile( scratch.Host( "folder" ), ToBytes( "x" ) ) );
        CHECK( fs :: is_directory( scratch.root / "folder" ) );
        CHECK_FALSE( fs :: exists( scratch.root / "folder.tmp" ) );
    }

    TEST_CASE( "RemoveFile removes a host file; false when there is none, for a folder, with host paths refused or an unknown scheme" )  {

        HostScratch       scratch;
        MemoryFileSystem  memory;
        Bytes             data;

        REQUIRE( memory.WriteFile( scratch.Host( "save.json" ), ToBytes( "x" ) ) );
        CHECK( memory.RemoveFile( scratch.Host( "save.json" ) ) );
        CHECK_FALSE( memory.Exists( scratch.Host( "save.json" ) ) );
        CHECK_FALSE( memory.ReadFile( scratch.Host( "save.json" ), data ) );
        CHECK_FALSE( memory.RemoveFile( scratch.Host( "save.json" ) ) );           // nothing there any more

        fs :: create_directories( scratch.root / "folder" );
        CHECK_FALSE( memory.RemoveFile( scratch.Host( "folder" ) ) );              // files only
        CHECK( fs :: is_directory( scratch.root / "folder" ) );

        REQUIRE( memory.WriteFile( scratch.Host( "kept.json" ), ToBytes( "x" ) ) );
        memory.SetHostPathEnabled( false );
        CHECK_FALSE( memory.RemoveFile( scratch.Host( "kept.json" ) ) );
        CHECK( fs :: exists( scratch.root / "kept.json" ) );
        memory.SetHostPathEnabled( true );

        CHECK_FALSE( memory.RemoveFile( "http://kept.json" ) );
    }

    TEST_CASE( "RemoveFile on a virtual path: only with a write directory, and only what is there" )  {

        MemoryFileSystem  memory;

        memory.files["save.json"] = ToBytes( "x" );
        CHECK_FALSE( memory.RemoveFile( "save.json" ) );                           // no write directory
        CHECK( memory.files.count( "save.json" ) == 1 );

        memory.SetWriteDirectory( "user" );
        CHECK( memory.RemoveFile( "vpath://save.json" ) );
        CHECK_FALSE( memory.Exists( "save.json" ) );
        CHECK_FALSE( memory.RemoveFile( "save.json" ) );
    }
}
