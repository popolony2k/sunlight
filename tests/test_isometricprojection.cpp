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
 * Reference values below are not derived from this engine's own formula - they were measured pixel-by-pixel
 * from real Tiled 1.11 "Export As Image" output (E1a of the master plan), for two differently-sized maps (to
 * catch a height-dependent term that a single map size could not reveal - see doc/MASTER_PLAN.md). Every
 * top-left value here is an independently observed fact, not a value this test's own code could have produced
 * by construction.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include "renderer/projection/isometricprojection.h"
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer :: Projection;
using SunLight :: Renderer :: TileMapRenderer;
using SunLight :: Renderer :: RendererConfig;
typedef SunLight :: TileMap :: stMatrixPosition  stMatrixPosition;
typedef SunLight :: TileMap :: ITileMap          ITM;
typedef MockEngine :: Event                      Event;

namespace  {

    tmx_map MakeMap( unsigned int nWidth, unsigned int nHeight, unsigned int nTileWidth, unsigned int nTileHeight )  {

        tmx_map  map {};

        map.width       = nWidth;
        map.height      = nHeight;
        map.tile_width  = nTileWidth;
        map.tile_height = nTileHeight;

        return map;
    }
}

TEST_SUITE( "Isometric projection" )  {

    TEST_CASE( "Map pixel size matches two independently measured Tiled exports" )  {

        IsometricProjection  projection;

        tmx_map  mapA = MakeMap( 3, 2, 64, 32 );
        tmx_map  mapB = MakeMap( 3, 4, 64, 32 );

        SunLight :: Base :: stSize2D  sizeA = projection.MapPixelSize( &mapA );
        SunLight :: Base :: stSize2D  sizeB = projection.MapPixelSize( &mapB );

        CHECK( sizeA.nWidth  == 160 );
        CHECK( sizeA.nHeight == 80 );
        CHECK( sizeB.nWidth  == 224 );
        CHECK( sizeB.nHeight == 112 );
    }

    TEST_CASE( "Tile draw position matches every cell of a 3x2 Tiled export" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 2, 64, 32 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 32, 0 },  { 1, 0, 64, 16 }, { 2, 0, 96, 32 },
            { 0, 1, 0,  16 }, { 1, 1, 32, 32 }, { 2, 1, 64, 48 },
        };

        for( const stExpected &e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 32, 32, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches every cell of a 3x4 Tiled export (a second map height)" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 4, 64, 32 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 96, 0  }, { 1, 0, 128, 16 }, { 2, 0, 160, 32 },
            { 0, 1, 64, 16 }, { 1, 1, 96,  32 }, { 2, 1, 128, 48 },
            { 0, 2, 32, 32 }, { 1, 2, 64,  48 }, { 2, 2, 96,  64 },
            { 0, 3, 0,  48 }, { 1, 3, 32,  64 }, { 2, 3, 64,  80 },
        };

        for( const stExpected &e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 32, 32, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "A layer's own offset shifts the draw position, same as orthogonal" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 2, 64, 32 );
        stMatrixPosition      pos { 1, 1 };

        SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 32, 32, 10, -5 );

        CHECK( dest.x == 32 + 10 );
        CHECK( dest.y == 32 - 5 );
    }

    TEST_CASE( "A resolved tile's own size does not move the lattice position" )  {

        // Confirmed by the probe maps themselves: every solid-color tile image was 32x32, smaller than the
        // declared 64x32 grid step, yet every cell landed exactly on the 64x32 lattice - so the position must
        // not depend on the nTileWidth/nTileHeight arguments at all, unlike orthogonal.
        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 2, 64, 32 );
        stMatrixPosition      pos { 1, 1 };

        SunLight :: Base :: stCoordinate2D  destSmall = projection.TileDrawPosition( pos, &map, 16, 16, 0, 0 );
        SunLight :: Base :: stCoordinate2D  destBig   = projection.TileDrawPosition( pos, &map, 128, 128, 0, 0 );

        CHECK( destSmall.x == 32 );
        CHECK( destSmall.y == 32 );
        CHECK( destBig.x == 32 );
        CHECK( destBig.y == 32 );
    }

    TEST_CASE( "View to tile matrix round-trips every cell of a 3x4 map, at the default view state" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 4, 64, 32 );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        for( int nRow = 0; nRow < 4; nRow++ )  {
            for( int nCol = 0; nCol < 3; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  topLeft = projection.TileDrawPosition( pos, &map, 32, 32, 0, 0 );

                // A point just inside the cell's own top-left corner - NOT the bounding box's center: a
                // neighboring cell's box overlaps this one by half its width/height (the lattice steps by
                // tileWidth/2 and tileHeight/2 per cell, half the box's own size), so the center of one cell's
                // box can land exactly on another cell's own top-left corner and round-trip to the wrong cell.
                // Only a point near THIS cell's own anchor corner is unambiguous.
                SunLight :: Base :: stCoordinate2D  probe { topLeft.x + 1, topLeft.y + 1 };

                stMatrixPosition  back {};
                bool  bFound = projection.ViewToTileMatrix( probe, &map, viewport, camera, 1.0f,
                                                            mapSize.nWidth, mapSize.nHeight, back );

                REQUIRE( bFound == true );
                CHECK( back.nTileCol == nCol );
                CHECK( back.nTileRow == nRow );
            }
        }
    }

    TEST_CASE( "View to tile matrix refuses a coordinate outside the map's own pixel bounds" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 2, 64, 32 );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        stMatrixPosition  pos {};

        CHECK( projection.ViewToTileMatrix( { -1, 0 }, &map, viewport, camera, 1.0f,
                                            mapSize.nWidth, mapSize.nHeight, pos ) == false );
        CHECK( projection.ViewToTileMatrix( { mapSize.nWidth, 0 }, &map, viewport, camera, 1.0f,
                                            mapSize.nWidth, mapSize.nHeight, pos ) == false );
    }

    TEST_CASE( "Default scroll step is one full tile, same convention as orthogonal" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 2, 64, 32 );

        SunLight :: Base :: stSize2D  step = projection.DefaultScrollStep( &map );

        CHECK( step.nWidth  == 64 );
        CHECK( step.nHeight == 32 );
    }

    TEST_CASE( "Visible tile range covers the whole map when the rectangle covers the whole map" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 4, 64, 32 );

        SunLight :: Base :: stSize2D  mapSize = projection.MapPixelSize( &map );
        SunLight :: Base :: stRectangle  wholeMap { 0.0f, 0.0f, ( float ) mapSize.nWidth, ( float ) mapSize.nHeight };

        stTileRange  range = projection.VisibleTileRange( &map, wholeMap );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 3 );
        CHECK( range.nRowEnd   == 4 );
    }

    TEST_CASE( "Visible tile range is bounded, not the whole grid, for a small rectangle" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 20, 20, 64, 32 );

        // A small rectangle near the map's own top corner - should only touch a handful of cells, not all 400.
        SunLight :: Base :: stRectangle  smallRect { 600.0f, 0.0f, 64.0f, 32.0f };

        stTileRange  range = projection.VisibleTileRange( &map, smallRect );

        int  nCellCount = ( range.nColEnd - range.nColStart ) * ( range.nRowEnd - range.nRowStart );

        CHECK( nCellCount > 0 );
        CHECK( nCellCount < 400 );
    }

    TEST_CASE( "Visible tile range clamps to the map's own grid, never past its edges" )  {

        IsometricProjection  projection;
        tmx_map              map = MakeMap( 3, 2, 64, 32 );

        // A rectangle far larger than the map in every direction.
        SunLight :: Base :: stRectangle  hugeRect { -10000.0f, -10000.0f, 20000.0f, 20000.0f };

        stTileRange  range = projection.VisibleTileRange( &map, hugeRect );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 3 );
        CHECK( range.nRowEnd   == 2 );
    }
}

namespace  {

    // A 3 x 2 isometric map (64 x 32 declared tile size), one tileset matching that size exactly (so
    // m_bUniformTileGrid is true and DrawLayer takes the culled path), every cell filled with gid 1.
    Bytes MakeIsometricTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"isometric\" renderorder=\"right-down\" width=\"3\""
            << " height=\"2\" tilewidth=\"64\" tileheight=\"32\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"64\" tileheight=\"32\""
            << " tilecount=\"1\" columns=\"1\"><image source=\"tiles.png\" width=\"64\" height=\"32\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"3\" height=\"2\"><data encoding=\"csv\">"
            << "1,1,1,1,1,1</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }
}

TEST_SUITE( "Isometric rendering (renderer integration)" )  {

    TEST_CASE( "An isometric map draws every cell at the measured Tiled position" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;

        fsFixture.fs.files["maps/iso.tmx"] = MakeIsometricTmx();

        RendererConfig  config;

        config.fWidth  = 1260.0f;
        config.fHeight = 920.0f;

        SunLight :: Base :: stDimension2D  viewport {};

        viewport.pos.x = 0;  viewport.pos.y = 0;  viewport.size.nWidth = 320;  viewport.size.nHeight = 240;
        config.viewport = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        windowFixture.window.nFramesUntilShouldClose = 1;
        REQUIRE( pRenderer -> Start() == true );
        REQUIRE( pRenderer -> LoadMap( "maps/iso.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        pRenderer -> Run();

        SunLight :: TileMap :: stMapInfo  info {};

        // GetMapInfo's mapSize is the grid's own cell count (3x2), not the projected pixel size - that pixel
        // formula is already covered directly by the "Map pixel size" test above.
        REQUIRE( pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.mapSize.nWidth  == 3 );
        CHECK( info.mapSize.nHeight == 2 );
        CHECK( info.tileSize.nWidth  == 64 );
        CHECK( info.tileSize.nHeight == 32 );

        struct stExpected { int nX; int nY; };

        // Same 3x2, 64x32 reference positions measured from Tiled in E1a, now checked end to end: LoadMap's
        // orientation switch, DrawLayer's culled loop and IsometricProjection must all agree with each other.
        const stExpected  expected[] = {
            { 32, 0 }, { 64, 16 }, { 96, 32 }, { 0, 16 }, { 32, 32 }, { 64, 48 },
        };

        for( const stExpected &e : expected )  {
            bool  bFound = false;

            for( const Event &evt : engineFixture.engine.events )  {
                if( ( evt.kind == Event :: TILE ) && ( ( int ) evt.x == e.nX ) && ( ( int ) evt.y == e.nY ) )  {
                    bFound = true;
                    break;
                }
            }

            INFO( "expected x=" << e.nX << " y=" << e.nY );
            CHECK( bFound == true );
        }

        pRenderer -> Stop();
    }
}
