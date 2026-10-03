/*
 * Tile lookup against a tileset (A6 of the master plan, companion to
 * test_tilelookup_bounds.cpp): the two reads that index libtmx's tiles[] table, which
 * holds tilecount entries, were not bounded. A gid past that table has no tile:
 *   - GetTile must answer "no tile" for it, not read tiles[gid];
 *   - the tile-animation path must not read tiles[] for an animation frame's gid, and a
 *     frame with no tile keeps the base tile on screen.
 * Drawn through the renderer with MockEngine, so the event log says what was drawn.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"
#include "mock_clock.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap       ITM;
typedef SunLight :: TileMap :: stMatrixPosition  MatrixPos;
typedef SunLight :: TileMap :: stLayer           Layer;
typedef SunLight :: TileMap :: stTile            Tile;
typedef MockEngine :: Event                      Event;

namespace  {

    // A 4 x 4 map of 16 px tiles, one tileset with 2 tiles (gids 1 and 2 are valid; the
    // tiles[] table has 2 entries):
    //   tile 0 of the tileset is ANIMATED: frame 0 shows gid 1 for 100 ms, frame 1 shows
    //   tileid 9 (gid 10 - past the table) for 100 ms.
    //   layer "ground" is all gid 1 (16 cells, the animated tile).
    //   layer "junk"   is all gid 7 (past the table: no tile exists for it).
    Bytes MakeTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"2\" columns=\"2\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"16\"/>"
            << "<tile id=\"0\"><animation>"
            << "<frame tileid=\"0\" duration=\"100\"/>"
            << "<frame tileid=\"9\" duration=\"100\"/>"
            << "</animation></tile>"
            << "</tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">"
            << "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
            << "<layer id=\"2\" name=\"junk\" width=\"4\" height=\"4\"><data encoding=\"csv\">"
            << "7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7</data></layer>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // Mocks and clock installed first (the clock must be in place before anything
    // timestamps against it), then a renderer over the map above.
    struct Scene  {

        MockClockFixture                    clockFixture;
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/tiles.tmx"] = MakeTmx();

            RendererConfig  config;

            config.fWidth  = 1260.0f;
            config.fHeight = 920.0f;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/tiles.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        int Count( Event :: Kind kind )  {
            int  nCount = 0;

            for( const Event &evt : engine().events )
                if( evt.kind == kind )
                    nCount++;

            return nCount;
        }
    };
}

TEST_SUITE( "renderer/tile lookup against a tileset" )  {

    TEST_CASE( "GetTile answers 'no tile' for a gid past the tileset, without reading tiles[]" )  {

        Scene  scene;

        Layer  junk;
        REQUIRE( scene.pRenderer -> GetLayer( "junk", junk ) == true );

        MatrixPos  pos = { 0, 0 };
        Tile       tile;

        // tiles[] has 2 entries; gid 7 used to be read as tiles[7] - past the end.
        CHECK( scene.pRenderer -> GetTile( pos, junk, tile ) == false );
        CHECK( tile.nGID == 0 );
        CHECK( tile.pTile == nullptr );
    }

    TEST_CASE( "A frame past the tileset draws nothing for the junk layer, and the rest of the frame is unchanged" )  {

        Scene  scene;

        // "ground" draws its 16 cells; "junk" (gid 7 everywhere) draws none.
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 16 );
    }

    TEST_CASE( "The animation's frame past the tileset keeps the base tile on screen, frame after frame" )  {

        Scene  scene;

        // t = 0: frame 0 (gid 1, valid). The animation starts here.
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 16 );

        // t = 100: the animation moves to frame 1 - tileid 9, gid 10, past the table.
        // That frame has no tile, so the base tile is drawn and nothing is read past tiles[].
        scene.clockFixture.clock.Advance( 100 );
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 16 );

        // t = 150: still inside frame 1 - the else branch with no tile to show.
        scene.clockFixture.clock.Advance( 50 );
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 16 );

        // t = 200: the animation wraps back to frame 0 (gid 1), and draws its tile again.
        scene.clockFixture.clock.Advance( 50 );
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 16 );
    }
}
