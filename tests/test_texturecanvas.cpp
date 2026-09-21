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

#include <doctest/doctest.h>
#include "canvas/texturecanvas.h"
#include "mock_engine.h"
#include <vector>

using namespace SunLight :: Canvas;
using namespace SunLight :: TileMap;
using namespace SunLight :: Base;

TEST_SUITE( "canvas/TextureCanvas" )  {

    TEST_CASE( "Load forwards the file name to the engine and adopts its reported size" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.nLoadTextureWidth  = 64;
        fixture.engine.nLoadTextureHeight = 32;

        CHECK( canvas.Load( "sprite.png" ) == true );
        CHECK( fixture.engine.nLoadTextureCalls == 1 );
        CHECK( fixture.engine.strLastLoadTextureFileName == "sprite.png" );
        CHECK( canvas.GetDimension2D().size.nWidth  == 64 );
        CHECK( canvas.GetDimension2D().size.nHeight == 32 );
    }

    TEST_CASE( "Load does not override a size the caller already set" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;
        stDimension2D      dim { { 0, 0 }, { 48, 0 } };

        canvas.SetDimension2D( dim );

        fixture.engine.nLoadTextureWidth  = 64;
        fixture.engine.nLoadTextureHeight = 32;

        CHECK( canvas.Load( "sprite.png" ) == true );
        CHECK( canvas.GetDimension2D().size.nWidth  == 48 );
        CHECK( canvas.GetDimension2D().size.nHeight == 0 );
    }

    TEST_CASE( "Load returns false when the engine fails to load the texture" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.hLoadTextureResult = nullptr;

        CHECK( canvas.Load( "missing.png" ) == false );
        CHECK( fixture.engine.nLoadTextureCalls == 1 );
    }

    TEST_CASE( "Unload forwards the loaded handle to the engine exactly once" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0x1234;
        canvas.Load( "sprite.png" );

        CHECK( canvas.Unload() == true );
        CHECK( fixture.engine.nUnloadTextureCalls == 1 );
        CHECK( fixture.engine.hLastUnloadedTexture == ( TextureHandle ) 0x1234 );

        // Handle already cleared - a second Unload() must be a no-op.
        CHECK( canvas.Unload() == false );
        CHECK( fixture.engine.nUnloadTextureCalls == 1 );
    }

    TEST_CASE( "Update draws the loaded texture through the engine when visible and inside the viewport" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;
        stDimension2D      viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D      canvasDim   { { 10, 10 }, { 32, 32 } };

        canvas.GetViewport().SetDimension2D( viewportDim );
        canvas.SetDimension2D( canvasDim );
        canvas.SetVisible( true );

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xABCD;
        canvas.Load( "sprite.png" );

        canvas.Update();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 1 );
        CHECK( fixture.engine.hLastDrawnTexture == ( TextureHandle ) 0xABCD );
    }

    TEST_CASE( "Update does not draw when the canvas is not visible" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;
        stDimension2D      viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D      canvasDim   { { 10, 10 }, { 32, 32 } };

        canvas.GetViewport().SetDimension2D( viewportDim );
        canvas.SetDimension2D( canvasDim );
        canvas.Load( "sprite.png" );

        canvas.Update();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );
    }

    TEST_CASE( "Update does not draw when the canvas falls fully outside the viewport" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;
        stDimension2D      viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D      canvasDim   { { 900, 10 }, { 32, 32 } };

        canvas.GetViewport().SetDimension2D( viewportDim );
        canvas.SetDimension2D( canvasDim );
        canvas.SetVisible( true );
        canvas.Load( "sprite.png" );

        canvas.Update();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );
    }
}

namespace  {

    // One observation of a canvas after a frame step.
    struct Observed  {
        unsigned  nTile;
        int       nDraws;
        float     fSrcX, fSrcW, fDstX, fDstW;

        bool operator == ( const Observed &o ) const  {
            return ( nTile == o.nTile ) && ( nDraws == o.nDraws ) && ( fSrcX == o.fSrcX ) && ( fSrcW == o.fSrcW ) &&
                   ( fDstX == o.fDstX ) && ( fDstW == o.fDstW );
        }
    };

    /**
     * Steps a fresh 64-wide, 16-tile canvas 40 frames in the given animation mode - a Reset() in the middle -
     * either with Update(), or with Advance() then Draw(), and records what each frame did.
     */
    std :: vector<Observed> RunFrames( AnimationMode mode, bool bSplit, int nCanvasX )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.nLoadTextureWidth  = 64;
        fixture.engine.nLoadTextureHeight = 16;
        canvas.Load( "sheet.png" );
        canvas.GetViewport().SetDimension2D( stDimension2D { { 10, 10 }, { 300, 200 } } );
        canvas.SetDimension2D( stDimension2D { { nCanvasX, 20 }, { 16, 16 } } );
        canvas.SetVisible( true );
        canvas.SetTileSize( 16 );
        canvas.SetCenterTileIndex( 2 );
        canvas.SetAnimationMode( mode );

        std :: vector<Observed>  frames;

        for( int nFrame = 0; nFrame < 40; nFrame++ )  {
            if( nFrame == 20 )
                canvas.Reset();

            int  nBefore = fixture.engine.nDrawTextureTiledCalls;

            if( bSplit )  {
                canvas.Advance();
                canvas.Draw();
            }
            else  {
                canvas.Update();
            }

            frames.push_back( Observed { canvas.GetActiveTileIndex(),
                                         fixture.engine.nDrawTextureTiledCalls - nBefore,
                                         fixture.engine.lastDrawTextureTiledSource.x,
                                         fixture.engine.lastDrawTextureTiledSource.width,
                                         fixture.engine.lastDrawTextureTiledDest.x,
                                         fixture.engine.lastDrawTextureTiledDest.width } );
        }

        return frames;
    }
}

TEST_SUITE( "canvas/TextureCanvas Advance/Draw" )  {

    TEST_CASE( "Advance steps the animation but draws nothing" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.nLoadTextureWidth  = 64;
        fixture.engine.nLoadTextureHeight = 16;
        canvas.Load( "sheet.png" );
        canvas.GetViewport().SetDimension2D( stDimension2D { { 0, 0 }, { 800, 600 } } );
        canvas.SetDimension2D( stDimension2D { { 10, 10 }, { 16, 16 } } );
        canvas.SetVisible( true );
        canvas.SetTileSize( 16 );
        canvas.SetAnimationMode( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT );

        CHECK( canvas.GetActiveTileIndex() == 0 );

        canvas.Advance();
        CHECK( canvas.GetActiveTileIndex() == 1 );
        canvas.Advance();
        CHECK( canvas.GetActiveTileIndex() == 2 );

        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );
    }

    TEST_CASE( "Draw draws the current state without changing it, and draws the same frame however often it is called" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.nLoadTextureWidth  = 64;
        fixture.engine.nLoadTextureHeight = 16;
        canvas.Load( "sheet.png" );
        canvas.GetViewport().SetDimension2D( stDimension2D { { 0, 0 }, { 800, 600 } } );
        canvas.SetDimension2D( stDimension2D { { 10, 10 }, { 16, 16 } } );
        canvas.SetVisible( true );
        canvas.SetTileSize( 16 );
        canvas.SetAnimationMode( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT );

        canvas.Advance();
        canvas.Advance();
        unsigned  nTile = canvas.GetActiveTileIndex();

        canvas.Draw();
        float  fSrcX = fixture.engine.lastDrawTextureTiledSource.x;
        float  fDstX = fixture.engine.lastDrawTextureTiledDest.x;

        canvas.Draw();
        canvas.Draw();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 3 );
        CHECK( canvas.GetActiveTileIndex() == nTile );                          // nothing stepped
        CHECK( fixture.engine.lastDrawTextureTiledSource.x == fSrcX );          // ...so it is the same frame
        CHECK( fixture.engine.lastDrawTextureTiledDest.x == fDstX );
        // ANIMATE_RIGHT sets the current tile before it increments the index, so after two steps (index 2)
        // the frame on screen is tile 1 - long-standing behaviour, unchanged.
        CHECK( fSrcX == 16.0f );
    }

    TEST_CASE( "Update is exactly Advance then Draw, in every animation mode, on screen and partly off it" )  {

        AnimationMode  aModes[] = { TEXTURE_ANIMATION_MODE_MANUAL,
                                                     TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR,
                                                     TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT,
                                                     TEXTURE_ANIMATION_MODE_ANIMATE_LEFT,
                                                     TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT,
                                                     TEXTURE_ANIMATION_MODE_ANIMATE_CENTER };
        long  nFramesCompared = 0;
        long  nMismatches     = 0;
        long  nDrawn          = 0;

        for( AnimationMode mode : aModes )  {
            for( int nCanvasX : { 20, 290, 305, 400, -5 } )  {            // inside, straddling the right edge, outside, before the origin
                std :: vector<Observed>  viaUpdate = RunFrames( mode, false, nCanvasX );
                std :: vector<Observed>  viaSplit  = RunFrames( mode, true, nCanvasX );

                REQUIRE( viaUpdate.size() == viaSplit.size() );

                for( size_t nFrame = 0; nFrame < viaUpdate.size(); nFrame++ )  {
                    nFramesCompared++;

                    if( !( viaUpdate[nFrame] == viaSplit[nFrame] ) )
                        nMismatches++;

                    nDrawn += viaUpdate[nFrame].nDraws;
                }
            }
        }

        CHECK( nFramesCompared == 1200 );
        CHECK( nDrawn > 600 );              // real drawing happened in the comparison, not just rejections
        CHECK( nMismatches == 0 );
    }

    TEST_CASE( "An animation that is off screen or invisible does not step in Advance and does not draw in Draw" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;

        fixture.engine.nLoadTextureWidth  = 64;
        fixture.engine.nLoadTextureHeight = 16;
        canvas.Load( "sheet.png" );
        canvas.GetViewport().SetDimension2D( stDimension2D { { 0, 0 }, { 800, 600 } } );
        canvas.SetTileSize( 16 );
        canvas.SetAnimationMode( TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT );

        // Fully outside the viewport.
        canvas.SetDimension2D( stDimension2D { { 900, 10 }, { 16, 16 } } );
        canvas.SetVisible( true );
        canvas.Advance();
        canvas.Draw();
        CHECK( canvas.GetActiveTileIndex() == 0 );
        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );

        // Inside, but invisible.
        canvas.SetDimension2D( stDimension2D { { 10, 10 }, { 16, 16 } } );
        canvas.SetVisible( false );
        canvas.Advance();
        canvas.Draw();
        CHECK( canvas.GetActiveTileIndex() == 0 );
        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );

        // Back on screen and visible: it steps and draws again.
        canvas.SetVisible( true );
        canvas.Advance();
        canvas.Draw();
        CHECK( canvas.GetActiveTileIndex() == 1 );
        CHECK( fixture.engine.nDrawTextureTiledCalls == 1 );
    }
}
