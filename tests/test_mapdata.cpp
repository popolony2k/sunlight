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
 * S1c1 of the master plan: MapData on its own - loading, what it works out, its lookups, the tile
 * animation state and what destroying it frees - with no TileMapRenderer, only an in-memory
 * FileSystem, the mock engine and the mock clock. test_maploading still pins the same pipeline
 * through the renderer (reload, failed loads, texture balance).
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
#include "renderer/map/mapdata.h"
#include "mock_engine.h"
#include "mock_clock.h"
#include "mock_filesystem.h"

using SunLight :: Renderer :: MapData;

namespace  {

    // A 3 x 2 orthogonal map of 16 x 8 tiles. Tileset "t" (firstgid 1, three tiles, an image beside
    // the map): tile 0 is animated - tile 0 for 100 ms, tile 1 for 50 ms, tile 2 for 70 ms. Layers: "ground"
    // (id 1: gids 1 2 0 / 0 9 1 - gid 9 is past the tileset), a group "grp" (id 2) holding the object
    // group "marks" (id 3, object "spawn").
    const char  *g_szMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"3\" height=\"2\" tilewidth=\"16\" tileheight=\"8\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"8\" tilecount=\"3\" columns=\"3\">"
        "<image source=\"t.png\" width=\"48\" height=\"8\"/>"
        "<tile id=\"0\"><animation><frame tileid=\"0\" duration=\"100\"/><frame tileid=\"1\" duration=\"50\"/>"
        "<frame tileid=\"2\" duration=\"70\"/></animation></tile>"
        "</tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"3\" height=\"2\"><data encoding=\"csv\">1,2,0,0,9,1</data></layer>"
        "<group id=\"2\" name=\"grp\"><objectgroup id=\"3\" name=\"marks\">"
        "<object id=\"1\" name=\"spawn\" x=\"5\" y=\"6\" width=\"7\" height=\"8\"/>"
        "</objectgroup></group>"
        "</map>";

    // The same map with a second tileset whose tiles are 32 x 32: not every tileset matches the grid.
    const char  *g_szMixedMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"3\" height=\"2\" tilewidth=\"16\" tileheight=\"8\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"8\" tilecount=\"2\" columns=\"2\"><image source=\"t.png\" width=\"32\" height=\"8\"/></tileset>"
        "<tileset firstgid=\"3\" name=\"big\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\"><image source=\"big.png\" width=\"32\" height=\"32\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"3\" height=\"2\"><data encoding=\"csv\">1,2,3,0,0,1</data></layer>"
        "</map>";

    const char  *g_szTruncated = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><map version=\"1.0\" orientation=\"orthogonal\" width=\"3\"";

    Bytes ToBytes( const char* sz )  {
        std :: string  str( sz );
        return Bytes( str.begin(), str.end() );
    }

    // The in-memory files and the mock engine every test loads through.
    struct Files  {
        MemoryFileSystemFixture  fsFixture;
        MockEngineFixture        engineFixture;

        Files( void )  {
            fsFixture.fs.files["maps/level.tmx"]     = ToBytes( g_szMap );
            fsFixture.fs.files["maps/mixed.tmx"]     = ToBytes( g_szMixedMap );
            fsFixture.fs.files["maps/truncated.tmx"] = ToBytes( g_szTruncated );
        }

        MockEngine& engine( void )  { return engineFixture.engine; }
    };
}

TEST_SUITE( "renderer/MapData" )  {

    TEST_CASE( "A map that cannot be read or parsed gives no MapData" )  {

        Files  files;

        CHECK( MapData :: Load( "maps/missing.tmx" ) == nullptr );
        CHECK( MapData :: Load( "maps/truncated.tmx" ) == nullptr );
        CHECK( files.engine().nLoadTextureCalls == 0 );
    }

    TEST_CASE( "A loaded map: its size in map pixels, its grid, its scroll step" )  {

        Files                        files;
        std :: unique_ptr<MapData>   pMap = MapData :: Load( "maps/level.tmx" );

        REQUIRE( pMap != nullptr );
        REQUIRE( pMap -> Tmx() != nullptr );
        CHECK( pMap -> PixelWidth() == 3 * 16 );
        CHECK( pMap -> PixelHeight() == 2 * 8 );
        CHECK( pMap -> IsUniformTileGrid() == true );
        CHECK( pMap -> DefaultScrollStep().nWidth == 16 );
        CHECK( pMap -> DefaultScrollStep().nHeight == 8 );

        std :: unique_ptr<MapData>   pMixed = MapData :: Load( "maps/mixed.tmx" );

        REQUIRE( pMixed != nullptr );
        CHECK( pMixed -> IsUniformTileGrid() == false );
    }

    TEST_CASE( "Each tileset image is loaded once, beside the map, and destroying the MapData releases each once" )  {

        Files  files;

        {
            std :: unique_ptr<MapData>  pMap = MapData :: Load( "maps/level.tmx" );

            REQUIRE( pMap != nullptr );
            CHECK( files.engine().nLoadTextureCalls == 1 );
            CHECK( files.engine().strLastLoadTextureFileName == "maps/t.png" );
            CHECK( files.engine().nUnloadTextureCalls == 0 );
        }

        CHECK( files.engine().nUnloadTextureCalls == 1 );
    }

    TEST_CASE( "Layers are found by id and by name, inside a group too; an unknown one is not" )  {

        Files                        files;
        std :: unique_ptr<MapData>   pMap = MapData :: Load( "maps/level.tmx" );

        REQUIRE( pMap != nullptr );
        REQUIRE( pMap -> FindLayer( 1 ) != nullptr );
        CHECK( std :: string( pMap -> FindLayer( 1 ) -> name ) == "ground" );
        CHECK( pMap -> FindLayer( "marks" ) == pMap -> FindLayer( 3 ) );
        CHECK( pMap -> FindLayer( 3 ) != nullptr );
        CHECK( pMap -> FindLayer( 99 ) == nullptr );
        CHECK( pMap -> FindLayer( "nope" ) == nullptr );
    }

    TEST_CASE( "TileAt: the gid and tile at a cell; none off the map, for an empty cell, or past the tileset" )  {

        Files                          files;
        std :: unique_ptr<MapData>     pMap = MapData :: Load( "maps/level.tmx" );
        SunLight :: TileMap :: stTile  tile;

        REQUIRE( pMap != nullptr );

        tmx_layer  *pGround = pMap -> FindLayer( 1 );

        CHECK( pMap -> TileAt( { 0, 1 }, pGround, tile ) == true );       // row 0, column 1: gid 2
        CHECK( tile.nGID == 2 );
        CHECK( tile.pTile == pMap -> Tmx() -> tiles[2] );

        CHECK( pMap -> TileAt( { 0, 2 }, pGround, tile ) == false );      // gid 0
        CHECK( pMap -> TileAt( { 1, 1 }, pGround, tile ) == false );      // gid 9, past the tileset
        CHECK( tile.nGID == 0 );
        CHECK( tile.pTile == nullptr );
        CHECK( pMap -> TileAt( { 2, 0 }, pGround, tile ) == false );      // the row past the last
        CHECK( pMap -> TileAt( { 0, 3 }, pGround, tile ) == false );      // the column past the last
        CHECK( pMap -> TileAt( { -1, 0 }, pGround, tile ) == false );
    }

    TEST_CASE( "An object is found by name inside a group" )  {

        Files                            files;
        std :: unique_ptr<MapData>       pMap = MapData :: Load( "maps/level.tmx" );
        SunLight :: TileMap :: stObject  object;

        REQUIRE( pMap != nullptr );
        REQUIRE( pMap -> FindObjectByName( "spawn", object ) == true );
        CHECK( object.dimension.pos.x == 5 );
        CHECK( object.dimension.pos.y == 6 );
        CHECK( object.dimension.size.nWidth == 7 );
        CHECK( object.dimension.size.nHeight == 8 );
        CHECK( pMap -> FindObjectByName( "nope", object ) == false );
    }

    TEST_CASE( "An animated tile shows each frame once per cycle, each for its own duration" )  {

        // Frames 0, 1, 2 for 100, 50 and 70 ms, stepped at each frame's end for two full cycles. The
        // cycle used to wrap one step late and show frame 0 twice in a row (0 1 2 0 0 1 2 0 0 ...).
        Files                        files;
        MockClockFixture             clockFixture;
        std :: unique_ptr<MapData>   pMap = MapData :: Load( "maps/level.tmx" );

        REQUIRE( pMap != nullptr );

        tmx_tile  *pFrames[] = { pMap -> Tmx() -> tiles[1], pMap -> Tmx() -> tiles[2], pMap -> Tmx() -> tiles[3] };
        tmx_tile  *pBase     = pFrames[0];

        REQUIRE( pBase -> animation_len == 3 );

        std :: vector<tmx_tile*>  shown;
        const int64_t             steps[] = { 0, 100, 50, 70, 100, 50, 70 };

        for( int64_t nStep : steps )  {
            clockFixture.clock.Advance( nStep );
            shown.push_back( pMap -> AnimatedFrameOf( pBase, 1 ) );
        }

        std :: vector<tmx_tile*>  expected = { pFrames[0], pFrames[1], pFrames[2], pFrames[0], pFrames[1], pFrames[2], pFrames[0] };

        CHECK( shown == expected );

        // Between two steps the frame on screen does not change: frame 0 stays until its 100 ms are up.
        clockFixture.clock.Advance( 99 );
        CHECK( pMap -> AnimatedFrameOf( pBase, 1 ) == pFrames[0] );
        clockFixture.clock.Advance( 1 );
        CHECK( pMap -> AnimatedFrameOf( pBase, 1 ) == pFrames[1] );
    }
}
