/*
 * Map orientation (A2 of the master plan, revised by E1b): orthogonal and isometric
 * maps are drawn correctly, behind IMapProjection (E0/E1b), so LoadMap accepts them;
 * staggered and hexagonal maps are still refused instead of being loaded and drawn in
 * the wrong places. A refused map leaves the renderer with no map, and the next
 * supported map loads normally.
 */

#include <doctest/doctest.h>
#include <memory>
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

    bool HasMap( TileMapRenderer &renderer )  {

        SunLight :: TileMap :: stMapInfo  info {};

        return renderer.GetMapInfo( info );
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

    TEST_CASE( "Staggered and hexagonal maps are refused, and no map is left loaded" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/sta.tmx"]  = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "staggered" );
        fs.fs.files["maps/hex.tmx"]  = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "hexagonal" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        for( const char *szFile : { "maps/sta.tmx", "maps/hex.tmx" } )  {
            INFO( "map: " << szFile );

            CHECK( pRenderer -> LoadMap( szFile, ITM :: MAP_ALIGNMENT_TOP_LEFT ) == false );
            CHECK( HasMap( *pRenderer ) == false );
        }

        pRenderer -> Stop();
    }

    TEST_CASE( "A refused map does not get in the way of the next map" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/hex.tmx"]  = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "hexagonal" );
        fs.fs.files["maps/ortho.tmx"] = MakeSquareTmx( 4, 16, false, 0, 0, 0, 0, "orthogonal" );

        std :: unique_ptr<TileMapRenderer>  pRenderer = StartedRenderer();

        REQUIRE( pRenderer -> LoadMap( "maps/hex.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == false );
        CHECK( pRenderer -> LoadMap( "maps/ortho.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( HasMap( *pRenderer ) == true );

        pRenderer -> Stop();
    }
}
