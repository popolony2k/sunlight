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
 * from real Tiled 1.11 exports (tmxrasterizer - E3a of the master plan), for both stagger_axis values, both
 * stagger_index values, two different hexsidelength values, and 3 differently-sized maps per axis (a
 * baseline, one that varies only row count, one that varies only column count). Every position here is an
 * independently observed fact, not a value this test's own code could have produced by construction.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include "renderer/projection/hexagonalprojection.h"
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
                      tmx_stagger_axis axis, tmx_stagger_index index, int nHexSideLength )  {

        tmx_map  map {};

        map.width         = nWidth;
        map.height        = nHeight;
        map.tile_width    = nTileWidth;
        map.tile_height   = nTileHeight;
        map.stagger_axis  = axis;
        map.stagger_index = index;
        map.hexsidelength = nHexSideLength;

        return map;
    }
}

TEST_SUITE( "Hexagonal projection" )  {

    TEST_CASE( "Map pixel size matches three independently measured Tiled exports, axis Y" )  {

        HexagonalProjection  projection;

        tmx_map  mapBase   = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );
        tmx_map  mapTaller = MakeMap( 4, 5, 60, 50, SA_Y, SI_ODD, 20 );
        tmx_map  mapWider  = MakeMap( 6, 3, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stSize2D  sizeBase   = projection.MapPixelSize( &mapBase );
        SunLight :: Base :: stSize2D  sizeTaller = projection.MapPixelSize( &mapTaller );
        SunLight :: Base :: stSize2D  sizeWider  = projection.MapPixelSize( &mapWider );

        CHECK( sizeBase.nWidth    == 270 );
        CHECK( sizeBase.nHeight   == 120 );
        CHECK( sizeTaller.nWidth  == 270 );
        CHECK( sizeTaller.nHeight == 190 );
        CHECK( sizeWider.nWidth   == 390 );
        CHECK( sizeWider.nHeight  == 120 );
    }

    TEST_CASE( "Map pixel size matches three independently measured Tiled exports, axis X" )  {

        HexagonalProjection  projection;

        tmx_map  mapBase   = MakeMap( 4, 3, 50, 60, SA_X, SI_ODD, 20 );
        tmx_map  mapTaller = MakeMap( 4, 5, 50, 60, SA_X, SI_ODD, 20 );
        tmx_map  mapWider  = MakeMap( 6, 3, 50, 60, SA_X, SI_ODD, 20 );

        SunLight :: Base :: stSize2D  sizeBase   = projection.MapPixelSize( &mapBase );
        SunLight :: Base :: stSize2D  sizeTaller = projection.MapPixelSize( &mapTaller );
        SunLight :: Base :: stSize2D  sizeWider  = projection.MapPixelSize( &mapWider );

        CHECK( sizeBase.nWidth    == 155 );
        CHECK( sizeBase.nHeight   == 210 );
        CHECK( sizeTaller.nWidth  == 155 );
        CHECK( sizeTaller.nHeight == 330 );
        CHECK( sizeWider.nWidth   == 225 );
        CHECK( sizeWider.nHeight  == 210 );
    }

    TEST_CASE( "Map pixel size does not depend on stagger_index, only stagger_axis and hexsidelength" )  {

        HexagonalProjection  projection;

        tmx_map  mapOdd  = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );
        tmx_map  mapEven = MakeMap( 4, 3, 60, 50, SA_Y, SI_EVEN, 20 );

        SunLight :: Base :: stSize2D  sizeOdd  = projection.MapPixelSize( &mapOdd );
        SunLight :: Base :: stSize2D  sizeEven = projection.MapPixelSize( &mapEven );

        CHECK( sizeOdd.nWidth  == sizeEven.nWidth );
        CHECK( sizeOdd.nHeight == sizeEven.nHeight );
    }

    TEST_CASE( "Map pixel size changes with hexsidelength, measured at two values" )  {

        HexagonalProjection  projection;

        tmx_map  mapH20 = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );
        tmx_map  mapH10 = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 10 );

        SunLight :: Base :: stSize2D  sizeH20 = projection.MapPixelSize( &mapH20 );
        SunLight :: Base :: stSize2D  sizeH10 = projection.MapPixelSize( &mapH10 );

        CHECK( sizeH20.nWidth  == sizeH10.nWidth );   // the non-staggered axis is unaffected
        CHECK( sizeH20.nHeight == 120 );
        CHECK( sizeH10.nHeight == 110 );
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis Y, index odd, hexside 20" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,   0 }, { 3, 0, 180, 0 },
            { 0, 2, 0,   70 }, { 3, 2, 180, 70 },
            { 1, 1, 90,  35 },
        };

        for( const stExpected& e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 60, 50, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis Y, index even, hexside 20" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_EVEN, 20 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 30,  0 }, { 3, 0, 210, 0 },
            { 0, 2, 30,  70 }, { 3, 2, 210, 70 },
            { 1, 1, 60,  35 },
        };

        for( const stExpected& e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 60, 50, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis Y, index odd, hexside 10" )  {

        // Same axis/index/map size as the hexside=20 case above - only the row pitch changes
        // (35 -> 30), demonstrating hexsidelength's own effect in isolation.
        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 10 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,  0 }, { 3, 0, 180, 0 },
            { 0, 2, 0,  60 }, { 3, 2, 180, 60 },
            { 1, 1, 90, 30 },
        };

        for( const stExpected& e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 60, 50, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis X, index odd, hexside 20" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 50, 60, SA_X, SI_ODD, 20 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,   0 }, { 3, 0, 105, 30 },
            { 0, 2, 0,   120 }, { 3, 2, 105, 150 },
            { 1, 1, 35,  90 },
        };

        for( const stExpected& e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 50, 60, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis X, index even, hexside 20" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 50, 60, SA_X, SI_EVEN, 20 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,   30 }, { 3, 0, 105, 0 },
            { 0, 2, 0,   150 }, { 3, 2, 105, 120 },
            { 1, 1, 35,  60 },
        };

        for( const stExpected& e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 50, 60, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position matches a 4x3 Tiled export, axis X, index odd, hexside 10" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 50, 60, SA_X, SI_ODD, 10 );

        struct stExpected { int nCol; int nRow; int nX; int nY; };

        const stExpected  expected[] = {
            { 0, 0, 0,  0 }, { 3, 0, 90, 30 },
            { 0, 2, 0,  120 }, { 3, 2, 90, 150 },
            { 1, 1, 30, 90 },
        };

        for( const stExpected& e : expected )  {
            INFO( "col=" << e.nCol << " row=" << e.nRow );

            stMatrixPosition  pos { e.nRow, e.nCol };
            SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 50, 60, 0, 0 );

            CHECK( dest.x == e.nX );
            CHECK( dest.y == e.nY );
        }
    }

    TEST_CASE( "Tile draw position does not depend on the map's own width/height, unlike isometric" )  {

        HexagonalProjection  projection;
        tmx_map              mapBase   = MakeMap( 4, 3, 60, 50, SA_Y, SI_EVEN, 20 );
        tmx_map              mapTaller = MakeMap( 4, 5, 60, 50, SA_Y, SI_EVEN, 20 );
        stMatrixPosition      pos { 0, 0 };

        SunLight :: Base :: stCoordinate2D  destBase   = projection.TileDrawPosition( pos, &mapBase, 60, 50, 0, 0 );
        SunLight :: Base :: stCoordinate2D  destTaller = projection.TileDrawPosition( pos, &mapTaller, 60, 50, 0, 0 );

        CHECK( destBase.x == 30 );
        CHECK( destBase.y == 0 );
        CHECK( destTaller.x == destBase.x );
        CHECK( destTaller.y == destBase.y );
    }

    TEST_CASE( "A layer's own offset shifts the draw position, same as the other projections" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );
        stMatrixPosition      pos { 1, 1 };

        SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, &map, 60, 50, 10, -5 );

        CHECK( dest.x == 90 + 10 );
        CHECK( dest.y == 35 - 5 );
    }

    TEST_CASE( "A resolved tile's own size does not move the lattice position" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );
        stMatrixPosition      pos { 1, 1 };

        SunLight :: Base :: stCoordinate2D  destSmall = projection.TileDrawPosition( pos, &map, 16, 16, 0, 0 );
        SunLight :: Base :: stCoordinate2D  destBig   = projection.TileDrawPosition( pos, &map, 128, 128, 0, 0 );

        CHECK( destSmall.x == 90 );
        CHECK( destSmall.y == 35 );
        CHECK( destBig.x == 90 );
        CHECK( destBig.y == 35 );
    }

    TEST_CASE( "hexsidelength 0 matches StaggeredProjection exactly - the derivation's own cross-check" )  {

        // Not designed in - this fell out of the independently-measured numbers during E3a, and is
        // kept here as a regression test tying the two projections together: if either one's formula
        // ever changes, this is the test that would catch the two falling out of agreement.
        HexagonalProjection  hexProjection;
        StaggeredProjection  staggeredProjection;

        tmx_map  hexMap       = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD, 0 );
        tmx_map  staggeredMap = MakeMap( 4, 3, 64, 48, SA_Y, SI_ODD, 0 );

        SunLight :: Base :: stSize2D  hexSize       = hexProjection.MapPixelSize( &hexMap );
        SunLight :: Base :: stSize2D  staggeredSize = staggeredProjection.MapPixelSize( &staggeredMap );

        CHECK( hexSize.nWidth  == staggeredSize.nWidth );
        CHECK( hexSize.nHeight == staggeredSize.nHeight );

        for( int nRow = 0; nRow < 3; nRow++ )  {
            for( int nCol = 0; nCol < 4; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  hexDest       = hexProjection.TileDrawPosition( pos, &hexMap, 64, 48, 0, 0 );
                SunLight :: Base :: stCoordinate2D  staggeredDest = staggeredProjection.TileDrawPosition( pos, &staggeredMap, 64, 48, 0, 0 );

                CHECK( hexDest.x == staggeredDest.x );
                CHECK( hexDest.y == staggeredDest.y );
            }
        }
    }

    TEST_CASE( "View to tile matrix round-trips every cell of a 4x3 map, axis Y index odd hexside 20" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        for( int nRow = 0; nRow < 3; nRow++ )  {
            for( int nCol = 0; nCol < 4; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  topLeft = projection.TileDrawPosition( pos, &map, 60, 50, 0, 0 );
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

    TEST_CASE( "View to tile matrix round-trips every cell of a 4x3 map, axis X index even hexside 20" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 50, 60, SA_X, SI_EVEN, 20 );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        for( int nRow = 0; nRow < 3; nRow++ )  {
            for( int nCol = 0; nCol < 4; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  topLeft = projection.TileDrawPosition( pos, &map, 50, 60, 0, 0 );
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

    TEST_CASE( "View to tile matrix round-trips with a different hexsidelength (10), axis Y index odd" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 10 );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        for( int nRow = 0; nRow < 3; nRow++ )  {
            for( int nCol = 0; nCol < 4; nCol++ )  {
                INFO( "col=" << nCol << " row=" << nRow );

                stMatrixPosition  pos { nRow, nCol };
                SunLight :: Base :: stCoordinate2D  topLeft = projection.TileDrawPosition( pos, &map, 60, 50, 0, 0 );
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

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stDimension2D  viewport {};
        SunLight :: Base :: stVector2D     camera { 0.0f, 0.0f };
        SunLight :: Base :: stSize2D       mapSize = projection.MapPixelSize( &map );

        stMatrixPosition  pos {};

        CHECK( projection.ViewToTileMatrix( { -1, 0 }, &map, viewport, camera, 1.0f,
                                            mapSize.nWidth, mapSize.nHeight, pos ) == false );
        CHECK( projection.ViewToTileMatrix( { mapSize.nWidth, 0 }, &map, viewport, camera, 1.0f,
                                            mapSize.nWidth, mapSize.nHeight, pos ) == false );
    }

    TEST_CASE( "Default scroll step is one full tile, same convention as the other projections" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stSize2D  step = projection.DefaultScrollStep( &map );

        CHECK( step.nWidth  == 60 );
        CHECK( step.nHeight == 50 );
    }

    TEST_CASE( "Visible tile range covers the whole map when the rectangle covers the whole map, axis Y" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stSize2D  mapSize = projection.MapPixelSize( &map );
        SunLight :: Base :: stRectangle  wholeMap { 0.0f, 0.0f, ( float ) mapSize.nWidth, ( float ) mapSize.nHeight };

        stTileRange  range = projection.VisibleTileRange( &map, wholeMap );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 4 );
        CHECK( range.nRowEnd   == 3 );
    }

    TEST_CASE( "Visible tile range covers the whole map when the rectangle covers the whole map, axis X" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 50, 60, SA_X, SI_EVEN, 20 );

        SunLight :: Base :: stSize2D  mapSize = projection.MapPixelSize( &map );
        SunLight :: Base :: stRectangle  wholeMap { 0.0f, 0.0f, ( float ) mapSize.nWidth, ( float ) mapSize.nHeight };

        stTileRange  range = projection.VisibleTileRange( &map, wholeMap );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 4 );
        CHECK( range.nRowEnd   == 3 );
    }

    TEST_CASE( "Visible tile range's margin includes a row whose cell starts one pitch earlier, axis Y" )  {

        // tileHeight=50, hexside=20, pitchY=35: floor(105/35)=3, so row3's cell starts at y=105 and
        // covers [105,155) - but row2's cell, starting one pitch (35px) earlier at y=70, covers
        // [70,120): 120 > 105, so it overlaps the rect at y=[105,110) too.
        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 10, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stRectangle  rect { 0.0f, 105.0f, 60.0f, 5.0f };

        stTileRange  range = projection.VisibleTileRange( &map, rect );

        CHECK( range.nRowStart <= 2 );
        CHECK( range.nRowEnd > 3 );
    }

    TEST_CASE( "Visible tile range's margin includes a column whose cell starts one pitch earlier, axis X" )  {

        // Mirrored: tileWidth=50, hexside=20, pitchX=35 - same numbers as the axis Y case above.
        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 10, 4, 50, 60, SA_X, SI_EVEN, 20 );

        SunLight :: Base :: stRectangle  rect { 105.0f, 0.0f, 5.0f, 60.0f };

        stTileRange  range = projection.VisibleTileRange( &map, rect );

        CHECK( range.nColStart <= 2 );
        CHECK( range.nColEnd > 3 );
    }

    TEST_CASE( "Visible tile range is bounded, not the whole grid, for a small rectangle" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 20, 20, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stRectangle  smallRect { 600.0f, 0.0f, 60.0f, 50.0f };

        stTileRange  range = projection.VisibleTileRange( &map, smallRect );

        int  nCellCount = ( range.nColEnd - range.nColStart ) * ( range.nRowEnd - range.nRowStart );

        CHECK( nCellCount > 0 );
        CHECK( nCellCount < 400 );
    }

    TEST_CASE( "Visible tile range clamps to the map's own grid, never past its edges" )  {

        HexagonalProjection  projection;
        tmx_map              map = MakeMap( 4, 3, 60, 50, SA_Y, SI_ODD, 20 );

        SunLight :: Base :: stRectangle  hugeRect { -10000.0f, -10000.0f, 20000.0f, 20000.0f };

        stTileRange  range = projection.VisibleTileRange( &map, hugeRect );

        CHECK( range.nColStart == 0 );
        CHECK( range.nRowStart == 0 );
        CHECK( range.nColEnd   == 4 );
        CHECK( range.nRowEnd   == 3 );
    }
}

namespace  {

    // A 3 x 2 hexagonal map (60 x 50 declared tile size, hexsidelength=20 - no default exists for
    // this attribute, so it is always set explicitly, the same as a real Tiled-authored hex map must
    // do; no explicit staggeraxis/staggerindex - libtmx defaults a missing attribute to SA_Y/SI_ODD),
    // one tileset matching that size exactly, every cell filled with gid 1.
    Bytes MakeHexagonalTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"hexagonal\" renderorder=\"right-down\" hexsidelength=\"20\" width=\"3\""
            << " height=\"2\" tilewidth=\"60\" tileheight=\"50\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"60\" tileheight=\"50\""
            << " tilecount=\"1\" columns=\"1\"><image source=\"tiles.png\" width=\"60\" height=\"50\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"3\" height=\"2\"><data encoding=\"csv\">"
            << "1,1,1,1,1,1</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }
}

TEST_SUITE( "Hexagonal rendering (renderer integration)" )  {

    TEST_CASE( "A hexagonal map draws every cell at the formula-derived position (default axis/index)" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;

        fsFixture.fs.files["maps/hex.tmx"] = MakeHexagonalTmx();

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
        REQUIRE( pRenderer -> LoadMap( "maps/hex.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        pRenderer -> Run();

        SunLight :: TileMap :: stMapInfo  info {};

        REQUIRE( pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.mapSize.nWidth  == 3 );
        CHECK( info.mapSize.nHeight == 2 );
        CHECK( info.tileSize.nWidth  == 60 );
        CHECK( info.tileSize.nHeight == 50 );

        struct stExpected { int nX; int nY; };

        // Default axis/index is SA_Y/SI_ODD; tileWidth=60, hexsidelength=20, pitchY=(50+20)/2=35:
        // row0 unshifted, row1 shifted by tileWidth/2=30.
        const stExpected  expected[] = {
            { 0, 0 }, { 60, 0 }, { 120, 0 }, { 30, 35 }, { 90, 35 }, { 150, 35 },
        };

        for( const stExpected& e : expected )  {
            bool  bFound = false;

            for( const Event& evt : engineFixture.engine.events )  {
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
