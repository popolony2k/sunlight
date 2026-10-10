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
 * from real Tiled 1.11 exports (tmxrasterizer, pixel-identical to its GUI "Export As Image" - E2a of the
 * master plan), for all 4 stagger_axis/stagger_index combinations, each at 3 differently-sized maps (a
 * baseline, one that varies only row count, one that varies only column count - so a row-dependent and a
 * column-dependent term could not be confused with each other). Every position here is an independently
 * observed fact, not a value this test's own code could have produced by construction.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include "renderer/projection/staggeredprojection.h"
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

    tmx_map MakeMap( unsigned int nWidth, unsigned int nHeight, unsigned int nTileWidth, unsigned int nTileHeight,
                      tmx_stagger_axis axis, tmx_stagger_index index )  {

        tmx_map  map {};

        map.width         = nWidth;
        map.height        = nHeight;
        map.tile_width    = nTileWidth;
        map.tile_height   = nTileHeight;
        map.stagger_axis  = axis;
        map.stagger_index = index;

        return map;
    }
}

TEST_SUITE( "Staggered projection" )  {

    TEST_CASE( "Map pixel size matches three independently measured Tiled exports, axis Y" )  {

        StaggeredProjection  projection;

        tmx_map  mapBase   = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );
        tmx_map  mapTaller = MakeMap( 4, 5, 64, 48, SA_Y, SI_ODD );
        tmx_map  mapWider  = MakeMap( 6, 3, 64, 48, SA_Y, SI_ODD );

        SunLight :: Base :: stSize2D  sizeBase   = projection.MapPixelSize( &mapBase );
        SunLight :: Base :: stSize2D  sizeTaller = projection.MapPixelSize( &mapTaller );
        SunLight :: Base :: stSize2D  sizeWider  = projection.MapPixelSize( &mapWider );

        CHECK( sizeBase.nWidth    == 288 );
        CHECK( sizeBase.nHeight   == 96 );
        CHECK( sizeTaller.nWidth  == 288 );
        CHECK( sizeTaller.nHeight == 144 );
        CHECK( sizeWider.nWidth   == 416 );
        CHECK( sizeWider.nHeight  == 96 );
    }

    TEST_CASE( "Map pixel size matches three independently measured Tiled exports, axis X" )  {

        StaggeredProjection  projection;

        tmx_map  mapBase   = MakeMap( 4, 3, 64, 48, SA_X, SI_ODD );
        tmx_map  mapTaller = MakeMap( 4, 5, 64, 48, SA_X, SI_ODD );
        tmx_map  mapWider  = MakeMap( 6, 3, 64, 48, SA_X, SI_ODD );

        SunLight :: Base :: stSize2D  sizeBase   = projection.MapPixelSize( &mapBase );
        SunLight :: Base :: stSize2D  sizeTaller = projection.MapPixelSize( &mapTaller );
        SunLight :: Base :: stSize2D  sizeWider  = projection.MapPixelSize( &mapWider );

        CHECK( sizeBase.nWidth    == 160 );
        CHECK( sizeBase.nHeight   == 168 );
        CHECK( sizeTaller.nWidth  == 160 );
        CHECK( sizeTaller.nHeight == 264 );
        CHECK( sizeWider.nWidth   == 224 );
        CHECK( sizeWider.nHeight  == 168 );
    }

    TEST_CASE( "Map pixel size does not depend on stagger_index, only stagger_axis" )  {

        StaggeredProjection  projection;

        tmx_map  mapOdd  = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );
        tmx_map  mapEven = MakeMap( 4, 3, 64, 48, SA_Y, SI_EVEN );

        SunLight :: Base :: stSize2D  sizeOdd  = projection.MapPixelSize( &mapOdd );
        SunLight :: Base :: stSize2D  sizeEven = projection.MapPixelSize( &mapEven );

        CHECK( sizeOdd.nWidth  == sizeEven.nWidth );
        CHECK( sizeOdd.nHeight == sizeEven.nHeight );
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis Y, index odd" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,   0 }, { 3, 0, 192, 0 },   // A, B
            { 0, 2, 0,   48 }, { 3, 2, 192, 48 }, // C, D
            { 1, 1, 96,  24 },                    // E
        };

        for( const stExpected &e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 64, 48, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis Y, index even" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_EVEN );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 32,  0 }, { 3, 0, 224, 0 },   // A, B
            { 0, 2, 32,  48 }, { 3, 2, 224, 48 }, // C, D
            { 1, 1, 64,  24 },                    // E
        };

        for( const stExpected &e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 64, 48, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis X, index odd" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_X, SI_ODD );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,  0 }, { 3, 0, 96, 24 },    // A, B
            { 0, 2, 0,  96 }, { 3, 2, 96, 120 },  // C, D
            { 1, 1, 32, 72 },                     // E
        };

        for( const stExpected &e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 64, 48, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis X, index even" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_X, SI_EVEN );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,  24 }, { 3, 0, 96, 0 },    // A, B
            { 0, 2, 0,  120 }, { 3, 2, 96, 96 },  // C, D
            { 1, 1, 32, 48 },                     // E
        };

        for( const stExpected &e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 64, 48, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position does not depend on the map's own width/height, unlike isometric" )  {

        // Confirmed by the probe maps themselves: cell (row0, col0) landed at the exact same pixel
        // position whether the map was 4x3, 4x5 or 6x3 - unlike isometric, whose lattice shifts with
        // the map's own height. Checked here against two of those measured sizes directly.
        StaggeredProjection  projection;
        tmx_map              mapBase   = MakeMap( 4, 3, 64, 48, SA_Y, SI_EVEN );
        tmx_map              mapTaller = MakeMap( 4, 5, 64, 48, SA_Y, SI_EVEN );
        stMatrixPosition      pos { 0, 0 };

        SunLight :: Base :: stCoordinate2D  destBase   = projection.TileDrawPosition( pos, &mapBase, 64, 48, 0, 0 );
        SunLight :: Base :: stCoordinate2D  destTaller = projection.TileDrawPosition( pos, &mapTaller, 64, 48, 0, 0 );

        CHECK( destBase.x == 32 );
        CHECK( destBase.y == 0 );
        CHECK( destTaller.x == destBase.x );
        CHECK( destTaller.y == destBase.y );
    }

    TEST_CASE( "A layer's own offset shifts the draw position, same as orthogonal and isometric" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );
        stMatrixPosition      pos { 1, 1 };

        SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 64, 48, 10, -5 );

        CHECK( dest.x == 96 + 10 );
        CHECK( dest.y == 24 - 5 );
    }

    TEST_CASE( "A resolved tile's own size does not move the lattice position" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );
        stMatrixPosition      pos { 1, 1 };

        SunLight :: Base :: stCoordinate2D  destSmall = projection.TileDrawPosition( pos, &map, 16, 16, 0, 0 );
        SunLight :: Base :: stCoordinate2D  destBig   = projection.TileDrawPosition( pos, &map, 128, 128, 0, 0 );

        CHECK( destSmall.x == 96 );
        CHECK( destSmall.y == 24 );
        CHECK( destBig.x == 96 );
        CHECK( destBig.y == 24 );
    }

    TEST_CASE( "View to tile matrix round-trips every cell of a 4x3 map, axis Y index odd" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        for( int nRow = 0; nRow < 3; nRow++ )  {
            for( int nCol = 0; nCol < 4; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  topLeft = projection.TileDrawPosition( pos, &map, 64, 48, 0, 0 );

                // A point just inside the cell's own top-left corner - NOT the bounding box's center: a
                // neighbouring cell's box overlaps this one by half its width/height on the staggered
                // axis (E2a), so the center of one cell's box can land inside another cell's own span.
                // Only a point near THIS cell's own anchor corner is unambiguous (see StaggeredProjection's
                // own doc comment).
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

    TEST_CASE( "View to tile matrix round-trips every cell of a 4x3 map, axis X index even" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_X, SI_EVEN );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        for( int nRow = 0; nRow < 3; nRow++ )  {
            for( int nCol = 0; nCol < 4; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  topLeft = projection.TileDrawPosition( pos, &map, 64, 48, 0, 0 );
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

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        stMatrixPosition  pos {};

        CHECK( projection.ViewToTileMatrix( { -1, 0 }, &map, viewport, camera, 1.0f,
                                            mapSize.nWidth, mapSize.nHeight, pos ) == false );
        CHECK( projection.ViewToTileMatrix( { mapSize.nWidth, 0 }, &map, viewport, camera, 1.0f,
                                            mapSize.nWidth, mapSize.nHeight, pos ) == false );
    }

    TEST_CASE( "Default scroll step is one full tile, same convention as orthogonal and isometric" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );

        SunLight :: Base :: stSize2D  step = projection.DefaultScrollStep( &map );

        CHECK( step.nWidth  == 64 );
        CHECK( step.nHeight == 48 );
    }

    TEST_CASE( "Visible tile range covers the whole map when the rectangle covers the whole map, axis Y" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );

        SunLight :: Base :: stSize2D  mapSize = projection.MapPixelSize( &map );
        SunLight :: Base :: stRectangle  wholeMap { 0.0f, 0.0f, ( float ) mapSize.nWidth, ( float ) mapSize.nHeight };

        stTileRange  range = projection.VisibleTileRange( &map, wholeMap );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 4 );
        CHECK( range.nRowEnd   == 3 );
    }

    TEST_CASE( "Visible tile range covers the whole map when the rectangle covers the whole map, axis X" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_X, SI_EVEN );

        SunLight :: Base :: stSize2D  mapSize = projection.MapPixelSize( &map );
        SunLight :: Base :: stRectangle  wholeMap { 0.0f, 0.0f, ( float ) mapSize.nWidth, ( float ) mapSize.nHeight };

        stTileRange  range = projection.VisibleTileRange( &map, wholeMap );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 4 );
        CHECK( range.nRowEnd   == 3 );
    }

    TEST_CASE( "Visible tile range's margin includes a row whose cell starts one pitch earlier, axis Y" )  {

        // A thin rectangle NOT at the map's own edge (where the raw floor would otherwise be clamped to
        // the same result with or without the margin, hiding a missing margin entirely - found this way
        // while mutation-testing the first version of this test, which only checked rect.y == 0). tileH =
        // 48, halfH = 24: floor(100/24) = 4, so row 4's cell starts at y=96 and covers [96,144), which does
        // reach the rect at y=[100,110) - but so does row 3's cell, which starts one pitch (24px) earlier
        // at y=72 and covers [72,120): 100 < 120, so it overlaps too, and omitting it would silently drop
        // a visible row from the culled draw loop.
        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 10, 64, 48, SA_Y, SI_ODD );

        SunLight :: Base :: stRectangle  rect { 0.0f, 100.0f, 64.0f, 10.0f };

        stTileRange  range = projection.VisibleTileRange( &map, rect );

        CHECK( range.nRowStart <= 3 );
        CHECK( range.nRowEnd > 4 );
    }

    TEST_CASE( "Visible tile range's margin includes a column whose cell starts one pitch earlier, axis X" )  {

        // The same finding, mirrored: tileW = 64, halfW = 32: floor(130/32) = 4, so column 4's cell starts
        // at x=128 and covers [128,192) - but column 3's cell starts one pitch (32px) earlier at x=96 and
        // covers [96,160): 130 < 160, so it overlaps the rect at x=[130,140) too.
        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 10, 4, 64, 48, SA_X, SI_EVEN );

        SunLight :: Base :: stRectangle  rect { 130.0f, 0.0f, 10.0f, 48.0f };

        stTileRange  range = projection.VisibleTileRange( &map, rect );

        CHECK( range.nColStart <= 3 );
        CHECK( range.nColEnd > 4 );
    }

    TEST_CASE( "Visible tile range is bounded, not the whole grid, for a small rectangle" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 20, 20, 64, 48, SA_Y, SI_ODD );

        // A small rectangle near the map's own top corner - should only touch a handful of cells, not all 400.
        SunLight :: Base :: stRectangle  smallRect { 600.0f, 0.0f, 64.0f, 48.0f };

        stTileRange  range = projection.VisibleTileRange( &map, smallRect );

        int  nCellCount = ( range.nColEnd - range.nColStart ) * ( range.nRowEnd - range.nRowStart );

        CHECK( nCellCount > 0 );
        CHECK( nCellCount < 400 );
    }

    TEST_CASE( "Visible tile range clamps to the map's own grid, never past its edges" )  {

        StaggeredProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD );

        // A rectangle far larger than the map in every direction.
        SunLight :: Base :: stRectangle  hugeRect { -10000.0f, -10000.0f, 20000.0f, 20000.0f };

        stTileRange  range = projection.VisibleTileRange( &map, hugeRect );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 4 );
        CHECK( range.nRowEnd   == 3 );
    }
}

namespace  {

    // A 3 x 2 staggered map (64 x 48 declared tile size, no explicit staggeraxis/staggerindex -
    // libtmx defaults a missing attribute to SA_Y/SI_ODD, confirmed against its own source in E2a), one
    // tileset matching that size exactly (so m_bUniformTileGrid is true and DrawLayer takes the culled
    // path), every cell filled with gid 1.
    Bytes MakeStaggeredTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"staggered\" renderorder=\"right-down\" width=\"3\""
            << " height=\"2\" tilewidth=\"64\" tileheight=\"48\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"64\" tileheight=\"48\""
            << " tilecount=\"1\" columns=\"1\"><image source=\"tiles.png\" width=\"64\" height=\"48\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"3\" height=\"2\"><data encoding=\"csv\">"
            << "1,1,1,1,1,1</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }
}

TEST_SUITE( "Staggered rendering (renderer integration)" )  {

    TEST_CASE( "A staggered map draws every cell at the formula-derived position (default axis/index)" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;

        fsFixture.fs.files["maps/sta.tmx"] = MakeStaggeredTmx();

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
        REQUIRE( pRenderer -> LoadMap( "maps/sta.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        pRenderer -> Run();

        SunLight :: TileMap :: stMapInfo  info {};

        REQUIRE( pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.mapSize.nWidth  == 3 );
        CHECK( info.mapSize.nHeight == 2 );
        CHECK( info.tileSize.nWidth  == 64 );
        CHECK( info.tileSize.nHeight == 48 );

        struct stExpected { int nX; int nY; };

        // Default axis/index is SA_Y/SI_ODD (libtmx's own default for a missing attribute, E2a): row0
        // unshifted, row1 shifted by tileWidth/2 = 32. Derived from the same, already Tiled-measured
        // formula (see the "Staggered projection" suite above) - this test checks LoadMap's orientation
        // switch, DrawLayer's culled loop and StaggeredProjection all agree with each other end to end,
        // the same purpose IsometricProjection's own integration test serves.
        const stExpected  expected[] = {
            { 0, 0 }, { 64, 0 }, { 128, 0 }, { 32, 24 }, { 96, 24 }, { 160, 24 },
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
