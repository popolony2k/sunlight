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
