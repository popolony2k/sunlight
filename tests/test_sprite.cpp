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
#include "sprite/sprite.h"
#include "mock_engine.h"
#include <algorithm>
#include <vector>
#include "mock_clock.h"

using namespace SunLight :: Sprite;
using namespace SunLight :: Canvas;
using namespace SunLight :: TileMap;
using namespace SunLight :: Base;

TEST_SUITE( "sprite/Sprite" )  {

    TEST_CASE( "AddTextureSequence adopts the sprite's size/position from the texture when both are still zero" )  {

        Sprite         sprite;
        TextureCanvas  canvas;
        stDimension2D  texDim { { 5, 6 }, { 32, 32 } };

        canvas.SetDimension2D( texDim );
        sprite.AddTextureSequence( 0, &canvas );

        CHECK( sprite.GetDimension2D().pos.x       == 5 );
        CHECK( sprite.GetDimension2D().pos.y       == 6 );
        CHECK( sprite.GetDimension2D().size.nWidth  == 32 );
        CHECK( sprite.GetDimension2D().size.nHeight == 32 );
    }

    TEST_CASE( "AddTextureSequence does not override a size/position the sprite already has" )  {

        Sprite         sprite;
        TextureCanvas  canvas;
        stDimension2D  spriteDim { { 100, 100 }, { 16, 16 } };
        stDimension2D  texDim    { { 5, 6 },     { 32, 32 } };

        sprite.SetDimension2D( spriteDim );
        canvas.SetDimension2D( texDim );

        sprite.AddTextureSequence( 0, &canvas );

        CHECK( sprite.GetDimension2D().pos.x       == 100 );
        CHECK( sprite.GetDimension2D().pos.y       == 100 );
        CHECK( sprite.GetDimension2D().size.nWidth  == 16 );
        CHECK( sprite.GetDimension2D().size.nHeight == 16 );
    }

    TEST_CASE( "AddTextureSequence aliases the texture's dimension to the sprite's own" )  {

        Sprite         sprite;
        TextureCanvas  canvas;
        stCoordinate2D step { 5, 5 };

        sprite.AddTextureSequence( 0, &canvas );
        sprite.Move( step );

        CHECK( canvas.GetDimension2D().pos.x == sprite.GetDimension2D().pos.x );
        CHECK( canvas.GetDimension2D().pos.y == sprite.GetDimension2D().pos.y );
    }

    TEST_CASE( "GetActiveTextureSequence/GetActiveTexture report -1/nullptr before any sequence is selected" )  {

        Sprite  sprite;

        CHECK( sprite.GetActiveTextureSequence() == -1 );
        CHECK( sprite.GetActiveTexture() == nullptr );
    }

    TEST_CASE( "SetActiveTextureSequence selects a registered sequence and rejects an unknown one" )  {

        Sprite         sprite;
        TextureCanvas  canvas;

        sprite.AddTextureSequence( 0, &canvas );

        CHECK( sprite.SetActiveTextureSequence( 0 ) == true );
        CHECK( sprite.GetActiveTextureSequence() == 0 );
        CHECK( sprite.GetActiveTexture() == &canvas );

        CHECK( sprite.SetActiveTextureSequence( 99 ) == false );
        CHECK( sprite.GetActiveTextureSequence() == -1 );
        CHECK( sprite.GetActiveTexture() == nullptr );
    }

    TEST_CASE( "SetVisible propagates to every texture across all sequences" )  {

        Sprite         sprite;
        TextureCanvas  canvasA, canvasB;

        sprite.AddTextureSequence( 0, &canvasA );
        sprite.AddTextureSequence( 1, &canvasB );

        sprite.SetVisible( true );
        CHECK( canvasA.GetVisible() == true );
        CHECK( canvasB.GetVisible() == true );

        sprite.SetVisible( false );
        CHECK( canvasA.GetVisible() == false );
        CHECK( canvasB.GetVisible() == false );
    }

    TEST_CASE( "Move offsets the sprite's own position by the given step" )  {

        Sprite         sprite;
        stDimension2D  dim  { { 10, 10 }, { 32, 32 } };
        stCoordinate2D step { 5, -3 };

        sprite.SetDimension2D( dim );
        sprite.Move( step );

        CHECK( sprite.GetDimension2D().pos.x == 15 );
        CHECK( sprite.GetDimension2D().pos.y == 7 );
    }

    TEST_CASE( "Update draws the active texture through the engine when visible" )  {

        MockEngineFixture  fixture;
        Sprite             sprite;
        TextureCanvas      canvas;
        stDimension2D      viewportDim { { 0, 0 }, { 800, 600 } };

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        canvas.Load( "sprite.png" );

        // AddTextureSequence() parents the texture under the sprite -
        // BaseCanvas::GetViewport() delegates to the parent's viewport once
        // a parent is set, so the viewport must be configured through the
        // sprite (or on the canvas after parenting, same object either way).
        sprite.AddTextureSequence( 0, &canvas );
        sprite.GetViewport().SetDimension2D( viewportDim );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetDimension2D( stDimension2D { { 10, 10 }, { 32, 32 } } );

        sprite.Update();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 1 );
    }

    TEST_CASE( "Update does not draw when the sprite is not visible" )  {

        MockEngineFixture  fixture;
        Sprite             sprite;
        TextureCanvas      canvas;
        stDimension2D      viewportDim { { 0, 0 }, { 800, 600 } };

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        canvas.Load( "sprite.png" );

        sprite.AddTextureSequence( 0, &canvas );
        sprite.GetViewport().SetDimension2D( viewportDim );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetDimension2D( stDimension2D { { 10, 10 }, { 32, 32 } } );

        sprite.Update();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );
    }

    TEST_CASE( "Unload unloads every texture across all sequences" )  {

        MockEngineFixture  fixture;
        Sprite             sprite;
        TextureCanvas      canvasA, canvasB;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0x1;
        canvasA.Load( "a.png" );
        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0x2;
        canvasB.Load( "b.png" );

        sprite.AddTextureSequence( 0, &canvasA );
        sprite.AddTextureSequence( 1, &canvasB );

        sprite.Unload();

        CHECK( fixture.engine.nUnloadTextureCalls == 2 );
    }

    TEST_CASE( "Advance lets the texture map pick its frame; Draw never does, however often it is called" )  {

        MockClockFixture   clockFixture;
        MockEngineFixture  fixture;
        Sprite             sprite;
        TextureCanvas      frameA;
        TextureCanvas      frameB;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        frameA.Load( "a.png" );
        frameB.Load( "b.png" );

        clockFixture.clock.nNow = 1000;
        sprite.AddTextureSequence( 0, &frameA, 100 );      // each frame lasts 100 ms
        sprite.AddTextureSequence( 0, &frameB, 100 );
        sprite.GetViewport().SetDimension2D( stDimension2D { { 0, 0 }, { 800, 600 } } );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetDimension2D( stDimension2D { { 10, 10 }, { 32, 32 } } );

        CHECK( sprite.GetActiveTexture() == &frameA );

        // Not due yet (1099 < 1100): the frame is held.
        clockFixture.clock.nNow = 1099;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &frameA );

        // Due: Advance steps to the next frame.
        clockFixture.clock.nNow = 1100;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &frameB );

        // Drawing any number of times - even far in the future - never steps it.
        clockFixture.clock.nNow = 100000;
        int  nDrawsBefore = fixture.engine.nDrawTextureTiledCalls;

        sprite.Draw();
        sprite.Draw();
        sprite.Draw();

        CHECK( sprite.GetActiveTexture() == &frameB );
        CHECK( fixture.engine.nDrawTextureTiledCalls - nDrawsBefore == 3 );
    }

    TEST_CASE( "Advance draws nothing" )  {

        MockClockFixture   clockFixture;
        MockEngineFixture  fixture;
        Sprite             sprite;
        TextureCanvas      canvas;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        canvas.Load( "sprite.png" );

        sprite.AddTextureSequence( 0, &canvas );
        sprite.GetViewport().SetDimension2D( stDimension2D { { 0, 0 }, { 800, 600 } } );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetDimension2D( stDimension2D { { 10, 10 }, { 32, 32 } } );

        sprite.Advance();
        sprite.Advance();

        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );
    }

    TEST_CASE( "Update is exactly Advance then Draw: same frames, same draw calls, over a run with held and stepped frames" )  {

        // Two identical sprites (a 4-tile sheet each, three frames per sequence, 20 ms per frame) stepped
        // 7 ms at a time, one with Update() and one with Advance() + Draw(): every frame's draw calls and the
        // active texture must match. Frames alternate between held (the map isn't due) and stepped.
        struct Run  {
            std :: vector<int>    draws;
            std :: vector<float>  srcX, srcW;
            std :: vector<int>    activeTexture;
        };

        auto  runIt = []( bool bSplit )  {
            MockClockFixture   clockFixture;
            MockEngineFixture  fixture;
            Sprite             sprite;
            TextureCanvas      canvasA, canvasB, canvasC;
            Run                run;

            fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
            fixture.engine.nLoadTextureWidth  = 64;
            fixture.engine.nLoadTextureHeight = 16;

            TextureCanvas  *aCanvases[] = { &canvasA, &canvasB, &canvasC };

            for( TextureCanvas *pCanvas : aCanvases )  {
                pCanvas -> Load( "sheet.png" );
                pCanvas -> SetTileSize( 16 );
                pCanvas -> SetAnimationMode( TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
            }

            clockFixture.clock.nNow = 1000;

            for( TextureCanvas *pCanvas : aCanvases )
                sprite.AddTextureSequence( 0, pCanvas, 20 );

            sprite.GetViewport().SetDimension2D( stDimension2D { { 0, 0 }, { 800, 600 } } );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );
            sprite.SetDimension2D( stDimension2D { { 10, 10 }, { 16, 16 } } );

            for( int nFrame = 0; nFrame < 80; nFrame++ )  {
                clockFixture.clock.Advance( 7 );

                int  nBefore = fixture.engine.nDrawTextureTiledCalls;

                if( bSplit )  {
                    sprite.Advance();
                    sprite.Draw();
                }
                else  {
                    sprite.Update();
                }

                run.draws.push_back( fixture.engine.nDrawTextureTiledCalls - nBefore );
                run.srcX.push_back( fixture.engine.lastDrawTextureTiledSource.x );
                run.srcW.push_back( fixture.engine.lastDrawTextureTiledSource.width );
                run.activeTexture.push_back( sprite.GetActiveTexture() == &canvasA ? 0 :
                                             sprite.GetActiveTexture() == &canvasB ? 1 : 2 );
            }

            return run;
        };

        Run  viaUpdate = runIt( false );
        Run  viaSplit  = runIt( true );

        CHECK( viaUpdate.draws == viaSplit.draws );
        CHECK( viaUpdate.srcX == viaSplit.srcX );
        CHECK( viaUpdate.srcW == viaSplit.srcW );
        CHECK( viaUpdate.activeTexture == viaSplit.activeTexture );

        // Non-vacuous: it drew every frame, the active texture really changed, and both a held frame
        // (whole-texture source width 64) and a stepped one (tile width 16) occurred.
        int  nDrawn = 0;
        bool bSawHeld = false, bSawStepped = false;
        std :: vector<int>  seen;

        for( size_t nFrame = 0; nFrame < viaUpdate.draws.size(); nFrame++ )  {
            nDrawn += viaUpdate.draws[nFrame];
            bSawHeld    = bSawHeld    || ( viaUpdate.srcW[nFrame] == 64.0f );
            bSawStepped = bSawStepped || ( viaUpdate.srcW[nFrame] == 16.0f );

            if( std :: find( seen.begin(), seen.end(), viaUpdate.activeTexture[nFrame] ) == seen.end() )
                seen.push_back( viaUpdate.activeTexture[nFrame] );
        }

        CHECK( nDrawn == 80 );
        CHECK( seen.size() == 3 );
        CHECK( bSawHeld );
        CHECK( bSawStepped );
    }

    TEST_CASE( "Destroying a Sprite does not unload its canvases' textures: they are the caller's, and unload themselves when destroyed" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;          // declared first, so it outlives the sprite below

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        REQUIRE( canvas.Load( "sprite.png" ) == true );

        {
            Sprite  sprite;

            sprite.AddTextureSequence( 0, &canvas );
        }

        // The sprite is gone; the canvas' texture is still loaded and is unloaded exactly once, by the canvas.
        CHECK( fixture.engine.nUnloadTextureCalls == 0 );
        CHECK( canvas.Unload() == true );
        CHECK( fixture.engine.nUnloadTextureCalls == 1 );
        CHECK( canvas.Unload() == false );
    }

    TEST_CASE( "The explicit Sprite::Unload() still unloads every canvas it holds" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvasA, canvasB;
        Sprite             sprite;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        REQUIRE( canvasA.Load( "a.png" ) == true );
        REQUIRE( canvasB.Load( "b.png" ) == true );

        sprite.AddTextureSequence( 0, &canvasA );
        sprite.AddTextureSequence( 1, &canvasB );

        sprite.Unload();

        CHECK( fixture.engine.nUnloadTextureCalls == 2 );
        CHECK( canvasA.Unload() == false );             // already unloaded by the explicit call
        CHECK( canvasB.Unload() == false );
    }

    TEST_CASE( "A canvas destroyed BEFORE its sprite is fine (nothing is touched through the dead pointer)" )  {

        // Only an address checker can see the difference (see ASAN in CLAUDE.md): the old
        // destructor read the dead canvas, which "worked" for want of anything to unload.
        MockEngineFixture                 fixture;
        std :: unique_ptr<TextureCanvas>  pCanvas = std :: make_unique<TextureCanvas>();
        Sprite                            sprite;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        REQUIRE( pCanvas -> Load( "sprite.png" ) == true );
        sprite.AddTextureSequence( 0, pCanvas.get() );

        pCanvas.reset();                                 // the canvas unloads itself...
        CHECK( fixture.engine.nUnloadTextureCalls == 1 );
    }                                                    // ...and the sprite's destructor must not touch it again

    TEST_CASE( "GetTextureSequenceSize counts the entries of a sequence: -1 for an unknown one, and a canvas added twice counts twice" )  {

        Sprite         sprite;
        TextureCanvas  first, second;

        CHECK( sprite.GetTextureSequenceSize( 0 ) == -1 );

        sprite.AddTextureSequence( 0, &first );
        CHECK( sprite.GetTextureSequenceSize( 0 ) == 1 );
        sprite.AddTextureSequence( 0, &second );
        CHECK( sprite.GetTextureSequenceSize( 0 ) == 2 );
        sprite.AddTextureSequence( 0, &first );                     // the same canvas again: another entry, as it always was
        CHECK( sprite.GetTextureSequenceSize( 0 ) == 3 );
        CHECK( sprite.GetTextureSequenceSize( 7 ) == -1 );
    }

    TEST_CASE( "SetTextureSequenceDelay changes the pace of a sequence from now on, and refuses an unknown sequence" )  {

        MockClockFixture  clockFixture;
        Sprite            sprite;
        TextureCanvas     first, second;

        clockFixture.clock.nNow = 1000;
        sprite.AddTextureSequence( 0, &first, 200 );                // due at 1200
        sprite.AddTextureSequence( 0, &second, 200 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );

        clockFixture.clock.nNow = 1199;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &first );

        CHECK( sprite.SetTextureSequenceDelay( 0, 40 ) == true );   // the current frame is now due at 1199 + 40 = 1239

        clockFixture.clock.nNow = 1238;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &first );

        clockFixture.clock.nNow = 1239;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &second );

        // The new delay is the pace of the following frames as well.
        clockFixture.clock.nNow = 1278;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &second );
        clockFixture.clock.nNow = 1279;
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &first );

        CHECK( sprite.SetTextureSequenceDelay( 9, 40 ) == false );
    }

    TEST_CASE( "SetTextureSequenceDelay is safe on every reconfigure: the same delay changes nothing, and the shown entry never changes" )  {

        MockClockFixture  clockFixture;
        Sprite            sprite;
        TextureCanvas     first, second;

        clockFixture.clock.nNow = 1000;
        sprite.AddTextureSequence( 0, &first, 100 );                // due at 1100
        sprite.AddTextureSequence( 0, &second, 100 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );

        clockFixture.clock.nNow = 1090;

        for( int nCall = 0; nCall < 5; nCall++ )  {
            CHECK( sprite.SetTextureSequenceDelay( 0, 100 ) == true );
            CHECK( sprite.GetActiveTexture() == &first );          // never moves what is shown
        }

        clockFixture.clock.nNow = 1100;                             // still due when it always was
        sprite.Advance();
        CHECK( sprite.GetActiveTexture() == &second );
    }

    TEST_CASE( "ClearTextureSequence removes a sequence's entries, unparents its canvases, keeps them loaded, and lets them be added again" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvas;
        Sprite             sprite;

        fixture.engine.hLoadTextureResult = ( TextureHandle ) 0xBEEF;
        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        REQUIRE( canvas.Load( "a.png" ) == true );

        sprite.AddTextureSequence( 0, &canvas );
        CHECK( canvas.GetParent() == &sprite );

        CHECK( sprite.ClearTextureSequence( 0 ) == true );
        CHECK( sprite.GetTextureSequenceSize( 0 ) == -1 );
        CHECK( canvas.GetParent() == nullptr );                     // released, like Sprite::Unload does
        CHECK( fixture.engine.nUnloadTextureCalls == 0 );           // ...but NOT unloaded: the canvas is the caller's
        CHECK( canvas.Unload() == true );                           // still loaded
        CHECK( canvas.Load( "a.png" ) == true );

        CHECK( sprite.ClearTextureSequence( 0 ) == false );         // already gone
        CHECK( sprite.ClearTextureSequence( 5 ) == false );

        // Reusable: added again, one entry, parented again.
        sprite.AddTextureSequence( 0, &canvas );
        CHECK( sprite.GetTextureSequenceSize( 0 ) == 1 );
        CHECK( canvas.GetParent() == &sprite );
    }

    TEST_CASE( "Clearing the ACTIVE sequence leaves no active sequence; clearing another one leaves the active one alone" )  {

        MockEngineFixture  fixture;
        TextureCanvas      canvasA, canvasB;
        Sprite             sprite;

        fixture.engine.nLoadTextureWidth  = 32;
        fixture.engine.nLoadTextureHeight = 32;
        REQUIRE( canvasA.Load( "a.png" ) == true );
        REQUIRE( canvasB.Load( "b.png" ) == true );

        sprite.AddTextureSequence( 0, &canvasA );
        sprite.AddTextureSequence( 1, &canvasB );
        sprite.SetVisible( true );
        REQUIRE( sprite.SetActiveTextureSequence( 1 ) == true );

        // Another sequence: the active one is untouched (its map iterator stays valid).
        CHECK( sprite.ClearTextureSequence( 0 ) == true );
        CHECK( sprite.GetActiveTextureSequence() == 1 );
        CHECK( sprite.GetActiveTexture() == &canvasB );

        // The active one: nothing active afterwards, and every frame step is a safe no-op.
        CHECK( sprite.ClearTextureSequence( 1 ) == true );
        CHECK( sprite.GetActiveTextureSequence() == -1 );
        CHECK( sprite.GetActiveTexture() == nullptr );
        CHECK( sprite.IsOnScreen() == false );

        sprite.Advance();
        sprite.Draw();
        sprite.Update();
        CHECK( fixture.engine.nDrawTextureTiledCalls == 0 );

        // A new active sequence can be chosen again.
        sprite.AddTextureSequence( 2, &canvasA );
        CHECK( sprite.SetActiveTextureSequence( 2 ) == true );
        CHECK( sprite.GetActiveTexture() == &canvasA );
    }
}
