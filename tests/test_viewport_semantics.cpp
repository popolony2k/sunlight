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
 * The viewport is the rectangle [pos, pos + size) - size is a width/height, not the
 * coordinate of the far edge (which it was, in effect, before v0.29.0). These pin what
 * that means for everything in the renderer that reads it: SetPixel's visible area,
 * where LoadMap's alignments put the camera (incl. the modulo "snap to a whole viewport
 * height" of the bottom alignments), how far MoveCameraLeft/Up may scroll, and the
 * screen -> tile conversion. Every expectation is either an explicit number or the
 * pre-v0.29.0 code copied as an ORACLE, fed the same visible area as legacy far-edge
 * coordinates - so "the same visible area gives the same result" is checked directly.
 */

#include <doctest/doctest.h>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    struct VpCase  {
        int       nPosX, nPosY, nWidth, nHeight;
        unsigned  nZoomPos;
    };

    // Null-backend renderer, viewport + zoom as given, a 640 x 640 map (40 x 40 tiles of 16)
    // loaded with the given alignment.
    std :: unique_ptr<TileMapRenderer> MakeRenderer( const VpCase &v, int nAlignment )  {

        RendererConfig  config;

        config.backend     = RENDERER_BACKEND_NULL;
        config.framePacing = FRAME_PACING_UNLIMITED;
        config.fWidth      = 1260.0f;
        config.fHeight     = 920.0f;
        config.nZoomPos    = v.nZoomPos;

        SunLight :: TileMap :: stDimension2D  viewport {};

        viewport.pos.x        = v.nPosX;
        viewport.pos.y        = v.nPosY;
        viewport.size.nWidth  = v.nWidth;
        viewport.size.nHeight = v.nHeight;
        config.viewport = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );
        REQUIRE( pRenderer -> LoadMap( "maps/square.tmx", ( ITM :: MapAlignment ) nAlignment ) == true );

        return pRenderer;
    }

    /**
     * The pre-v0.29.0 alignment code, verbatim, with its "vp.size" being the far-edge
     * coordinate (nFarX, nFarY) - the oracle for LoadMap. Returns the camera the way
     * GetCameraPosition reports it.
     */
    void LegacyAlignment( int nAlignment, int nMapW, int nMapH, int nPosX, int nPosY,
                          int nFarX, int nFarY, float fZoomFactor, int &nCamX, int &nCamY )  {

        float  fCamX = 0.0f;
        float  fCamY = 0.0f;
        int    nVpHeight = std :: abs( ( ( nMapH / nFarY ) * nFarY ) - nMapH );

        switch( nAlignment & 0xFF )  {
            case ITM :: MAP_ALIGNMENT_TOP_RIGHT :
                fCamX = ( float ) -( nMapW - nFarX + nPosX );
                break;

            case ITM :: MAP_ALIGNMENT_TOP_LEFT :
                break;

            case ITM :: MAP_ALIGNMENT_BOTTOM_RIGHT :
                fCamX = ( float ) -( nMapW - nFarX + nPosX );
                fCamY = ( float ) -( nMapH - nVpHeight + nPosY );
                break;

            case ITM :: MAP_ALIGNMENT_BOTTOM_LEFT :
                fCamY = ( float ) -( nMapH - nFarY + nPosY );
                break;

            case ITM :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM :
                fCamX = -round( ( ( ( ( nMapW * fZoomFactor ) - nFarX + nPosX ) / 2 ) / fZoomFactor ) );
                fCamY = ( float ) -( nMapH - nVpHeight + nPosY );
                break;

            case ITM :: MAP_ALIGNMENT_CENTER_WIDTH_TOP :
                fCamX = -round( ( ( ( ( nMapW * fZoomFactor ) - nFarX + nPosX ) / 2 ) / fZoomFactor ) );
                break;

            case ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT :
                fCamY = -round( ( ( ( ( nMapH * fZoomFactor ) - nFarY + nPosY ) / 2 ) / fZoomFactor ) );
                break;

            case ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT :
                fCamX = ( float ) -( nMapW - nFarX + nPosX );
                fCamY = -round( ( ( ( ( nMapH * fZoomFactor ) - nFarY + nPosY ) / 2 ) / fZoomFactor ) );
                break;

            case ITM :: MAP_ALIGNMENT_CENTER :
                fCamX = -round( ( ( ( ( nMapW * fZoomFactor ) - nFarX + nPosX ) / 2 ) / fZoomFactor ) );
                fCamY = -round( ( ( ( ( nMapH * fZoomFactor ) - nFarY + nPosY ) / 2 ) / fZoomFactor ) );
                break;
        }

        nCamX = ( int ) -fCamX;
        nCamY = ( int ) -fCamY;
    }

    const int  g_Alignments[] = { ITM :: MAP_ALIGNMENT_CENTER, ITM :: MAP_ALIGNMENT_TOP_RIGHT, ITM :: MAP_ALIGNMENT_TOP_LEFT,
                                  ITM :: MAP_ALIGNMENT_BOTTOM_RIGHT, ITM :: MAP_ALIGNMENT_BOTTOM_LEFT,
                                  ITM :: MAP_ALIGNMENT_CENTER_WIDTH_TOP, ITM :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM,
                                  ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT, ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT };
}

TEST_SUITE( "renderer/viewport semantics" )  {

    TEST_CASE( "LoadMap alignments at Caravellius' viewport (10, 10, 1240, 900), zoom 3.8125: pinned camera positions" )  {

        // 640 x 640 map. Legacy (10, 10, 1250, 910) gave exactly these; (10, 10, 1240, 900) shows the same
        // area and must too. Note BOTTOM_RIGHT / CENTER_WIDTH_BOTTOM: y = 10, not 260 - they snap to a
        // whole multiple of the viewport's bottom edge (910): 640 mod 910 = 640, so map height - 640 = 0.
        struct  { int nAlignment; int nX; int nY; }  expected[] = {
            { ITM :: MAP_ALIGNMENT_CENTER,               157,  202 },
            { ITM :: MAP_ALIGNMENT_TOP_RIGHT,           -600,    0 },
            { ITM :: MAP_ALIGNMENT_TOP_LEFT,               0,    0 },
            { ITM :: MAP_ALIGNMENT_BOTTOM_RIGHT,        -600,   10 },
            { ITM :: MAP_ALIGNMENT_BOTTOM_LEFT,            0, -260 },
            { ITM :: MAP_ALIGNMENT_CENTER_WIDTH_TOP,     157,    0 },
            { ITM :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM,  157,   10 },
            { ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT,     0,  202 },
            { ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT, -600,  202 },
        };

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        for( const auto &e : expected )  {
            VpCase                              v { 10, 10, 1240, 900, 60 };
            std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( v, e.nAlignment );
            int                                 nX = 12345, nY = 12345;

            pRenderer -> GetCameraPosition( nX, nY );

            INFO( "alignment " << e.nAlignment );
            CHECK( nX == e.nX );
            CHECK( nY == e.nY );

            pRenderer -> Stop();
        }
    }

    TEST_CASE( "LoadMap alignments equal the legacy code on the same visible area - every alignment, several viewports and zooms" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        // (pos x, pos y, width, height, zoom position): Caravellius', the samples', origin-anchored, odd.
        VpCase  aCases[] = { { 10, 10, 1240, 900, 60 }, { 10, 10, 890, 790, 15 }, { 0, 0, 800, 600, 30 },
                             { 0, 0, 1260, 920, 60 },   { 25, 40, 875, 660, 30 }, { 7, 3, 633, 477, 15 },
                             { 100, 100, 200, 200, 60 } };
        long    nCompared   = 0;
        long    nMismatches = 0;

        for( const VpCase &v : aCases )  {
            for( int nAlignment : g_Alignments )  {
                std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( v, nAlignment );
                int                                 nX = 0, nY = 0, nExpX = 0, nExpY = 0;
                float                               fZoom = pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;

                pRenderer -> GetCameraPosition( nX, nY );
                LegacyAlignment( nAlignment, 640, 640, v.nPosX, v.nPosY, v.nPosX + v.nWidth, v.nPosY + v.nHeight, fZoom, nExpX, nExpY );

                nCompared++;

                if( ( nX != nExpX ) || ( nY != nExpY ) )
                    nMismatches++;

                pRenderer -> Stop();
            }
        }

        CHECK( nCompared == 63 );
        CHECK( nMismatches == 0 );
    }

    TEST_CASE( "The bottom alignments snap to a whole multiple of the viewport's bottom edge (pos.y + height), so a taller map lands further along" )  {

        // BOTTOM_RIGHT / CENTER_WIDTH_BOTTOM put the camera at (map height - map height mod bottom edge + pos.y).
        // Map 1920 tall, viewport (10, 10, 1240, 900) -> bottom edge 910: 1920 mod 910 = 100, so y = -(1920 - 100 + 10) = -1830.
        // (Legacy (10, 10, 1250, 910) gave the same, because it IS the same bottom edge.)
        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/tall.tmx"] = MakeSquareTmx( 120, 16 );          // 1920 x 1920

        RendererConfig  config;

        config.backend     = RENDERER_BACKEND_NULL;
        config.framePacing = FRAME_PACING_UNLIMITED;
        config.fWidth      = 1260.0f;
        config.fHeight     = 920.0f;
        config.nZoomPos    = 60;

        SunLight :: TileMap :: stDimension2D  viewport {};

        viewport.pos.x = 10;  viewport.pos.y = 10;  viewport.size.nWidth = 1240;  viewport.size.nHeight = 900;
        config.viewport = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );
        REQUIRE( pRenderer -> LoadMap( "maps/tall.tmx", ITM :: MAP_ALIGNMENT_BOTTOM_RIGHT ) == true );

        int  nX = 0, nY = 0;

        pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nY == 1830 );                       // GetCameraPosition reports the negated camera: camera y = -1830
        pRenderer -> Stop();
    }

    TEST_CASE( "MoveCameraLeft/Up limits equal the legacy code on the same visible area, step by step" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        VpCase  aCases[] = { { 10, 10, 1240, 900, 60 }, { 0, 0, 300, 300, 15 }, { 25, 40, 875, 660, 30 }, { 10, 10, 400, 350, 30 } };
        long    nCompared   = 0;
        long    nMismatches = 0;
        long    nMoved      = 0;
        long    nMovedUp    = 0;

        for( const VpCase &v : aCases )  {
            std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( v, ITM :: MAP_ALIGNMENT_TOP_LEFT );
            float                               fZoom     = pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;
            int                                 nFarX     = v.nPosX + v.nWidth;
            int                                 nFarY     = v.nPosY + v.nHeight;
            float                               fCamX     = 0.0f;      // the legacy simulation's camera
            float                               fCamY     = 0.0f;
            const int                           nStep     = 16;        // LoadMap sets the scroll step to the tile size
            int                                 nErrorFix = ( int ) ( 16 / std :: round( fZoom ) );

            for( int nIter = 0; nIter < 400; nIter++ )  {
                // Legacy MoveCameraLeft / MoveCameraUp, verbatim, against the far edges.
                int  nBoundX = ( int ) std :: round( ( fCamX - nStep + 640 + nErrorFix ) * fZoom );
                int  nBoundY = ( int ) std :: round( ( fCamY - nStep + 640 + 16 ) * fZoom );

                if( nFarX < nBoundX )  {
                    fCamX-=nStep;
                    nMoved++;
                }

                if( nFarY < nBoundY )  {
                    fCamY-=nStep;
                    nMovedUp++;
                }

                pRenderer -> MoveCameraLeft();
                pRenderer -> MoveCameraUp();

                int  nX = 0, nY = 0;

                pRenderer -> GetCameraPosition( nX, nY );
                nCompared++;

                if( ( nX != ( int ) -fCamX ) || ( nY != ( int ) -fCamY ) )
                    nMismatches++;
            }

            pRenderer -> Stop();
        }

        CHECK( nCompared == 1600 );
        CHECK( nMoved > 40 );             // the scrolling really happens in both directions (not vacuous)
        CHECK( nMovedUp > 40 );
        CHECK( nMismatches == 0 );
    }

    TEST_CASE( "TileMapToTileMatrix uses the viewport's pos.y for the row (it used pos.x)" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        // pos.x != pos.y is what tells them apart: (10, 40).
        VpCase                              v { 10, 40, 500, 500, 15 };
        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( v, ITM :: MAP_ALIGNMENT_TOP_LEFT );
        SunLight :: TileMap :: stCoordinate2D    coord { 100, 100 };
        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

        REQUIRE( pRenderer -> TileMapToTileMatrix( coord, pos ) == true );
        CHECK( pos.nTileCol == ( 100 + 10 ) / 16 );        // 6
        CHECK( pos.nTileRow == ( 100 + 40 ) / 16 );        // 8   (was (100 + 10) / 16 = 6)

        SunLight :: TileMap :: stCoordinate2D    coord2 { 20, 30 };

        REQUIRE( pRenderer -> TileMapToTileMatrix( coord2, pos ) == true );
        CHECK( pos.nTileCol == ( 20 + 10 ) / 16 );         // 1
        CHECK( pos.nTileRow == ( 30 + 40 ) / 16 );         // 4

        pRenderer -> Stop();
    }

    TEST_CASE( "SetPixel draws inside [pos, pos + size) and nowhere else, so a shape reaches the viewport's true right/bottom edge" )  {

        // Viewport (10, 10, 100, 100) shows x, y in [10, 110). A rectangle object at (50, 50), 200 x 200 world
        // pixels, drawn at zoom 1 / camera 0 starts at screen (60, 60): its top and left lines cross the
        // viewport and must be cut at 109 - not at 99, which is where reading size as the far edge cut them.
        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;

        fsFixture.fs.files["maps/shape.tmx"] = MakeSquareTmx( 40, 16, true, 50, 50, 200, 200 );

        RendererConfig  config;

        config.fWidth  = 1260.0f;
        config.fHeight = 920.0f;

        SunLight :: TileMap :: stDimension2D  viewport {};

        viewport.pos.x = 10;  viewport.pos.y = 10;  viewport.size.nWidth = 100;  viewport.size.nHeight = 100;
        config.viewport = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        windowFixture.window.nFramesUntilShouldClose = 1;
        REQUIRE( pRenderer -> Start() == true );
        REQUIRE( pRenderer -> LoadMap( "maps/shape.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        pRenderer -> Run();

        const std :: vector<std :: pair<int, int>>  &pixels = engineFixture.engine.setPixelPositions;

        REQUIRE( pixels.size() > 0 );

        int  nMinX = 100000, nMaxX = -100000, nMinY = 100000, nMaxY = -100000;

        for( const auto &pixel : pixels )  {
            nMinX = std :: min( nMinX, pixel.first );
            nMaxX = std :: max( nMaxX, pixel.first );
            nMinY = std :: min( nMinY, pixel.second );
            nMaxY = std :: max( nMaxY, pixel.second );
        }

        // Never outside the visible rectangle (its top/left edge itself excluded, as ever)...
        CHECK( nMinX > 10 );
        CHECK( nMinY > 10 );
        CHECK( nMaxX < 110 );
        CHECK( nMaxY < 110 );

        // ...and it reaches all the way to the last visible column and row.
        CHECK( nMinX == 60 );
        CHECK( nMinY == 60 );
        CHECK( nMaxX == 109 );
        CHECK( nMaxY == 109 );

        pRenderer -> Stop();
    }
}
