/*
 * Tile lookup bounds (A6 of the master plan): the collision lookup turns a sprite's
 * position into a tile row/column (TileMapToTileMatrix) and GetTile indexes the
 * layer's gids[] with it. The row/column used to be checked against zero only, so a
 * position past the far edge of the map - which the viewport origin and a camera
 * offset can produce from a sprite that is still on the map - indexed past the end
 * of gids[]. Both now refuse anything outside the map.
 */

#include <doctest/doctest.h>
#include <memory>
#include "renderer/tilemaprenderer.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef SunLight :: Base :: stCoordinate2D    Coord;
typedef SunLight :: TileMap :: stMatrixPosition  MatrixPos;
typedef SunLight :: TileMap :: stLayer           Layer;
typedef SunLight :: TileMap :: stTile            Tile;

namespace  {

    // The viewport origin is not zero on purpose: that is what pushed the lookup
    // past the far edge in the first place (the origin is added to the coordinate).
    std :: unique_ptr<TileMapRenderer> LoadedRenderer( void )  {

        RendererConfig  config;

        config.backend     = RENDERER_BACKEND_NULL;
        config.framePacing = FRAME_PACING_UNLIMITED;
        config.nMaxFrames  = 1;
        config.viewport.emplace();
        config.viewport -> pos.x       = 10;
        config.viewport -> pos.y       = 10;
        config.viewport -> size.nWidth  = 100;
        config.viewport -> size.nHeight = 100;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        // A 4 x 4 map of 16 px tiles: 64 x 64 px, gids[] holds 16 entries.
        REQUIRE( pRenderer -> LoadMap( "maps/bounds.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        return pRenderer;
    }

    bool Lookup( TileMapRenderer &renderer, int nX, int nY, MatrixPos &pos )  {

        Coord  coord = { nX, nY };

        pos = { -99, -99 };

        return renderer.TileMapToTileMatrix( coord, pos );
    }
}

TEST_SUITE( "renderer/tile lookup bounds" )  {

    TEST_CASE( "A position inside the map is found (the control)" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/bounds.tmx"] = MakeSquareTmx( 4, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = LoadedRenderer();
        MatrixPos                           pos;

        pRenderer -> SetCameraPosition( 0, 0 );

        REQUIRE( Lookup( *pRenderer, 0, 0, pos ) == true );
        CHECK( pos.nTileRow == 0 );
        CHECK( pos.nTileCol == 0 );

        // (48 + 10) / 16 = 3: the last column is still on the map.
        REQUIRE( Lookup( *pRenderer, 48, 48, pos ) == true );
        CHECK( pos.nTileRow == 3 );
        CHECK( pos.nTileCol == 3 );

        pRenderer -> Stop();
    }

    TEST_CASE( "A sprite in the strip past the last column is refused, not wrapped into the next row" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/bounds.tmx"] = MakeSquareTmx( 4, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = LoadedRenderer();
        MatrixPos                           pos;

        pRenderer -> SetCameraPosition( 0, 0 );

        // (56 + 10) / 16 = 4: column 4 does not exist. The old lookup returned true
        // here, and GetTile read gids[row * 4 + 4] - the next row's first entry.
        CHECK( Lookup( *pRenderer, 56, 0, pos ) == false );

        pRenderer -> Stop();
    }

    TEST_CASE( "A sprite in the strip past the last row is refused (the read ran past gids[])" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/bounds.tmx"] = MakeSquareTmx( 4, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = LoadedRenderer();
        MatrixPos                           pos;

        pRenderer -> SetCameraPosition( 0, 0 );

        // The A6 case: (58 + 10) / 16 = 4 on both axes, so gids[4 * 4 + 4] = gids[20]
        // - past the 16 entries. ASan showed this as a heap-buffer-overflow read.
        CHECK( Lookup( *pRenderer, 58, 58, pos ) == false );

        pRenderer -> Stop();
    }

    TEST_CASE( "A camera offset that carries a sprite past the far edge is refused" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/bounds.tmx"] = MakeSquareTmx( 4, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = LoadedRenderer();
        MatrixPos                           pos;

        // Sprite at x = 40 is on the map (column 3 with no offset) ...
        pRenderer -> SetCameraPosition( 0, 0 );
        REQUIRE( Lookup( *pRenderer, 40, 0, pos ) == true );
        CHECK( pos.nTileCol == 3 );

        // ... but with the camera 16 px along, (40 + 10 + 16) / 16 = 4: past the edge.
        pRenderer -> SetCameraPosition( 16, 0 );
        CHECK( Lookup( *pRenderer, 40, 0, pos ) == false );

        pRenderer -> Stop();
    }

    TEST_CASE( "GetTile reads no gid for a row or column off the map, and says so" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/bounds.tmx"] = MakeSquareTmx( 4, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = LoadedRenderer();

        Layer  layer;
        REQUIRE( pRenderer -> GetLayer( "ground", layer ) == true );

        // Each of these used to index gids[] with a row or column outside 0..3.
        const MatrixPos  outside[] = { { 4, 0 }, { 0, 4 }, { 4, 4 }, { -1, 0 }, { 0, -1 }, { 3, 4 } };

        for( const MatrixPos &pos : outside )  {
            Tile  tile;

            tile.nGID  = 12345;
            tile.pTile = reinterpret_cast<tmx_tile*>( 0x1 );   // sentinel: must be overwritten

            CHECK( pRenderer -> GetTile( pos, layer, tile ) == false );
            CHECK( tile.nGID == 0 );
            CHECK( tile.pTile == nullptr );
        }

        pRenderer -> Stop();
    }

    TEST_CASE( "GetTile on a position inside the map still answers (an empty tile, false)" )  {

        MemoryFileSystemFixture  fs;

        fs.fs.files["maps/bounds.tmx"] = MakeSquareTmx( 4, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = LoadedRenderer();

        Layer  layer;
        REQUIRE( pRenderer -> GetLayer( "ground", layer ) == true );

        // MakeSquareTmx leaves every gid 0, so there is no tile to report - but the
        // read is legal, and it is the same answer GetTile always gave for gid 0.
        MatrixPos  pos = { 3, 3 };
        Tile       tile;

        CHECK( pRenderer -> GetTile( pos, layer, tile ) == false );
        CHECK( tile.nGID == 0 );

        pRenderer -> Stop();
    }
}
