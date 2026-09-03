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
 * TileMapRenderer's window lifecycle (Start()/Run()/Stop()) calls raylib's
 * InitWindow()/WindowShouldClose()/BeginDrawing()/EndDrawing() directly, so
 * it needs a real display and isn't unit-testable here (see
 * doc/MISSING_FEATURES.md). What IS safely testable without ever calling
 * Start(): every entry point that gates on m_bIsStarted or a loaded
 * m_pTmxMap (LoadMap, AddSprite, RemoveSprite, GetMapInfo, GetLayer,
 * SetLayer, TileMapToTileMatrix) is documented to fail gracefully rather
 * than crash when called too early - this file locks that contract in,
 * plus the handful of accessors (GetInputHandler, GetCollisionManager)
 * that work standalone.
 */

#include <doctest/doctest.h>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "mock_engine.h"

using namespace SunLight :: Renderer;
using namespace SunLight :: TileMap;

TEST_SUITE( "renderer/TileMapRenderer" )  {

    TEST_CASE( "Construction does not crash and does not start the renderer" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, true );
        stMapInfo        mapInfo;

        CHECK( renderer.GetMapInfo( mapInfo ) == false );
    }

    TEST_CASE( "GetInputHandler returns the same handler instance on every call" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( &renderer.GetInputHandler() == &renderer.GetInputHandler() );
    }

    TEST_CASE( "GetCollisionManager returns a stable, usable reference before any map is loaded" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( &renderer.GetCollisionManager() == &renderer.GetCollisionManager() );

        // AddColliderToColliderRule only touches the collider-layer lists,
        // not the parent tile map, so it works even with nothing loaded.
        CHECK( renderer.GetCollisionManager().AddColliderToColliderRule( 0, 1 ) == true );
    }

    TEST_CASE( "LoadMap fails without crashing when the renderer hasn't been started" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( renderer.LoadMap( "does-not-matter.tmx" ) == false );
    }

    TEST_CASE( "UnloadMap/GetMapInfo report failure when no map has been loaded" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );
        stMapInfo        mapInfo;

        CHECK( renderer.UnloadMap() == false );
        CHECK( renderer.GetMapInfo( mapInfo ) == false );
    }

    TEST_CASE( "GetLayer/SetLayer (by id and by name) report failure when no map has been loaded" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );
        stLayer          layer {};

        CHECK( renderer.GetLayer( 1, layer ) == false );
        CHECK( renderer.GetLayer( "layer1", layer ) == false );
        CHECK( renderer.SetLayer( 1, layer ) == false );
        CHECK( renderer.SetLayer( "layer1", layer ) == false );
    }

    TEST_CASE( "TileMapToTileMatrix reports failure when no map has been loaded" )  {

        TileMapRenderer   renderer( 800, 600, "test", -1, false );
        stCoordinate2D    coord { 0, 0 };
        stMatrixPosition  pos   { 0, 0 };

        CHECK( renderer.TileMapToTileMatrix( coord, pos ) == false );
    }

    TEST_CASE( "AddSprite/RemoveSprite report failure when the renderer hasn't been started" )  {

        TileMapRenderer                    renderer( 800, 600, "test", -1, false );
        SunLight :: Sprite :: Sprite       sprite;

        CHECK( renderer.AddSprite( 1, sprite ) == false );
        CHECK( renderer.RemoveSprite( 1, sprite ) == false );
    }

    TEST_CASE( "GetDrawFPS/GetWindowResizeable/GetStretchToFill/GetTargetFPS/GetExitRequested round-trip what their setters last set" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( renderer.GetExitRequested() == false );

        renderer.RequestExit();
        CHECK( renderer.GetExitRequested() == true );

        renderer.SetDrawFPS( true );
        CHECK( renderer.GetDrawFPS() == true );

        renderer.SetDrawFPS( false );
        CHECK( renderer.GetDrawFPS() == false );

        renderer.SetWindowResizeable( true );
        CHECK( renderer.GetWindowResizeable() == true );

        renderer.SetWindowResizeable( false );
        CHECK( renderer.GetWindowResizeable() == false );

        renderer.SetStretchToFill( true );
        CHECK( renderer.GetStretchToFill() == true );

        renderer.SetStretchToFill( false );
        CHECK( renderer.GetStretchToFill() == false );

        renderer.SetTargetFPS( 144 );
        CHECK( renderer.GetTargetFPS() == 144 );

        renderer.SetTargetFPS( 30 );
        CHECK( renderer.GetTargetFPS() == 30 );
    }

    TEST_CASE( "SetWindowResizeable before Start() only updates local state, never touches IEngine" )  {

        // IEngine::SetWindowResizeable is only meaningful once the window
        // exists (see its own doc comment) - this locks in the m_bIsStarted
        // gate that keeps TileMapRenderer from calling it too early, using
        // MockEngineFixture rather than a real window/display.
        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.SetWindowResizeable( true );

        CHECK( renderer.GetWindowResizeable() == true );
        CHECK( fixture.engine.nSetWindowResizeableCalls == 0 );
    }

    TEST_CASE( "SetTargetFPS before Start() only updates local state, never touches IEngine" )  {

        // Same gate as SetWindowResizeable above - IEngine::SetTargetFPS
        // is only meaningful once the window exists (raylib's own
        // ::SetTargetFPS acts on the live game loop's own frame pacing).
        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.SetTargetFPS( 144 );

        CHECK( renderer.GetTargetFPS() == 144 );
        CHECK( fixture.engine.nSetTargetFPSCalls == 0 );
    }

    TEST_CASE( "SetWindowTitle before Start() only updates local state, never touches IEngine" )  {

        // Same gate as SetWindowResizeable above - IEngine::SetWindowTitle
        // is only meaningful once the window exists (raylib's own
        // SetWindowTitle acts on the live window handle).
        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.SetWindowTitle( "new title" );

        CHECK( fixture.engine.nSetWindowTitleCalls == 0 );
    }

    TEST_CASE( "DrawFilledRectangle forwards straight through to IEngine" )  {

        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.DrawFilledRectangle( 10, 20, 30, 40, 255, 0, 0, 128 );

        CHECK( fixture.engine.nDrawFilledRectangleCalls  == 1 );
        CHECK( fixture.engine.nLastFilledRectangleX      == 10 );
        CHECK( fixture.engine.nLastFilledRectangleY      == 20 );
        CHECK( fixture.engine.nLastFilledRectangleWidth  == 30 );
        CHECK( fixture.engine.nLastFilledRectangleHeight == 40 );
        CHECK( fixture.engine.lastFilledRectangleColor.nRed   == 255 );
        CHECK( fixture.engine.lastFilledRectangleColor.nGreen == 0 );
        CHECK( fixture.engine.lastFilledRectangleColor.nBlue  == 0 );
        CHECK( fixture.engine.lastFilledRectangleColor.nAlpha == 128 );
    }

    TEST_CASE( "SetFullscreen forwards straight through to IEngine, defaulting to the real strategy" )  {

        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.SetFullscreen( true );

        CHECK( fixture.engine.nSetFullscreenCalls == 1 );
        CHECK( fixture.engine.bFullscreen == true );
        CHECK( fixture.engine.lastFullscreenStrategy == SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL );

        renderer.SetFullscreen( false, SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );

        CHECK( fixture.engine.nSetFullscreenCalls == 2 );
        CHECK( fixture.engine.bFullscreen == false );
        CHECK( fixture.engine.lastFullscreenStrategy == SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );
    }

    TEST_CASE( "DrawText forwards straight through to IEngine" )  {

        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.DrawText( "score: 0", 5, 5, 16, 255, 255, 255, 255 );

        CHECK( fixture.engine.nDrawTextCalls == 1 );
        CHECK( fixture.engine.strLastDrawnText == "score: 0" );
    }

    TEST_CASE( "MeasureText forwards straight through to IEngine and returns its result" )  {

        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        fixture.engine.nMeasureTextResult = 42;

        CHECK( renderer.MeasureText( "score: 0", 16 ) == 42 );
        CHECK( fixture.engine.nMeasureTextCalls == 1 );
    }

    TEST_CASE( "SetFont forwards straight through to IEngine and returns its result" )  {

        MockEngineFixture  fixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        fixture.engine.bSetFontResult = false;

        CHECK( renderer.SetFont( "does-not-matter.ttf" ) == false );
        CHECK( fixture.engine.nSetFontCalls == 1 );
        CHECK( fixture.engine.strLastSetFontPath == "does-not-matter.ttf" );
    }
}
