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
 * TileMapRenderer's camera scrolling against a real (tiny, in-memory) TMX
 * map, run on the null backend - no display. The boundary checks compare the
 * VIEWPORT size against the map: horizontal scrolling must use the
 * viewport's width, vertical scrolling its height. These use deliberately
 * non-square viewports, where mixing them up is visible.
 */

#include <doctest/doctest.h>
#include <string>
#include <memory>
#include "renderer/tilemaprenderer.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;

namespace  {

    // A 40 x 40 tile map of 16 x 16 tiles: 640 x 640 pixels, all empty.
    Bytes MakeSquareMap( void )  {

        return MakeSquareTmx( 40, 16 );
    }

    // A null-backend renderer with the given viewport, the map loaded at the
    // top-left (camera at the origin), scroll step = the 16 px tile size.
    std :: unique_ptr<TileMapRenderer> MakeRenderer( int nViewportWidth, int nViewportHeight )  {

        RendererConfig  config;

        config.backend     = RENDERER_BACKEND_NULL;
        config.framePacing = FRAME_PACING_UNLIMITED;
        config.fWidth      = 1000.0f;
        config.fHeight     = 1000.0f;

        SunLight :: TileMap :: stDimension2D  viewport {};

        viewport.size.nWidth  = nViewportWidth;
        viewport.size.nHeight = nViewportHeight;
        config.viewport = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );
        REQUIRE( pRenderer -> LoadMap( "maps/square.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        return pRenderer;
    }
}

TEST_SUITE( "renderer/TileMapRenderer camera" )  {

    TEST_CASE( "MoveCameraUp is bounded by the viewport HEIGHT: a wide-but-short viewport scrolls up" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareMap();

        // 1000 wide, 100 tall over a 640 x 640 map: there IS vertical room
        // to scroll (the viewport is shorter than the map) even though the
        // viewport is wider than the whole map.
        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( 1000, 100 );
        int                                 nX0 = 0, nY0 = 0, nX1 = 0, nY1 = 0;

        pRenderer -> GetCameraPosition( nX0, nY0 );
        pRenderer -> MoveCameraUp();
        pRenderer -> GetCameraPosition( nX1, nY1 );

        CHECK( nX1 == nX0 );
        CHECK( nY1 == nY0 + 16 );        // moved one scroll step (the tile height)

        pRenderer -> Stop();
    }

    TEST_CASE( "MoveCameraUp does not scroll when the viewport is taller than the map, however narrow it is" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareMap();

        // 100 wide, 1000 tall: the viewport already shows the whole map
        // vertically, so there's nothing to scroll up to - even though its
        // WIDTH (100) is smaller than the map.
        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( 100, 1000 );
        int                                 nX0 = 0, nY0 = 0, nX1 = 0, nY1 = 0;

        pRenderer -> GetCameraPosition( nX0, nY0 );
        pRenderer -> MoveCameraUp();
        pRenderer -> GetCameraPosition( nX1, nY1 );

        CHECK( nY1 == nY0 );
        CHECK( nX1 == nX0 );

        pRenderer -> Stop();
    }

    TEST_CASE( "MoveCameraLeft is bounded by the viewport WIDTH (the horizontal counterpart, already correct)" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareMap();

        int  nX0 = 0, nY0 = 0, nX1 = 0, nY1 = 0;

        {
            // Narrow-but-tall: there is horizontal room -> scrolls.
            std :: unique_ptr<TileMapRenderer>  pNarrow = MakeRenderer( 100, 1000 );

            pNarrow -> GetCameraPosition( nX0, nY0 );
            pNarrow -> MoveCameraLeft();
            pNarrow -> GetCameraPosition( nX1, nY1 );

            CHECK( nX1 == nX0 + 16 );
            CHECK( nY1 == nY0 );
            pNarrow -> Stop();
        }

        {
            // Wide-but-short: the viewport already shows the whole map
            // horizontally -> doesn't scroll.
            std :: unique_ptr<TileMapRenderer>  pWide = MakeRenderer( 1000, 100 );

            pWide -> GetCameraPosition( nX0, nY0 );
            pWide -> MoveCameraLeft();
            pWide -> GetCameraPosition( nX1, nY1 );

            CHECK( nX1 == nX0 );
            pWide -> Stop();
        }
    }
}
