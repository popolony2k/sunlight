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
 * I11 of the master plan: an animated tile shows the frame for the time since the map loaded,
 * modulo its cycle, as Tiled animates - whether or not it was drawn, with no drift - and
 * GetTileFrame / GetObjectTileFrame read that frame without changing it. Asked by Scarab (its
 * V13), to test the animation without a window.
 *
 * The tile: gid 1, frames gid 1, 2, 3 for 100, 200 and 300 ms - a cycle of 600 ms: gid 1 in
 * [0, 100), gid 2 in [100, 300), gid 3 in [300, 600).
 */

#include <doctest/doctest.h>
#include <memory>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "mock_clock.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    const int           __TILE_SIZE    = 32;
    const unsigned int  __ANIMATED_GID = 1;
    const unsigned int  __SECOND_GID   = 2;
    const unsigned int  __THIRD_GID    = 3;
    const unsigned int  __STATIC_GID   = 4;
    const int           __GROUND_ID    = 1;
    const int           __FLIPPED_ID   = 2;
    const int           __THINGS_ID    = 3;
    const int           __SPECIAL_ID   = 4;
    const unsigned int  __ZERO_FIRST   = 5;     // tile 4: frames tile 4, tile 5, both 0 ms
    const unsigned int  __MIXED_FIRST  = 7;     // tile 6: frames tile 6 (100 ms), tile 7 (0 ms), tile 5 (100 ms)
    const unsigned int  __MIXED_LAST   = 6;

    // "ground": the animated tile, a static one and an empty cell. "flipped": the animated tile
    // flipped horizontally (gid 1 | 0x80000000), not visible. "things": the animated tile as a tile
    // object, a flipped one, and a rectangle.
    const char * const  __TMX =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"3\" height=\"1\""
        " tilewidth=\"32\" tileheight=\"32\" nextlayerid=\"5\" nextobjectid=\"4\">"
        "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"8\" columns=\"8\">"
        "<image source=\"tiles.png\" width=\"256\" height=\"32\"/>"
        "<tile id=\"0\"><animation>"
        "<frame tileid=\"0\" duration=\"100\"/><frame tileid=\"1\" duration=\"200\"/><frame tileid=\"2\" duration=\"300\"/>"
        "</animation></tile>"
        "<tile id=\"4\"><animation>"
        "<frame tileid=\"4\" duration=\"0\"/><frame tileid=\"5\" duration=\"0\"/>"
        "</animation></tile>"
        "<tile id=\"6\"><animation>"
        "<frame tileid=\"6\" duration=\"100\"/><frame tileid=\"7\" duration=\"0\"/><frame tileid=\"5\" duration=\"100\"/>"
        "</animation></tile></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"3\" height=\"1\"><data encoding=\"csv\">1,4,0</data></layer>"
        "<layer id=\"2\" name=\"flipped\" width=\"3\" height=\"1\" visible=\"0\"><data encoding=\"csv\">2147483649,0,0</data></layer>"
        "<layer id=\"4\" name=\"special\" width=\"3\" height=\"1\" visible=\"0\"><data encoding=\"csv\">5,7,0</data></layer>"
        "<objectgroup id=\"3\" name=\"things\">"
        "<object id=\"1\" name=\"torch\" gid=\"1\" x=\"0\" y=\"96\" width=\"32\" height=\"32\"/>"
        "<object id=\"2\" name=\"mirror\" gid=\"2147483649\" x=\"64\" y=\"96\" width=\"32\" height=\"32\"/>"
        "<object id=\"3\" name=\"box\" x=\"0\" y=\"128\" width=\"16\" height=\"16\"/>"
        "</objectgroup>"
        "</map>";

    // A started renderer over the map, with a clock that only moves when told (installed first:
    // the map's load is the animation's time 0).
    struct Scene  {

        MockClockFixture                    clockFixture;
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( bool bLoadMap = true )  {
            std :: string  str( __TMX );

            fsFixture.fs.files["maps/frames.tmx"] = Bytes( str.begin(), str.end() );

            RendererConfig                     config;
            SunLight :: Base :: stDimension2D  viewport {};

            viewport.size.nWidth  = 1000;
            viewport.size.nHeight = 800;
            config.fWidth         = 1260.0f;
            config.fHeight        = 920.0f;
            config.viewport       = viewport;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );

            if( bLoadMap )
                REQUIRE( pRenderer -> LoadMap( "maps/frames.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        void At( int64_t nMillis )  {
            clockFixture.clock.Advance( nMillis - clockFixture.clock.NowMilliseconds() );
        }

        unsigned int CellFrame( int nLayerId = __GROUND_ID, int nCol = 0 )  {
            unsigned int  nGID = 0;

            REQUIRE( pRenderer -> GetTileFrame( nLayerId, { 0, nCol }, nGID ) == true );

            return nGID;
        }

        unsigned int ObjectFrame( const char* szName = "torch" )  {
            unsigned int  nGID = 0;

            REQUIRE( pRenderer -> GetObjectTileFrame( szName, nGID ) == true );

            return nGID;
        }

        void RunFrame( void )  {
            engineFixture.engine.events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = 1;
            pRenderer -> Run();
        }

        // Draws one frame; the gid the ground's animated cell was drawn with (its source x / tile size + 1).
        unsigned int DrawnFrame( void )  {
            RunFrame();

            for( const Event& evt : engineFixture.engine.events )
                if( ( evt.kind == Event :: TILE ) && ( evt.y == 0.0f ) && ( evt.x == 0.0f ) )
                    return ( unsigned int ) ( evt.srcX / __TILE_SIZE ) + 1;

            FAIL( "the animated cell was not drawn" );
            return 0;
        }

        void SetGroundVisible( bool bVisible )  {
            SunLight :: TileMap :: stLayer  layer {};

            REQUIRE( pRenderer -> GetLayer( __GROUND_ID, layer ) == true );
            layer.bVisible = bVisible;
            REQUIRE( pRenderer -> SetLayer( __GROUND_ID, layer ) == true );
        }
    };
}

TEST_SUITE( "renderer/tile_frame" )  {

    TEST_CASE( "An animated cell's frame follows the time since the map loaded, every cycle" )  {

        Scene  scene;

        struct stSample  { int64_t nMillis; unsigned int nGID; };

        const stSample  samples[] = { { 0, __ANIMATED_GID }, { 99, __ANIMATED_GID }, { 100, __SECOND_GID },
                                      { 299, __SECOND_GID }, { 300, __THIRD_GID }, { 599, __THIRD_GID },
                                      { 600, __ANIMATED_GID }, { 700, __SECOND_GID }, { 1300, __SECOND_GID },
                                      { 6000, __ANIMATED_GID } };

        for( const stSample& sample : samples )  {
            INFO( "at " << sample.nMillis << " ms" );
            scene.At( sample.nMillis );
            CHECK( scene.CellFrame() == sample.nGID );
            CHECK( scene.ObjectFrame() == sample.nGID );                 // a tile object shows the same frame
            CHECK( scene.CellFrame( __FLIPPED_ID ) == sample.nGID );     // flip bits removed
            CHECK( scene.ObjectFrame( "mirror" ) == sample.nGID );
        }
    }

    TEST_CASE( "An animation whose frames are all 0 ms shows its first frame" )  {

        // Found by Scarab (its V13): with a cycle of 0 ms the frame walk stepped past every frame
        // (0 >= 0) and showed the last one (gid 6), drawn and asked alike.
        Scene  scene;

        for( int64_t nMillis : { 0, 1, 100, 5000 } )  {
            INFO( "at " << nMillis << " ms" );
            scene.At( nMillis );
            CHECK( scene.CellFrame( __SPECIAL_ID, 0 ) == __ZERO_FIRST );
        }
    }

    TEST_CASE( "A 0 ms frame inside a longer cycle is never shown: it has no time to be shown in" )  {

        // Frames tile 6 (100 ms), tile 7 (0 ms), tile 5 (100 ms): gid 7 in [0, 100), gid 6 in [100, 200).
        Scene  scene;

        struct stSample  { int64_t nMillis; unsigned int nGID; };

        const stSample  samples[] = { { 0, __MIXED_FIRST }, { 99, __MIXED_FIRST }, { 100, __MIXED_LAST },
                                      { 199, __MIXED_LAST }, { 200, __MIXED_FIRST } };

        for( const stSample& sample : samples )  {
            INFO( "at " << sample.nMillis << " ms" );
            scene.At( sample.nMillis );
            CHECK( scene.CellFrame( __SPECIAL_ID, 1 ) == sample.nGID );
        }
    }

    TEST_CASE( "A static cell reports its own gid at any time" )  {

        Scene  scene;

        for( int64_t nMillis : { 0, 150, 450, 900 } )  {
            scene.At( nMillis );
            CHECK( scene.CellFrame( __GROUND_ID, 1 ) == __STATIC_GID );
        }
    }

    TEST_CASE( "Asking does not change the frame, and the drawn frame is the frame asked" )  {

        Scene  scene;

        scene.At( 150 );

        for( int nAsk = 0; nAsk < 10; nAsk++ )
            CHECK( scene.CellFrame() == __SECOND_GID );

        CHECK( scene.DrawnFrame() == __SECOND_GID );
        CHECK( scene.CellFrame() == __SECOND_GID );

        scene.At( 450 );
        CHECK( scene.CellFrame() == __THIRD_GID );
        CHECK( scene.DrawnFrame() == __THIRD_GID );
    }

    TEST_CASE( "A tile that is not drawn for a while does not pause: it shows the frame for the time now" )  {

        // Before I11 the animation advanced only when drawn: hidden from 0 to 300 ms, the tile came
        // back one step on (gid 2) instead of at the frame for 300 ms (gid 3).
        Scene  scene;

        CHECK( scene.DrawnFrame() == __ANIMATED_GID );
        scene.SetGroundVisible( false );
        scene.At( 300 );
        scene.RunFrame();                     // the ground is not drawn
        scene.SetGroundVisible( true );
        CHECK( scene.DrawnFrame() == __THIRD_GID );
    }

    TEST_CASE( "A frame's end does not drift with the draw that sees it" )  {

        // Before I11 a frame's end was counted from the draw that noticed the previous one was over:
        // drawn at 0 and 150 ms, the second frame then ran to 350 ms, so at 320 ms it still showed
        // gid 2. It ends at 300 ms, whenever it was drawn.
        Scene  scene;

        CHECK( scene.DrawnFrame() == __ANIMATED_GID );
        scene.At( 150 );
        CHECK( scene.DrawnFrame() == __SECOND_GID );
        scene.At( 320 );
        CHECK( scene.DrawnFrame() == __THIRD_GID );
    }

    TEST_CASE( "GetTileFrame and GetObjectTileFrame refuse what has no tile frame" )  {

        unsigned int  nGID = 0;

        {
            Scene  unloaded( false );

            CHECK( unloaded.pRenderer -> GetTileFrame( __GROUND_ID, { 0, 0 }, nGID ) == false );
            CHECK( unloaded.pRenderer -> GetObjectTileFrame( "torch", nGID ) == false );
        }

        Scene  scene;

        CHECK( scene.pRenderer -> GetTileFrame( 99, { 0, 0 }, nGID ) == false );            // no such layer
        CHECK( scene.pRenderer -> GetTileFrame( __THINGS_ID, { 0, 0 }, nGID ) == false );   // not a tile layer
        CHECK( scene.pRenderer -> GetTileFrame( __GROUND_ID, { 0, 2 }, nGID ) == false );   // an empty cell
        CHECK( scene.pRenderer -> GetTileFrame( __GROUND_ID, { 0, 3 }, nGID ) == false );   // past the last column
        CHECK( scene.pRenderer -> GetTileFrame( __GROUND_ID, { 1, 0 }, nGID ) == false );   // past the last row
        CHECK( scene.pRenderer -> GetTileFrame( __GROUND_ID, { -1, 0 }, nGID ) == false );
        CHECK( scene.pRenderer -> GetObjectTileFrame( "nope", nGID ) == false );            // no such object
        CHECK( scene.pRenderer -> GetObjectTileFrame( "box", nGID ) == false );             // not a tile object
    }
}
