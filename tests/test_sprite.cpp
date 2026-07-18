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
}
