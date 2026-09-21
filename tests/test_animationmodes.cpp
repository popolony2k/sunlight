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
 * The frame sequence of every TextureCanvas animation mode, on a sheet of N frames of 32 px, for N = 3, 4, 5 and 8:
 * the tile drawn at each successive Update() (Advance() then Draw()), starting index 0. Each expected sequence is
 * written as a closed formula in the step number k (1 = the first Update) and checked against a literal for N = 4,
 * so the table a reader sees is the table that is tested.
 *
 *   AUTOMATIC_CIRCULAR      1 2 3 0 1 2 3 0 ...        every frame once per cycle, period N (the first Update shows frame 1)
 *   AUTOMATIC_RIGHT_LEFT    0 1 2 3 2 1 0 1 2 3 2 1 0 ...   ping-pong, each end shown ONCE, period 2N - 2
 *   ANIMATE_RIGHT           0 1 2 3 3 3 ...            to the last frame, then held
 *   ANIMATE_LEFT (start N-1)  N-2 ... 0 0 0 ...        to frame 0, then held
 *   ANIMATE_CENTER          toward the center frame one tile per Update, then held
 *   MANUAL                  the active index, always
 *
 * Before this table was pinned, CIRCULAR stepped ONE PAST the last frame every cycle (1 2 3 [4] 0: an
 * out-of-sheet source rectangle) and RIGHT_LEFT held its ends unevenly (0 1 2 3 3 2 1 0 0 0 1: the last frame
 * twice, frame 0 three times, period 2N + 1).
 */

#include <doctest/doctest.h>
#include <vector>
#include "canvas/texturecanvas.h"
#include "mock_engine.h"

using namespace SunLight :: Canvas;

namespace  {

    const int  g_TileSize = 32;

    // The tile index drawn at each of nSteps successive Update() calls.
    std :: vector<int> Sequence( AnimationMode mode, int nFrames, int nSteps, int nStartIndex = 0, int nCenterIndex = 0 )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.nLoadTextureWidth  = nFrames * g_TileSize;
        fixture.engine.nLoadTextureHeight = g_TileSize;
        REQUIRE( canvas.Load( "sheet.png" ) == true );

        canvas.SetTileSize( g_TileSize );
        canvas.SetAnimationMode( mode );
        canvas.SetCenterTileIndex( nCenterIndex );
        canvas.SetActiveTileIndex( nStartIndex );
        canvas.SetDimension2D( SunLight :: TileMap :: stDimension2D { { 100, 100 }, { g_TileSize, g_TileSize } } );
        canvas.GetViewport().SetDimension2D( SunLight :: TileMap :: stDimension2D { { 0, 0 }, { 800, 600 } } );
        canvas.SetVisible( true );

        std :: vector<int>  tiles;

        for( int nStep = 0; nStep < nSteps; nStep++ )  {
            int  nCalls = fixture.engine.nDrawTextureTiledCalls;

            canvas.Update();
            REQUIRE( fixture.engine.nDrawTextureTiledCalls == nCalls + 1 );          // one draw per step, always

            // The source x is an exact multiple of the tile size, so the division is exact.
            tiles.push_back( ( int ) ( fixture.engine.lastDrawTextureTiledSource.x / g_TileSize ) );
        }

        return tiles;
    }

    const int  g_Ns[] = { 3, 4, 5, 8 };
}

TEST_SUITE( "canvas/animation modes" )  {

    TEST_CASE( "AUTOMATIC_CIRCULAR: every frame once per cycle, period N - never a step past the last frame" )  {

        // The literal, N = 4.
        CHECK( Sequence( TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 4, 10 ) == std :: vector<int> { 1, 2, 3, 0, 1, 2, 3, 0, 1, 2 } );

        for( int nFrames : g_Ns )  {
            INFO( "N = " << nFrames );

            std :: vector<int>  actual = Sequence( TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, nFrames, 4 * nFrames );

            for( int nStep = 1; nStep <= 4 * nFrames; nStep++ )  {
                CHECK( actual[nStep - 1] == nStep % nFrames );
                CHECK( actual[nStep - 1] < nFrames );                                   // always inside the sheet
            }
        }
    }

    TEST_CASE( "AUTOMATIC_RIGHT_LEFT: a ping-pong that shows each end ONCE, period 2N - 2" )  {

        // The literal, N = 4.
        CHECK( Sequence( TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT, 4, 14 ) == std :: vector<int> { 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0, 1 } );

        for( int nFrames : g_Ns )  {
            INFO( "N = " << nFrames );

            int                 nPeriod = 2 * nFrames - 2;
            std :: vector<int>  actual  = Sequence( TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT, nFrames, 3 * nPeriod + 2 );

            for( int nStep = 1; nStep <= 3 * nPeriod + 2; nStep++ )  {
                int  nPhase    = ( nStep - 1 ) % nPeriod;
                int  nExpected = ( nPhase < nFrames ? nPhase : nPeriod - nPhase );

                CHECK( actual[nStep - 1] == nExpected );
                CHECK( actual[nStep - 1] < nFrames );

                // Never the same frame twice in a row: no end is held.
                if( nStep > 1 )
                    CHECK( actual[nStep - 1] != actual[nStep - 2] );
            }
        }
    }

    TEST_CASE( "ANIMATE_RIGHT: to the last frame, then held (unchanged)" )  {

        CHECK( Sequence( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT, 4, 8 ) == std :: vector<int> { 0, 1, 2, 3, 3, 3, 3, 3 } );

        for( int nFrames : g_Ns )  {
            INFO( "N = " << nFrames );

            std :: vector<int>  actual = Sequence( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT, nFrames, 2 * nFrames );

            for( int nStep = 1; nStep <= 2 * nFrames; nStep++ )
                CHECK( actual[nStep - 1] == ( nStep - 1 < nFrames - 1 ? nStep - 1 : nFrames - 1 ) );
        }
    }

    TEST_CASE( "ANIMATE_LEFT (started on the last frame): to frame 0, then held (unchanged)" )  {

        CHECK( Sequence( TEXTURE_ANIMATION_MODE_ANIMATE_LEFT, 4, 8, 3 ) == std :: vector<int> { 2, 1, 0, 0, 0, 0, 0, 0 } );

        for( int nFrames : g_Ns )  {
            INFO( "N = " << nFrames );

            std :: vector<int>  actual = Sequence( TEXTURE_ANIMATION_MODE_ANIMATE_LEFT, nFrames, 2 * nFrames, nFrames - 1 );

            for( int nStep = 1; nStep <= 2 * nFrames; nStep++ )
                CHECK( actual[nStep - 1] == ( nFrames - 1 - nStep > 0 ? nFrames - 1 - nStep : 0 ) );
        }
    }

    TEST_CASE( "ANIMATE_CENTER: one tile per step toward the center frame, then held (unchanged)" )  {

        // Started on frame 0 with the center on frame 2.
        CHECK( Sequence( TEXTURE_ANIMATION_MODE_ANIMATE_CENTER, 4, 6, 0, 2 ) == std :: vector<int> { 1, 2, 2, 2, 2, 2 } );

        // Started past the center: it comes back.
        CHECK( Sequence( TEXTURE_ANIMATION_MODE_ANIMATE_CENTER, 5, 6, 4, 1 ) == std :: vector<int> { 3, 2, 1, 1, 1, 1 } );
    }

    TEST_CASE( "MANUAL: the active index, always (unchanged)" )  {

        for( int nFrames : g_Ns )  {
            INFO( "N = " << nFrames );

            for( int nTile : Sequence( TEXTURE_ANIMATION_MODE_MANUAL, nFrames, 2 * nFrames, nFrames - 1 ) )
                CHECK( nTile == nFrames - 1 );
        }
    }

    TEST_CASE( "A one-frame sheet never leaves frame 0 in any stepping mode" )  {

        for( AnimationMode mode : { TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT,
                                    TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT } )
            for( int nTile : Sequence( mode, 1, 6 ) )
                CHECK( nTile == 0 );
    }
}
