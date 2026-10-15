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
 * that means for everything in the renderer that reads it: a shape's clip rectangle,
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

        SunLight :: Base :: stDimension2D  viewport {};

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

        SunLight :: Base :: stDimension2D  viewport {};

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

    TEST_CASE( "MoveCameraLeft/Up scroll until the map's far edge reaches the viewport's, and never past it" )  {

        // Checked step by step, at four viewports and zooms (60 is 3.8125, non-integer), against
        // what the limit means rather than a copy of its formula. A map position p is drawn at
        // (p + camera) x zoom + viewport.pos; the map is 640 x 640. After every step:
        // - the map's far edge is at or past the viewport's (no gap of the clear colour);
        // - the camera moved by at most one scroll step, and stays a whole number;
        // - once it stops moving, one more map pixel would open a gap: it went all the way.
        // The old rule refused a step that went past the edge, so it stopped up to a step short.
        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        VpCase      aCases[] = { { 10, 10, 1240, 900, 60 }, { 0, 0, 300, 300, 15 }, { 25, 40, 875, 660, 30 }, { 10, 10, 400, 350, 30 } };
        const int   nStep    = 16;        // LoadMap sets the scroll step to the tile size
        const int   nMapSize = 640;
        const int   nIters   = 400;       // far more steps than any case needs to reach its edge
        long        nCompared = 0;
        long        nMovedX   = 0;
        long        nMovedY   = 0;

        for( const VpCase &v : aCases )  {
            std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( v, ITM :: MAP_ALIGNMENT_TOP_LEFT );
            float                               fZoom     = pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;
            int                                 nFarX     = v.nPosX + v.nWidth;
            int                                 nFarY     = v.nPosY + v.nHeight;
            int                                 nPrevX    = 0, nPrevY = 0;

            // The far edge's screen position for a camera; GetCameraPosition reports -camera.
            auto  edge = [fZoom, nMapSize]( int nReported, int nPos )  {
                return ( ( -nReported + nMapSize ) * fZoom ) + nPos;
            };

            pRenderer -> GetCameraPosition( nPrevX, nPrevY );

            for( int nIter = 0; nIter < nIters; nIter++ )  {
                pRenderer -> MoveCameraLeft();
                pRenderer -> MoveCameraUp();

                int  nX = 0, nY = 0;

                pRenderer -> GetCameraPosition( nX, nY );
                nCompared++;

                CHECK( edge( nX, v.nPosX ) >= nFarX );
                CHECK( edge( nY, v.nPosY ) >= nFarY );
                CHECK( ( nX - nPrevX ) >= 0 );
                CHECK( ( nX - nPrevX ) <= nStep );
                CHECK( ( nY - nPrevY ) >= 0 );
                CHECK( ( nY - nPrevY ) <= nStep );

                if( nX == nPrevX )
                    CHECK( edge( nX + 1, v.nPosX ) < nFarX );
                else
                    nMovedX++;

                if( nY == nPrevY )
                    CHECK( edge( nY + 1, v.nPosY ) < nFarY );
                else
                    nMovedY++;

                nPrevX = nX;
                nPrevY = nY;
            }

            pRenderer -> Stop();
        }

        CHECK( nCompared == 4 * nIters );
        CHECK( nMovedX > 40 );            // the scrolling really happens in both directions (not vacuous)
        CHECK( nMovedY > 40 );
    }

    TEST_CASE( "TileMapToTileMatrix uses the viewport's pos.y for the row (it used pos.x)" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        // pos.x != pos.y is what tells them apart: (10, 40).
        VpCase                              v { 10, 40, 500, 500, 15 };
        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( v, ITM :: MAP_ALIGNMENT_TOP_LEFT );
        SunLight :: Base :: stCoordinate2D    coord { 100, 100 };
        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

        REQUIRE( pRenderer -> TileMapToTileMatrix( coord, pos ) == true );
        CHECK( pos.nTileCol == ( 100 + 10 ) / 16 );        // 6
        CHECK( pos.nTileRow == ( 100 + 40 ) / 16 );        // 8   (was (100 + 10) / 16 = 6)

        SunLight :: Base :: stCoordinate2D    coord2 { 20, 30 };

        REQUIRE( pRenderer -> TileMapToTileMatrix( coord2, pos ) == true );
        CHECK( pos.nTileCol == ( 20 + 10 ) / 16 );         // 1
        CHECK( pos.nTileRow == ( 30 + 40 ) / 16 );         // 4

        pRenderer -> Stop();
    }

    TEST_CASE( "A shape's clip covers [pos, pos + size) minus the top/left edge, so it reaches the viewport's true right/bottom edge" )  {

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

        SunLight :: Base :: stDimension2D  viewport {};

        viewport.pos.x = 10;  viewport.pos.y = 10;  viewport.size.nWidth = 100;  viewport.size.nHeight = 100;
        config.viewport = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        windowFixture.window.nFramesUntilShouldClose = 1;
        REQUIRE( pRenderer -> Start() == true );
        REQUIRE( pRenderer -> LoadMap( "maps/shape.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        pRenderer -> Run();

        // The frame's clip is the viewport (10, 10, 100, 100); the shape's own clip is the one from
        // PrimitiveClip, which starts one pixel inside the top/left edge (see __PRIMITIVE_EDGE_INSET).
        const std :: vector<MockEngine :: Event>  &events = engineFixture.engine.events;
        const MockEngine :: Event                 *pShapeClip = nullptr;

        for( const MockEngine :: Event &evt : events )
            if( ( evt.kind == MockEngine :: Event :: CLIP_BEGIN ) && ( evt.x == 11.0f ) )
                pShapeClip = &evt;

        REQUIRE( pShapeClip != nullptr );

        // Top/left edge excluded (first pixel 11), far edge reached: x + w and y + h are the first
        // pixel OUTSIDE the clip, so the last visible column and row are 109 - not 99 (size as far edge).
        CHECK( pShapeClip -> x == 11.0f );
        CHECK( pShapeClip -> y == 11.0f );
        CHECK( pShapeClip -> x + pShapeClip -> w == 110.0f );
        CHECK( pShapeClip -> y + pShapeClip -> h == 110.0f );

        // The rectangle's top-left corner is drawn at screen (60, 60), where the clip starts cutting it.
        bool bCornerDrawn = false;

        for( const MockEngine :: Event &evt : events )
            if( ( evt.kind == MockEngine :: Event :: LINE ) && ( evt.x == 60.0f ) && ( evt.y == 60.0f ) )
                bCornerDrawn = true;

        CHECK( bCornerDrawn );

        pRenderer -> Stop();
    }
}
