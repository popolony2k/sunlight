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
 * TileMapRenderer's window lifecycle - Start()/Run()/Stop() - exercised
 * end to end against MockWindow + MockEngine, with no real display. Only
 * possible since the lifecycle calls (window create/close, should-close,
 * begin/end frame) moved behind IWindow; before that they were raw raylib
 * calls no test double could reach, so this whole area had no coverage.
 */

#include <doctest/doctest.h>
#include <functional>
#include "renderer/tilemaprenderer.h"
#include "tilemap/itilemaplistener.h"
#include "mock_engine.h"
#include "mock_window.h"

using namespace SunLight :: Renderer;
using namespace SunLight :: TileMap;

namespace  {

    /**
     * @brief Listener that counts OnUpdate/OnStop calls and optionally runs
     * a callback from inside OnUpdate (i.e. from inside Run()'s frame).
     */
    class CountingListener : public ITileMapListener  {

        public:

        int                                  nUpdates = 0;
        int                                  nStops   = 0;
        std :: function<void( int )>         onUpdate;

        void OnUpdate( ITileMap& )  {
            nUpdates++;

            if( onUpdate )
                onUpdate( nUpdates );
        }

        void OnStop( void )  {
            nStops++;
        }
    };
}

TEST_SUITE( "renderer/TileMapRenderer lifecycle" )  {

    TEST_CASE( "Start creates the window from the constructor arguments, sets the default exit key and target FPS, and allocates the render target" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "my title", -1, false );

        CHECK( renderer.Start() == true );

        CHECK( windowFixture.window.nCreateCalls == 1 );
        CHECK( windowFixture.window.nLastCreateWidth == 800 );
        CHECK( windowFixture.window.nLastCreateHeight == 600 );
        CHECK( windowFixture.window.strLastCreateTitle == "my title" );
        CHECK( windowFixture.window.bLastCreateResizeable == false );

        CHECK( windowFixture.window.nSetExitKeyCalls == 1 );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_ESCAPE );

        // -1 (the constructor default) resolves to the renderer's own
        // default FPS at Start() and never leaks back out.
        CHECK( windowFixture.window.nSetTargetFPSCalls == 1 );
        CHECK( windowFixture.window.nLastTargetFps == 30 );
        CHECK( renderer.GetTargetFPS() == 30 );

        CHECK( engineFixture.engine.nLoadRenderTargetCalls == 1 );

        renderer.Stop();
    }

    TEST_CASE( "Start passes an explicit target FPS and the pre-Start resizeable choice through to window creation" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", 144, false );

        renderer.SetWindowResizeable( true );

        CHECK( renderer.Start() == true );

        CHECK( windowFixture.window.bLastCreateResizeable == true );
        // Applied at creation, not as a separate live toggle.
        CHECK( windowFixture.window.nSetWindowResizeableCalls == 0 );
        CHECK( windowFixture.window.nLastTargetFps == 144 );

        renderer.Stop();
    }

    TEST_CASE( "Start fails cleanly when window creation fails, and Stop then leaves the window alone" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        windowFixture.window.bCreateResult = false;

        CHECK( renderer.Start() == false );

        CHECK( windowFixture.window.nSetExitKeyCalls == 0 );
        CHECK( windowFixture.window.nSetTargetFPSCalls == 0 );
        CHECK( engineFixture.engine.nLoadRenderTargetCalls == 0 );

        // Never started, so nothing to run and nothing to tear down.
        CHECK( renderer.Run() == false );

        renderer.Stop();

        CHECK( engineFixture.engine.nOnWindowClosingCalls == 0 );
        CHECK( windowFixture.window.nCloseCalls == 0 );
    }

    TEST_CASE( "Start clears an exit request left over from a previous run" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.RequestExit();
        CHECK( renderer.GetExitRequested() == true );

        CHECK( renderer.Start() == true );
        CHECK( renderer.GetExitRequested() == false );

        renderer.Stop();
    }

    TEST_CASE( "SetExitKey forwards to the window, including before Start" )  {

        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        renderer.SetExitKey( SunLight :: Input :: KEY_NULL );

        CHECK( windowFixture.window.nSetExitKeyCalls == 1 );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_NULL );
    }

    TEST_CASE( "Run does nothing and returns false before Start" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        CHECK( renderer.Run() == false );

        CHECK( windowFixture.window.nShouldCloseCalls == 0 );
        CHECK( windowFixture.window.nBeginFrameCalls == 0 );
        CHECK( engineFixture.engine.nBeginRenderTargetCalls == 0 );
    }

    TEST_CASE( "Run draws exactly as many frames as the window allows, bracketing each in BeginFrame/EndFrame" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        windowFixture.window.nFramesUntilShouldClose = 3;

        REQUIRE( renderer.Start() == true );
        CHECK( renderer.Run() == true );

        CHECK( windowFixture.window.nBeginFrameCalls == 3 );
        CHECK( windowFixture.window.nEndFrameCalls == 3 );
        CHECK( engineFixture.engine.nBeginRenderTargetCalls == 3 );
        CHECK( engineFixture.engine.nEndRenderTargetCalls == 3 );
        CHECK( engineFixture.engine.nDrawTextureScaledCalls == 3 );

        // One clear per frame inside the render target (RenderMap) plus
        // one per frame on the window frame itself (the blit), the latter
        // always black.
        CHECK( engineFixture.engine.nClearBackgroundCalls == 6 );
        CHECK( engineFixture.engine.lastClearBackgroundColor.nRed == 0 );
        CHECK( engineFixture.engine.lastClearBackgroundColor.nGreen == 0 );
        CHECK( engineFixture.engine.lastClearBackgroundColor.nBlue == 0 );
        CHECK( engineFixture.engine.lastClearBackgroundColor.nAlpha == 255 );

        // Run() never closes the window itself - that's Stop()'s job.
        CHECK( windowFixture.window.nCloseCalls == 0 );

        renderer.Stop();
    }

    TEST_CASE( "Run fires the listeners' OnUpdate once per frame" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );
        CountingListener   listener;

        renderer.AddTileMapListener( &listener );
        windowFixture.window.nFramesUntilShouldClose = 4;

        REQUIRE( renderer.Start() == true );
        renderer.Run();

        CHECK( listener.nUpdates == 4 );

        renderer.Stop();
    }

    TEST_CASE( "RequestExit from inside a frame lets that frame finish drawing, then ends the loop" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );
        CountingListener   listener;

        // The window never asks to close on its own here - only the
        // programmatic request can end this loop.
        windowFixture.window.nFramesUntilShouldClose = -1;

        listener.onUpdate = [&renderer]( int nUpdate )  {
            if( nUpdate == 2 )
                renderer.RequestExit();
        };

        renderer.AddTileMapListener( &listener );

        REQUIRE( renderer.Start() == true );
        CHECK( renderer.Run() == true );

        // The request was made during frame 2 - that frame still
        // completed (blit + present) before the loop noticed it.
        CHECK( listener.nUpdates == 2 );
        CHECK( windowFixture.window.nBeginFrameCalls == 2 );
        CHECK( windowFixture.window.nEndFrameCalls == 2 );
        CHECK( renderer.GetExitRequested() == true );
        CHECK( windowFixture.window.nCloseCalls == 0 );

        renderer.Stop();
    }

    TEST_CASE( "Run letterboxes the render target into the real window size, preserving aspect ratio" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        windowFixture.window.nScreenWidthResult      = 1600;
        windowFixture.window.nScreenHeightResult     = 900;
        windowFixture.window.nFramesUntilShouldClose = 1;

        REQUIRE( renderer.Start() == true );
        renderer.Run();

        // Uniform scale = min( 1600/800, 900/600 ) = 1.5 -> 1200x900,
        // centered horizontally in the 1600-wide window.
        const SunLight :: Base :: stRectangle  &source = engineFixture.engine.lastDrawTextureScaledSource;
        const SunLight :: Base :: stRectangle  &dest   = engineFixture.engine.lastDrawTextureScaledDest;

        CHECK( source.width  == doctest :: Approx( 800.0f ) );
        CHECK( source.height == doctest :: Approx( -600.0f ) );  // negative: render targets are stored bottom-up
        CHECK( dest.x        == doctest :: Approx( 200.0f ) );
        CHECK( dest.y        == doctest :: Approx( 0.0f ) );
        CHECK( dest.width    == doctest :: Approx( 1200.0f ) );
        CHECK( dest.height   == doctest :: Approx( 900.0f ) );

        renderer.Stop();
    }

    TEST_CASE( "Run stretches the render target to the whole window when stretch-to-fill is on" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        windowFixture.window.nScreenWidthResult      = 1600;
        windowFixture.window.nScreenHeightResult     = 900;
        windowFixture.window.nFramesUntilShouldClose = 1;
        renderer.SetStretchToFill( true );

        REQUIRE( renderer.Start() == true );
        renderer.Run();

        const SunLight :: Base :: stRectangle  &dest = engineFixture.engine.lastDrawTextureScaledDest;

        CHECK( dest.x      == doctest :: Approx( 0.0f ) );
        CHECK( dest.y      == doctest :: Approx( 0.0f ) );
        CHECK( dest.width  == doctest :: Approx( 1600.0f ) );
        CHECK( dest.height == doctest :: Approx( 900.0f ) );

        renderer.Stop();
    }

    TEST_CASE( "Run draws the FPS counter every frame only when it's enabled" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        windowFixture.window.nFramesUntilShouldClose = 2;

        REQUIRE( renderer.Start() == true );
        renderer.Run();

        CHECK( engineFixture.engine.nDrawFPSCalls == 0 );

        // Second run on the same started renderer: the mock's frame
        // counter is cumulative, so allow two more frames.
        renderer.SetDrawFPS( true );
        windowFixture.window.nFramesUntilShouldClose = 4;
        renderer.Run();

        CHECK( engineFixture.engine.nDrawFPSCalls == 2 );

        renderer.Stop();
    }

    TEST_CASE( "Run draws the screen-fade overlay over the whole render area, only while fading" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        windowFixture.window.nFramesUntilShouldClose = 2;

        REQUIRE( renderer.Start() == true );
        renderer.Run();

        CHECK( engineFixture.engine.nDrawFilledRectangleCalls == 0 );

        renderer.SetScreenFade( 0.5f );
        windowFixture.window.nFramesUntilShouldClose = 4;
        renderer.Run();

        CHECK( engineFixture.engine.nDrawFilledRectangleCalls == 2 );
        CHECK( engineFixture.engine.nLastFilledRectangleX == 0 );
        CHECK( engineFixture.engine.nLastFilledRectangleY == 0 );
        CHECK( engineFixture.engine.nLastFilledRectangleWidth == 800 );
        CHECK( engineFixture.engine.nLastFilledRectangleHeight == 600 );
        CHECK( engineFixture.engine.lastFilledRectangleColor.nAlpha == 127 );

        renderer.Stop();
    }

    TEST_CASE( "Stop notifies listeners, releases the render target, lets the engine clean up, then closes the window - in that order" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );
        CountingListener   listener;

        renderer.AddTileMapListener( &listener );
        windowFixture.window.pEngineOnWindowClosingCalls = &engineFixture.engine.nOnWindowClosingCalls;

        REQUIRE( renderer.Start() == true );
        renderer.Stop();

        CHECK( listener.nStops == 1 );
        CHECK( engineFixture.engine.nUnloadRenderTargetCalls == 1 );
        CHECK( engineFixture.engine.nOnWindowClosingCalls == 1 );
        CHECK( windowFixture.window.nCloseCalls == 1 );

        // The engine's own pre-close hook had already run by the time the
        // window actually closed (see IEngine::OnWindowClosing).
        CHECK( windowFixture.window.nEngineOnWindowClosingSeenAtClose == 1 );
    }

    TEST_CASE( "Stop is idempotent for the window - a second Stop never closes it twice" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        REQUIRE( renderer.Start() == true );
        renderer.Stop();
        renderer.Stop();

        CHECK( windowFixture.window.nCloseCalls == 1 );
        CHECK( engineFixture.engine.nOnWindowClosingCalls == 1 );
    }

    TEST_CASE( "A stopped renderer can be started again, creating a fresh window and render target" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        REQUIRE( renderer.Start() == true );
        renderer.Stop();
        REQUIRE( renderer.Start() == true );

        CHECK( windowFixture.window.nCreateCalls == 2 );
        CHECK( engineFixture.engine.nLoadRenderTargetCalls == 2 );

        renderer.Stop();
    }
}
