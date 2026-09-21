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
 * Fullscreen at window creation: RendererConfig::bFullscreen / fullscreenStrategy are handed to IWindow::Create
 * (so a backend can bring the window up already fullscreen, with no windowed phase), the null backend accepts
 * and ignores them, and the strategy in effect can be read back. The raylib side (hide the window, apply the
 * same call SetFullscreen makes, show it) needs a real display and is verified by hand.
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "renderer/rendererconfig.h"
#include "mock_engine.h"
#include "mock_window.h"

using namespace SunLight :: Renderer;
using namespace SunLight :: Window;

TEST_SUITE( "renderer/fullscreen at creation" )  {

    TEST_CASE( "RendererConfig defaults to windowed with the REAL strategy" )  {

        RendererConfig  config;

        CHECK( config.bFullscreen == false );
        CHECK( config.fullscreenStrategy == FULLSCREEN_STRATEGY_REAL );

        std :: string  strError;

        CHECK( config.Validate( &strError ) == true );
    }

    TEST_CASE( "Validate accepts both strategies and rejects a value outside the enum with a clear message" )  {

        RendererConfig  config;
        std :: string   strError;

        config.bFullscreen = true;

        config.fullscreenStrategy = FULLSCREEN_STRATEGY_REAL;
        CHECK( config.Validate( &strError ) == true );

        config.fullscreenStrategy = FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED;
        CHECK( config.Validate( &strError ) == true );

        config.fullscreenStrategy = ( FullscreenStrategy ) 7;
        CHECK( config.Validate( &strError ) == false );
        CHECK( strError.find( "fullscreen strategy" ) != std :: string :: npos );
        CHECK( strError.find( "7" ) != std :: string :: npos );

        config.fullscreenStrategy = ( FullscreenStrategy ) -1;
        CHECK( config.Validate( &strError ) == false );

        // Even when fullscreen is off: a bad value is a bad config, not something to be silently ignored.
        config.bFullscreen = false;
        CHECK( config.Validate( &strError ) == false );

        // And Create refuses to build a renderer from it, saying why.
        std :: string  strCreateError;

        CHECK( TileMapRenderer :: Create( config, &strCreateError ) == nullptr );
        CHECK( strCreateError.find( "fullscreen strategy" ) != std :: string :: npos );
    }

    TEST_CASE( "Start() hands the fullscreen request to the window at creation, and the window reports the resulting state" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;

        RendererConfig  config;

        config.bFullscreen        = true;
        config.fullscreenStrategy = FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        CHECK( windowFixture.window.nCreateCalls == 1 );
        CHECK( windowFixture.window.bLastCreateFullscreen == true );
        CHECK( windowFixture.window.lastCreateStrategy == FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );

        // Born fullscreen: no SetFullscreen call was needed to get there.
        CHECK( windowFixture.window.nSetFullscreenCalls == 0 );
        CHECK( pRenderer -> GetFullscreen() == true );
        CHECK( pRenderer -> GetFullscreenStrategy() == FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );

        pRenderer -> Stop();
    }

    TEST_CASE( "The REAL strategy is handed over too, and a default config asks for a plain window" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;

        {
            RendererConfig  config;

            config.bFullscreen = true;                              // strategy left at its default

            std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            CHECK( windowFixture.window.bLastCreateFullscreen == true );
            CHECK( windowFixture.window.lastCreateStrategy == FULLSCREEN_STRATEGY_REAL );
            CHECK( pRenderer -> GetFullscreenStrategy() == FULLSCREEN_STRATEGY_REAL );
            pRenderer -> Stop();
        }

        windowFixture.window.bFullscreen = false;

        {
            RendererConfig  config;                                 // defaults

            std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            CHECK( windowFixture.window.bLastCreateFullscreen == false );
            CHECK( pRenderer -> GetFullscreen() == false );
            pRenderer -> Stop();
        }
    }

    TEST_CASE( "The request applies at EVERY Start(), like the exit key, and later SetFullscreen calls still work" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;

        RendererConfig  config;

        config.bFullscreen = true;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );
        pRenderer -> SetFullscreen( false );                        // the player leaves fullscreen
        CHECK( pRenderer -> GetFullscreen() == false );
        pRenderer -> Stop();

        windowFixture.window.bFullscreen = false;
        windowFixture.window.bLastCreateFullscreen = false;

        REQUIRE( pRenderer -> Start() == true );                    // a restart is born fullscreen again
        CHECK( windowFixture.window.bLastCreateFullscreen == true );
        CHECK( windowFixture.window.nCreateCalls == 2 );
        pRenderer -> Stop();
    }

    TEST_CASE( "A window that fails to create is a failed Start(), fullscreen or not" )  {

        MockEngineFixture  engineFixture;
        MockWindowFixture  windowFixture;

        RendererConfig  config;

        config.bFullscreen = true;
        windowFixture.window.bCreateResult = false;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        CHECK( pRenderer -> Start() == false );
        CHECK( pRenderer -> GetFullscreen() == false );
    }

    TEST_CASE( "The null backend accepts and ignores the request: no error, GetFullscreen stays false, the strategy reads REAL" )  {

        RendererConfig  config;

        config.backend            = RENDERER_BACKEND_NULL;
        config.framePacing        = FRAME_PACING_UNLIMITED;
        config.nMaxFrames         = 2;
        config.bFullscreen        = true;
        config.fullscreenStrategy = FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED;

        std :: string                       strError;
        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, &strError );

        REQUIRE( pRenderer != nullptr );
        CHECK( strError.empty() );
        REQUIRE( pRenderer -> Start() == true );
        CHECK( pRenderer -> Run() == true );

        CHECK( pRenderer -> GetFullscreen() == false );
        CHECK( pRenderer -> GetFullscreenStrategy() == FULLSCREEN_STRATEGY_REAL );

        // An explicit SetFullscreen still works on it, as it always did, and reads back its strategy.
        pRenderer -> SetFullscreen( true, FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );
        CHECK( pRenderer -> GetFullscreen() == true );
        CHECK( pRenderer -> GetFullscreenStrategy() == FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );

        pRenderer -> SetFullscreen( false );
        CHECK( pRenderer -> GetFullscreenStrategy() == FULLSCREEN_STRATEGY_REAL );      // windowed: the default

        pRenderer -> Stop();
    }
}
