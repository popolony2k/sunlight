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
 * Sprite-against-tile collisions (I7 of the master plan), through a real renderer and map: worked
 * out in MAP space, so the viewport origin, the camera and the zoom do not move the window in which a
 * sprite collides, a world-space sprite collides with the tile it is on, every tile a sprite overlaps
 * is reported, and the reported tile's rectangle is its map rectangle. Found by Scarab (its V11),
 * whose sweep these tests follow: a 32 x 32 sprite moved along x past a 32 x 32 collision tile at map
 * pixels 160..191 collides for x = 129..191 (half-open overlap: sharing an edge is not touching).
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    const int  __TILE        = 32;
    const int  __CELLS       = 10;
    const int  __WALL_ROW    = 5;
    const int  __WALL_COL    = 5;
    const int  __GROUND_ID   = 1;       // the tile layer
    const int  __SPRITES_ID  = 2;       // the object layer the sprite is registered on
    const int  __WALL_MIN_X  = 129;     // the sprite's x range overlapping map pixels 160..191
    const int  __WALL_MAX_X  = 191;

    // A map of __CELLS x __CELLS tiles of __TILE px, orientation as given, with the wall tile (tile id 0,
    // gid 1, a full-tile collision rectangle) at every cell listed in walls.
    Bytes WallMap( const std :: vector<std :: pair<int, int>>& walls, const char* szOrientation = "orthogonal" )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"" << szOrientation << "\" renderorder=\"right-down\" width=\"" << __CELLS
            << "\" height=\"" << __CELLS << "\" tilewidth=\"" << __TILE << "\" tileheight=\"" << __TILE << "\">"
            << "<tileset firstgid=\"1\" name=\"walls\" tilewidth=\"" << __TILE << "\" tileheight=\"" << __TILE
            << "\" tilecount=\"1\" columns=\"1\"><image source=\"wall.png\" width=\"" << __TILE << "\" height=\"" << __TILE << "\"/>"
            << "<tile id=\"0\"><objectgroup draworder=\"index\"><object id=\"1\" x=\"0\" y=\"0\" width=\"" << __TILE
            << "\" height=\"" << __TILE << "\"/></objectgroup></tile></tileset>"
            << "<layer id=\"" << __GROUND_ID << "\" name=\"ground\" width=\"" << __CELLS << "\" height=\"" << __CELLS
            << "\"><data encoding=\"csv\">";

        for( int nRow = 0; nRow < __CELLS; nRow++ )  {
            for( int nCol = 0; nCol < __CELLS; nCol++ )  {
                bool  bWall = false;

                for( const auto& wall : walls )
                    bWall = bWall || ( ( wall.first == nRow ) && ( wall.second == nCol ) );

                tmx << ( ( nRow || nCol ) ? "," : "" ) << ( bWall ? 1 : 0 );
            }
        }

        tmx << "</data></layer><objectgroup id=\"" << __SPRITES_ID << "\" name=\"sprites\"/></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    struct TileHits : public SunLight :: Collision :: ICollisionListener  {
        std :: vector<SunLight :: TileMap :: stTile>  tiles;

        void OnCollision( SunLight :: Collision :: Collider*, SunLight :: Collision :: Collider* ) override  {}
        void OnCollision( SunLight :: Collision :: Collider*, SunLight :: TileMap :: stTile* pTile ) override  { tiles.push_back( *pTile ); }
    };

    // A started renderer (viewport origin (10, 10), not zero on purpose) over a wall map, with a 32 x 32
    // sprite on the object layer and a rule between it and the tile layer.
    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;
        SunLight :: Sprite :: Sprite        sprite;
        TileHits                            hits;

        Scene( const std :: vector<std :: pair<int, int>>& walls, bool bWorldSpace, const char* szOrientation = "orthogonal" )  {

            fsFixture.fs.files["maps/walls.tmx"] = WallMap( walls, szOrientation );

            RendererConfig  config;

            config.fWidth  = 1260.0f;
            config.fHeight = 920.0f;
            config.viewport.emplace();
            config.viewport -> pos.x        = 10;
            config.viewport -> pos.y        = 10;
            config.viewport -> size.nWidth  = 320;
            config.viewport -> size.nHeight = 320;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/walls.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

            sprite.SetWorldSpace( bWorldSpace );
            sprite.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { __TILE, __TILE } } );

            REQUIRE( pRenderer -> AddSprite( __SPRITES_ID, sprite ) == true );
            REQUIRE( pRenderer -> GetCollisionManager().AddColliderToTileRule( __SPRITES_ID, __GROUND_ID ) == true );
            pRenderer -> GetCollisionManager().AddCollisionListener( &hits );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        // The tiles hit in one frame with the sprite at (nX, nY).
        std :: vector<SunLight :: TileMap :: stTile> HitsAt( int nX, int nY )  {
            sprite.SetDimension2D( SunLight :: Base :: stDimension2D { { nX, nY }, { __TILE, __TILE } } );
            hits.tiles.clear();
            windowFixture.window.nEndFrameCalls          = 0;      // the mock closes after this many frames from 0
            windowFixture.window.nFramesUntilShouldClose = 1;
            pRenderer -> Run();

            return hits.tiles;
        }

        // The first and last x in [nFrom, nTo] at which the sprite, at that x and nY, hits the wall:
        // { -1, -1 } when it never does.
        std :: pair<int, int> Window( int nFrom, int nTo, int nY )  {
            std :: pair<int, int>  window { -1, -1 };

            for( int nX = nFrom; nX <= nTo; nX++ )  {
                if( !HitsAt( nX, nY ).empty() )  {
                    if( window.first < 0 )
                        window.first = nX;
                    window.second = nX;
                }
            }

            return window;
        }
    };

    const std :: vector<std :: pair<int, int>>  __ONE_WALL  = { { __WALL_ROW, __WALL_COL } };
    const int                                    __WALL_Y    = __WALL_ROW * __TILE;     // 160
    const std :: pair<int, int>                  __EXPECTED { __WALL_MIN_X, __WALL_MAX_X };
}

TEST_SUITE( "collision/tiles in map space" )  {

    TEST_CASE( "A view-relative sprite collides over exactly the tile's width plus its own, whatever the viewport origin" )  {

        Scene  scene( __ONE_WALL, false );

        scene.pRenderer -> SetCameraPosition( 0, 0 );

        // Scarab measured 150..180: the top-left corner alone, shifted by the viewport origin (10 px).
        CHECK( scene.Window( 100, 220, __WALL_Y ) == __EXPECTED );
    }

    TEST_CASE( "With the camera moved, a view-relative sprite collides where the tile is drawn in the view" )  {

        const int  nCamera = 64;
        Scene      scene( __ONE_WALL, false );

        scene.pRenderer -> SetCameraPosition( nCamera, nCamera );

        // Map x 129..191 is view x 65..127 with the camera at 64.
        CHECK( scene.Window( 30, 160, __WALL_Y - nCamera ) == std :: pair<int, int> { __WALL_MIN_X - nCamera, __WALL_MAX_X - nCamera } );
    }

    TEST_CASE( "A world-space sprite collides with the tile it is on, whatever the camera" )  {

        const int  nCamera = 64;
        Scene      scene( __ONE_WALL, true );

        scene.pRenderer -> SetCameraPosition( nCamera, nCamera );

        // Scarab measured no collision at map y 160, and a collision at map y 96 (two rows above the wall),
        // the camera having been added to a map position.
        CHECK( scene.Window( 100, 220, __WALL_Y ) == __EXPECTED );
        CHECK( scene.Window( 100, 220, __WALL_Y - nCamera ) == std :: pair<int, int> { -1, -1 } );
    }

    TEST_CASE( "The zoom does not move the window: positions and tiles are both in map pixels" )  {

        const unsigned  nZoomedIn = 30;
        Scene           scene( __ONE_WALL, false );

        scene.pRenderer -> SetCameraPosition( 0, 0 );
        scene.pRenderer -> GetViewport().SetZoom( nZoomedIn );
        REQUIRE( scene.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor != 1.0f );

        CHECK( scene.Window( 100, 220, __WALL_Y ) == __EXPECTED );
    }

    TEST_CASE( "A sprite over two walls hits both, each reported once with its map rectangle" )  {

        Scene  scene( { { __WALL_ROW, __WALL_COL }, { __WALL_ROW, __WALL_COL + 1 } }, true );

        std :: vector<SunLight :: TileMap :: stTile>  tiles = scene.HitsAt( 176, __WALL_Y );    // across the two

        REQUIRE( tiles.size() == 2 );
        CHECK( tiles[0].dimension.pos.x == 160 );
        CHECK( tiles[0].dimension.pos.y == 160 );
        CHECK( tiles[0].dimension.size.nWidth == __TILE );
        CHECK( tiles[1].dimension.pos.x == 192 );
        CHECK( tiles[1].dimension.pos.y == 160 );
    }

    TEST_CASE( "A sprite reaching a wall from above collides, and one sharing only its edge does not" )  {

        Scene  scene( __ONE_WALL, true );

        CHECK( scene.HitsAt( 160, 129 ).size() == 1 );         // overlapping the wall's top row
        CHECK( scene.HitsAt( 160, 128 ).empty() );             // its bottom edge on the wall's top edge
        CHECK( scene.HitsAt( 192, 160 ).empty() );             // its left edge on the wall's right edge
    }

    TEST_CASE( "GetTile and TileMapToTileMatrix answer in map space, round-tripping a wall cell" )  {

        Scene                                    scene( __ONE_WALL, false );
        SunLight :: TileMap :: stLayer           layer;
        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };
        SunLight :: TileMap :: stTile            tile;

        scene.pRenderer -> SetCameraPosition( 64, 64 );                            // must not matter

        REQUIRE( scene.pRenderer -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 170, 165 }, pos ) == true );
        CHECK( pos.nTileRow == __WALL_ROW );
        CHECK( pos.nTileCol == __WALL_COL );

        REQUIRE( scene.pRenderer -> GetLayer( __GROUND_ID, layer ) == true );
        REQUIRE( scene.pRenderer -> GetTile( pos, layer, tile ) == true );
        CHECK( tile.dimension.pos.x == 160 );
        CHECK( tile.dimension.pos.y == 160 );
    }

    TEST_CASE( "An isometric map: the wall's map rectangle, and a world-space sprite on it collides whatever the camera" )  {

        // The diamond lattice's cell top-left (IsometricProjection::CellTopLeft), worked out by hand: on a
        // 10-row map of 32 px tiles, cell (row 5, col 5) is at x = (5 - 5 + 9) * 16 = 144, y = (5 + 5) * 16 = 160.
        const int                                nWallX = 144;
        const int                                nWallY = 160;
        Scene                                    scene( __ONE_WALL, true, "isometric" );
        SunLight :: TileMap :: stLayer           layer;
        SunLight :: TileMap :: stTile            tile;
        SunLight :: TileMap :: stMatrixPosition  wall { __WALL_ROW, __WALL_COL };

        scene.pRenderer -> SetCameraPosition( 64, 64 );                            // must not matter

        REQUIRE( scene.pRenderer -> GetLayer( __GROUND_ID, layer ) == true );
        REQUIRE( scene.pRenderer -> GetTile( wall, layer, tile ) == true );
        CHECK( tile.dimension.pos.x == nWallX );
        CHECK( tile.dimension.pos.y == nWallY );

        std :: vector<SunLight :: TileMap :: stTile>  tiles = scene.HitsAt( nWallX, nWallY );

        REQUIRE( tiles.size() == 1 );
        CHECK( tiles[0].dimension.pos.x == nWallX );
        CHECK( tiles[0].dimension.pos.y == nWallY );

        CHECK( scene.HitsAt( 0, 0 ).empty() );
    }

    TEST_CASE( "No map, or a layer that is not a tile layer, reports no tile" )  {

        Scene                           scene( __ONE_WALL, true );
        SunLight :: TileMap :: stLayer  objects;
        std :: vector<SunLight :: TileMap :: stTile>  tiles;

        REQUIRE( scene.pRenderer -> GetLayer( __SPRITES_ID, objects ) == true );
        CHECK( scene.pRenderer -> GetCollidingTiles( SunLight :: Base :: stRectangle { 160, 160, 32, 32 }, true, objects, tiles ) == false );

        SunLight :: TileMap :: stLayer  ground;

        REQUIRE( scene.pRenderer -> GetLayer( __GROUND_ID, ground ) == true );
        REQUIRE( scene.pRenderer -> UnloadMap() == true );
        CHECK( scene.pRenderer -> GetCollidingTiles( SunLight :: Base :: stRectangle { 160, 160, 32, 32 }, true, ground, tiles ) == false );
        CHECK( tiles.empty() );
    }
}
