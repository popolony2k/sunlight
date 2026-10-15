/*
 * Map orientation (A2 of the master plan, revised by E1b, E2b and E3b): orthogonal,
 * isometric, staggered and hexagonal maps are all drawn correctly, behind IMapProjection,
 * so LoadMap accepts every orientation string libtmx itself recognizes. A map missing the
 * "orientation" attribute entirely is refused too, but by libtmx's own parser, not by
 * TileMapRenderer's own projection switch - checked against libtmx's source (tmx_xml.c):
 * a missing attribute and an unrecognized one are both rejected at parse time, so LoadMap's
 * own orientation switch never sees an orientation it does not recognize (the same finding
 * B3 made for L_NONE/OT_NONE - the switch's own default case is unreachable in practice).
 * A refused map leaves the renderer with no map, and the next supported map loads normally.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    std :: unique_ptr<TileMapRenderer> StartedRenderer( void )  {

        RendererConfig  config;

        config.backend     = RENDERER_BACKEND_NULL;
        config.framePacing = FRAME_PACING_UNLIMITED;
        config.nMaxFrames  = 1;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        return pRenderer;
    }

    bool HasMap( TileMapRenderer& renderer )  {

        SunLight :: TileMap :: stMapInfo  info {};

        return renderer.GetMapInfo( info );
    }

    // A 4x4 orthogonal-sized map with no "orientation" attribute at all - libtmx's own parser
    // rejects this (tmx_xml.c: a missing "orientation" attribute is its own explicit error,
    // separate from an unrecognized one), so LoadMap fails the same way it already does for any
    // other unparseable map - the same path "cannot open extern tileset" exercises elsewhere.
    Bytes MakeNoOrientationTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" renderorder=\"right-down\" width=\"4\""
            << " height=\"4\" tilewidth=\"16\" tileheight=\"16\">"
            << "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">"
            << "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }
}

TEST_SUITE( "renderer/map orientation" )  {

    TEST_CASE( "An orthogonal map loads (the control)" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/ortho.tmx"] = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "orthogonal" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        CHECK( pRenderer -> LoadMap( "maps/ortho.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( HasMap( *pRenderer ) == true );

        pRenderer -> Stop();
    }

    TEST_CASE( "An isometric map loads (E1b)" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/iso.tmx"] = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "isometric" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        CHECK( pRenderer -> LoadMap( "maps/iso.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( HasMap( *pRenderer ) == true );

        pRenderer -> Stop();
    }

    TEST_CASE( "A staggered map loads (E2b)" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/sta.tmx"] = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "staggered" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        CHECK( pRenderer -> LoadMap( "maps/sta.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( HasMap( *pRenderer ) == true );

        pRenderer -> Stop();
    }

    TEST_CASE( "A hexagonal map loads (E3b)" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/hex.tmx"] = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "hexagonal" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        CHECK( pRenderer -> LoadMap( "maps/hex.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( HasMap( *pRenderer ) == true );

        pRenderer -> Stop();
    }

    TEST_CASE( "A map with no orientation attribute is refused, and no map is left loaded" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/none.tmx"] = MakeNoOrientationTmx();

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        CHECK( pRenderer -> LoadMap( "maps/none.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == false );
        CHECK( HasMap( *pRenderer ) == false );

        pRenderer -> Stop();
    }

    TEST_CASE( "A refused map does not get in the way of the next map" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/none.tmx"]  = MakeNoOrientationTmx();
        fs.fs.files["maps/ortho.tmx"] = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "orthogonal" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        REQUIRE( pRenderer -> LoadMap( "maps/none.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == false );
        CHECK( pRenderer -> LoadMap( "maps/ortho.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( HasMap( *pRenderer ) == true );

        pRenderer -> Stop();
    }
}
