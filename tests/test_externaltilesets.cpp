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
 * A map's EXTERNAL tilesets (.tsx) and object templates (.tx) must be read through
 * SunLight::FileSystem like everything else - so they honour mounts (a .zip pack, with no
 * loose copy on disk) and the read filter (an encrypted pack) - instead of libtmx opening them
 * straight from the OS relative to the working directory. Nothing here needs a window or a GPU:
 * the renderer runs on a MockEngine/MockWindow and every file lives in memory or in a throwaway
 * archive.
 */

#include <doctest/doctest.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "backends/physfs/physfsfilesystem.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;

namespace  {

    // Map in maps/, its tileset in a SIBLING directory (../tilesets/), the tileset's image beside the tileset.
    const char  *g_szMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\">"
        "<tileset firstgid=\"1\" source=\"../tilesets/ground.tsx\"/>"
        "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
        "</map>";

    const char  *g_szTileset =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<tileset version=\"1.0\" name=\"ground\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"16\" columns=\"4\">"
        "<image source=\"ground.png\" width=\"64\" height=\"64\"/>"
        "</tileset>";

    // A template (in templates/) that itself points at the same tileset, relative to ITS directory.
    const char  *g_szMapWithTemplate =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\">"
        "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
        "<objectgroup id=\"2\" name=\"things\"><object id=\"1\" template=\"../templates/thing.tx\" x=\"8\" y=\"8\"/></objectgroup>"
        "</map>";

    const char  *g_szTemplate =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<template>"
        "<tileset firstgid=\"1\" source=\"../tilesets/ground.tsx\"/>"
        "<object gid=\"1\" width=\"16\" height=\"16\"/>"
        "</template>";

    Bytes ToBytes( const char *sz )  {
        std :: string  str( sz );
        return Bytes( str.begin(), str.end() );
    }

    Bytes PngHeader( unsigned nWidth, unsigned nHeight )  {

        Bytes  b = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 'I', 'H', 'D', 'R' };

        for( unsigned n : { nWidth, nHeight } )  {
            b.push_back( ( n >> 24 ) & 0xFF ); b.push_back( ( n >> 16 ) & 0xFF );
            b.push_back( ( n >> 8 ) & 0xFF );  b.push_back( n & 0xFF );
        }
        for( int n = 0; n < 5; n++ )
            b.push_back( 0 );

        return b;
    }

    // A started renderer on the mocks (default backend), ready to LoadMap.
    struct Rig  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Rig( void )  {
            RendererConfig  config;

            config.fWidth  = 320.0f;
            config.fHeight = 240.0f;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
        }

        ~Rig( void )  {
            pRenderer -> Stop();
        }
    };
}

TEST_SUITE( "renderer/external tilesets and templates" )  {

    TEST_CASE( "An external tileset is read through the FileSystem, its image resolved relative to the tileset" )  {

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;

        fsFixture.fs.files["maps/level.tmx"]      = ToBytes( g_szMap );
        fsFixture.fs.files["tilesets/ground.tsx"] = ToBytes( g_szTileset );

        // The map references "../tilesets/ground.tsx" from maps/, and the tileset its image as plain
        // "ground.png" - so the texture the renderer asks for is tilesets/ground.png.
        REQUIRE( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        SunLight :: TileMap :: stMapInfo  info {};

        REQUIRE( rig.pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.mapSize.nWidth == 4 );
        CHECK( rig.engineFixture.engine.nLoadTextureCalls == 1 );
        CHECK( rig.engineFixture.engine.strLastLoadTextureFileName == "tilesets/ground.png" );
    }

    TEST_CASE( "An object template, and the tileset it references, are read through the FileSystem too" )  {

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;

        fsFixture.fs.files["maps/level.tmx"]      = ToBytes( g_szMapWithTemplate );
        fsFixture.fs.files["templates/thing.tx"]  = ToBytes( g_szTemplate );
        fsFixture.fs.files["tilesets/ground.tsx"] = ToBytes( g_szTileset );

        REQUIRE( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        SunLight :: TileMap :: stMapInfo  info {};

        REQUIRE( rig.pRenderer -> GetMapInfo( info ) == true );
        CHECK( rig.engineFixture.engine.strLastLoadTextureFileName == "tilesets/ground.png" );
    }

    TEST_CASE( "The very first map loaded in a process works: libtmx's allocator hooks are not set up yet" )  {

        // libtmx's allocator hooks start out NULL and only its own tmx_load*/tmx_rcmgr_load* entry points
        // set them - tmx_make_resource_manager() does not, and crashes if it runs first. Whatever tests ran
        // before this one have already set them, so put the process back in its startup state.
        tmx_alloc_func = nullptr;
        tmx_free_func  = nullptr;

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;

        fsFixture.fs.files["maps/level.tmx"]      = ToBytes( g_szMap );
        fsFixture.fs.files["tilesets/ground.tsx"] = ToBytes( g_szTileset );

        CHECK( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
    }

    TEST_CASE( "References written Windows-style (backslashes) or with single quotes resolve too" )  {

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;
        std :: string            strMap( g_szMap );
        size_t                   nAt = strMap.find( "source=\"../tilesets/ground.tsx\"" );

        REQUIRE( nAt != std :: string :: npos );
        strMap.replace( nAt, strlen( "source=\"../tilesets/ground.tsx\"" ), "source='..\\tilesets\\ground.tsx'" );

        fsFixture.fs.files["maps/level.tmx"]      = ToBytes( strMap.c_str() );
        fsFixture.fs.files["tilesets/ground.tsx"] = ToBytes( g_szTileset );

        CHECK( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( rig.engineFixture.engine.strLastLoadTextureFileName == "tilesets/ground.png" );
    }

    TEST_CASE( "A missing external tileset fails LoadMap cleanly, and the renderer loads a good map afterwards" )  {

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;

        fsFixture.fs.files["maps/level.tmx"] = ToBytes( g_szMap );      // no tilesets/ground.tsx anywhere

        CHECK( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == false );

        fsFixture.fs.files["tilesets/ground.tsx"] = ToBytes( g_szTileset );

        CHECK( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
    }

    TEST_CASE( "UnloadMap releases the external tileset's texture, and the map can be loaded again" )  {

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;

        fsFixture.fs.files["maps/level.tmx"]      = ToBytes( g_szMap );
        fsFixture.fs.files["tilesets/ground.tsx"] = ToBytes( g_szTileset );

        REQUIRE( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( rig.engineFixture.engine.nUnloadTextureCalls == 0 );

        CHECK( rig.pRenderer -> UnloadMap() == true );
        CHECK( rig.engineFixture.engine.nUnloadTextureCalls == 1 );     // the tileset image went with it

        REQUIRE( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( rig.engineFixture.engine.nLoadTextureCalls == 2 );
    }

    TEST_CASE( "A map with only embedded tilesets still loads exactly as before" )  {

        MemoryFileSystemFixture  fsFixture;
        Rig                      rig;

        fsFixture.fs.files["maps/tiny.tmx"] = MakeSquareTmx( 4, 16 );

        REQUIRE( rig.pRenderer -> LoadMap( "maps/tiny.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( rig.engineFixture.engine.nLoadTextureCalls == 0 );
    }

    TEST_CASE( "The production case: a packed .zip with NO loose copy on disk, behind a read filter (an encrypted pack)" )  {

        // The files are stored XOR-scrambled in a real .zip; the read filter unscrambles every read. A raw
        // OS open of the .tsx could never do either: not find it (it is only in the archive) and not
        // decode it. Only a read through SunLight::FileSystem can.
        namespace fs = std :: filesystem;

        fs :: path  scratch = fs :: temp_directory_path() / "sunlight_extileset_test";
        fs :: path  zipPath = fs :: temp_directory_path() / "sunlight_extileset_test.zip";

        fs :: remove_all( scratch );
        fs :: remove( zipPath );
        fs :: create_directories( scratch / "maps" );
        fs :: create_directories( scratch / "tilesets" );

        auto  writeScrambled = [&]( const fs :: path &file, const char *szContent )  {
            std :: string  str( szContent );

            for( char &c : str )
                c = ( char ) ( c ^ 0x5A );

            std :: ofstream( file, std :: ios :: binary ) << str;
        };

        writeScrambled( scratch / "maps" / "level.tmx", g_szMap );
        writeScrambled( scratch / "tilesets" / "ground.tsx", g_szTileset );

        std :: string  strZipCmd = "cmake -E chdir \"" + scratch.string() + "\" cmake -E tar cf \"" + zipPath.string() + "\" --format=zip .";

        REQUIRE( std :: system( strZipCmd.c_str() ) == 0 );
        fs :: remove_all( scratch );                                    // NO loose copy left anywhere

        {
            SunLight :: FileSystem :: PhysFs :: PhysFsFileSystem  packFs;

            REQUIRE( packFs.Init( "sunlight_tests" ) );
            REQUIRE( packFs.Mount( zipPath.string(), "/", true ) );
            packFs.SetReadFilter( []( const std :: vector<unsigned char> &in, std :: vector<unsigned char> &out )  {
                out = in;

                for( unsigned char &c : out )
                    c = ( unsigned char ) ( c ^ 0x5A );

                return true;
            } );

            SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( &packFs );

            {
                Rig  rig;

                CHECK( rig.pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
                CHECK( rig.engineFixture.engine.strLastLoadTextureFileName == "tilesets/ground.png" );
            }

            SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( nullptr );
            packFs.Shutdown();
        }

        fs :: remove( zipPath );
    }
}
