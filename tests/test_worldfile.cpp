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
 * Tiled .world files (I19f of the master plan): WorldFile::Read on its own (the in-memory FileSystem),
 * then IWorld::LoadWorld through a renderer - all or nothing.
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
#include "renderer/world/worldfile.h"
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef std :: shared_ptr<SunLight :: TileMap :: IWorld>  WorldHandle;

namespace  {

    Bytes ToBytes( const std :: string& str )  {
        return Bytes( str.begin(), str.end() );
    }

    // A 2 x 2 orthogonal map of 16 px tiles (32 x 32 pixels), one layer.
    const char  *g_szSmallMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"2\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"t.png\" width=\"16\" height=\"16\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,1,1,1</data></layer>"
        "</map>";

    // Tiled's own layout: maps by file name relative to the .world file, pixels, width/height it computed.
    const char  *g_szWorld =
        "{ \"maps\": [ { \"fileName\": \"west.tmx\", \"x\": -32, \"y\": 0, \"width\": 32, \"height\": 32 },"
        "              { \"fileName\": \"sub/east.tmx\", \"x\": 0, \"y\": 16, \"width\": 32, \"height\": 32 } ],"
        "  \"onlyShowAdjacentMaps\": false, \"type\": \"world\" }";

    struct Files  {
        MemoryFileSystemFixture  fsFixture;

        Files( void )  {
            fsFixture.fs.files["levels/one.world"]     = ToBytes( g_szWorld );
            fsFixture.fs.files["levels/west.tmx"]      = ToBytes( g_szSmallMap );
            fsFixture.fs.files["levels/sub/east.tmx"]  = ToBytes( g_szSmallMap );
            fsFixture.fs.files["levels/broken.world"]  = ToBytes( "{ \"maps\": [ " );
            fsFixture.fs.files["levels/array.world"]   = ToBytes( "[ 1, 2 ]" );
            fsFixture.fs.files["levels/nomaps.world"]  = ToBytes( "{ \"type\": \"world\" }" );
            fsFixture.fs.files["levels/badmap.world"]  = ToBytes( "{ \"maps\": [ { \"fileName\": \"west.tmx\", \"x\": \"10\", \"y\": 0 } ] }" );
            fsFixture.fs.files["levels/pattern.world"] = ToBytes( "{ \"patterns\": [ { \"regexp\": \"m_(\\\\d+)_(\\\\d+)\\\\.tmx\" } ] }" );
            fsFixture.fs.files["levels/nopattern.world"] = ToBytes( "{ \"maps\": [ { \"fileName\": \"west.tmx\", \"x\": 5, \"y\": 6 } ], \"patterns\": [] }" );
            fsFixture.fs.files["levels/missing.world"] = ToBytes( "{ \"maps\": [ { \"fileName\": \"west.tmx\", \"x\": 0, \"y\": 0 },"
                                                                 " { \"fileName\": \"nowhere.tmx\", \"x\": 32, \"y\": 0 } ] }" );
        }
    };
}

TEST_SUITE( "renderer/worldfile" )  {

    TEST_CASE( "WorldFile::Read: Tiled's maps list, paths resolved against the .world file's folder" )  {

        Files                           files;
        std :: vector<stWorldFileMap>  maps;
        std :: string                  strError;

        REQUIRE( WorldFile :: Read( "levels/one.world", maps, strError ) == true );
        REQUIRE( maps.size() == 2 );
        CHECK( maps[0].strPath == "levels/west.tmx" );
        CHECK( maps[0].nX == -32 );
        CHECK( maps[0].nY == 0 );
        CHECK( maps[1].strPath == "levels/sub/east.tmx" );
        CHECK( maps[1].nX == 0 );
        CHECK( maps[1].nY == 16 );

        // A path scheme carries over to the maps.
        REQUIRE( WorldFile :: Read( "vpath://levels/one.world", maps, strError ) == true );
        CHECK( maps[0].strPath == "vpath://levels/west.tmx" );

        // An empty "patterns" list is fine.
        REQUIRE( WorldFile :: Read( "levels/nopattern.world", maps, strError ) == true );
        CHECK( maps.size() == 1 );
    }

    TEST_CASE( "WorldFile::Read refuses what it cannot use, saying why: unreadable, not JSON, not an object, no maps, a bad entry, patterns" )  {

        Files                           files;
        std :: vector<stWorldFileMap>  maps;

        for( const char* szPath : { "levels/absent.world", "levels/broken.world", "levels/array.world", "levels/nomaps.world",
                                    "levels/badmap.world", "levels/pattern.world" } )  {
            std :: string  strError;

            INFO( szPath );
            CHECK( WorldFile :: Read( szPath, maps, strError ) == false );
            CHECK( !strError.empty() );
            CHECK( maps.empty() );
        }

        std :: string  strError;

        REQUIRE( WorldFile :: Read( "levels/pattern.world", maps, strError ) == false );
        CHECK( strError.find( "patterns" ) != std :: string :: npos );
    }

    TEST_CASE( "IWorld::LoadWorld: the world becomes the file's maps at their offsets, the first map's layer ids the world's" )  {

        Files               files;
        MockEngineFixture   engineFixture;
        MockWindowFixture   windowFixture;
        RendererConfig      config;
        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );

        WorldHandle  pWorld = pRenderer -> CreateWorld();

        CHECK( pWorld -> LoadWorld( "levels/one.world" ) == false );         // not started yet
        REQUIRE( pRenderer -> Start() == true );

        REQUIRE( pWorld -> AddMap( "levels/west.tmx", 500, 500 ) >= 0 );    // a map it had: replaced
        REQUIRE( pWorld -> LoadWorld( "levels/one.world" ) == true );
        CHECK( pWorld -> GetMapCount() == 2 );

        SunLight :: Base :: stDimension2D  bounds {};

        REQUIRE( pWorld -> GetBounds( bounds ) == true );
        CHECK( bounds.pos.x == -32 );
        CHECK( bounds.pos.y == 0 );
        CHECK( bounds.size.nWidth == 64 );                                   // -32 .. 32
        CHECK( bounds.size.nHeight == 48 );                                  // 0 .. 48
        CHECK( pWorld -> GetLayerId( "ground" ) == 1 );

        pRenderer -> Stop();
    }

    TEST_CASE( "IWorld::LoadWorld is all or nothing: a missing map, a bad file or a removed world leave the world as it was" )  {

        Files               files;
        MockEngineFixture   engineFixture;
        MockWindowFixture   windowFixture;
        RendererConfig      config;
        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        WorldHandle  pWorld = pRenderer -> CreateWorld();
        int          nKept  = pWorld -> AddMap( "levels/west.tmx", 7, 9 );

        REQUIRE( nKept >= 0 );

        int  nUnloadsBefore = engineFixture.engine.nUnloadTextureCalls;

        CHECK( pWorld -> LoadWorld( "levels/missing.world" ) == false );     // its first map loads, the second does not
        CHECK( engineFixture.engine.nUnloadTextureCalls == nUnloadsBefore + 1 );   // the one it loaded, freed again
        CHECK( pWorld -> LoadWorld( "levels/broken.world" ) == false );
        CHECK( pWorld -> LoadWorld( "levels/pattern.world" ) == false );

        int  nX = 0, nY = 0;

        CHECK( pWorld -> GetMapCount() == 1 );                               // the map it had, where it was
        REQUIRE( pWorld -> GetMapOffset( nKept, nX, nY ) == true );
        CHECK( nX == 7 );
        CHECK( nY == 9 );

        REQUIRE( pRenderer -> RemoveWorld( pWorld ) == true );
        CHECK( pWorld -> LoadWorld( "levels/one.world" ) == false );

        pRenderer -> Stop();
    }
}
