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
 * RendererConfig + the config-based TileMapRenderer constructor: none of
 * this needs a window. The Start()-time behavior (exit key applied,
 * title/size/resizeable reaching window creation) goes through MockWindow.
 */

#include <doctest/doctest.h>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"

using namespace SunLight :: Renderer;

namespace  {

    SunLight :: TileMap :: stDimension2D  MakeViewport( int nX, int nY, int nW, int nH )  {

        SunLight :: TileMap :: stDimension2D  dim {};

        dim.pos.x       = nX;
        dim.pos.y       = nY;
        dim.size.nWidth = nW;
        dim.size.nHeight = nH;

        return dim;
    }
}

TEST_SUITE( "renderer/RendererConfig" )  {

    TEST_CASE( "The default config validates" )  {

        RendererConfig  config;
        std :: string   strError = "untouched";

        CHECK( config.Validate( &strError ) == true );
        CHECK( strError.empty() );
    }

    TEST_CASE( "DEFAULT and RAYLIB backends are available in this build; NULL and the sentinel are not" )  {

        CHECK( RendererConfig :: IsBackendAvailable( RENDERER_BACKEND_DEFAULT ) == true );
        CHECK( RendererConfig :: IsBackendAvailable( RENDERER_BACKEND_RAYLIB ) == true );

        // NULL is part of the enum contract but has no implementation yet.
        CHECK( RendererConfig :: IsBackendAvailable( RENDERER_BACKEND_NULL ) == false );

        CHECK( RendererConfig :: IsBackendAvailable( RENDERER_BACKEND_LAST ) == false );
        CHECK( RendererConfig :: IsBackendAvailable( ( RendererBackend ) 99 ) == false );
        CHECK( RendererConfig :: IsBackendAvailable( ( RendererBackend ) -1 ) == false );
    }

    TEST_CASE( "Validate rejects an unavailable backend, an unknown value and a non-positive size, with a message" )  {

        RendererConfig  config;
        std :: string   strError;

        config.backend = RENDERER_BACKEND_NULL;
        CHECK( config.Validate( &strError ) == false );
        CHECK( strError == "renderer backend 'null' is not available in this build" );

        config.backend = RENDERER_BACKEND_LAST;
        CHECK( config.Validate( &strError ) == false );
        CHECK( strError.find( "unknown renderer backend" ) != std :: string :: npos );

        config.backend = ( RendererBackend ) 99;
        CHECK( config.Validate( &strError ) == false );
        CHECK( strError.find( "99" ) != std :: string :: npos );

        config.backend = RENDERER_BACKEND_DEFAULT;
        config.fWidth  = 0.0f;
        CHECK( config.Validate( &strError ) == false );
        CHECK( strError.find( "width and height" ) != std :: string :: npos );

        config.fWidth  = 800.0f;
        config.fHeight = -5.0f;
        CHECK( config.Validate( &strError ) == false );

        // A null error pointer is fine.
        CHECK( config.Validate() == false );
    }

    TEST_CASE( "Create returns nullptr with the reason for an invalid config, and a renderer for a valid one" )  {

        RendererConfig  bad;
        std :: string   strError;

        bad.backend = RENDERER_BACKEND_NULL;

        CHECK( TileMapRenderer :: Create( bad, &strError ) == nullptr );
        CHECK( strError.empty() == false );

        RendererConfig                       good;
        std :: unique_ptr<TileMapRenderer>   pRenderer = TileMapRenderer :: Create( good, &strError );

        REQUIRE( pRenderer != nullptr );
        CHECK( strError.empty() );
        CHECK( pRenderer -> GetWindowWidth() == 800 );
    }

    TEST_CASE( "BackendName names every enum value" )  {

        CHECK( std :: string( RendererConfig :: BackendName( RENDERER_BACKEND_DEFAULT ) ) == "default" );
        CHECK( std :: string( RendererConfig :: BackendName( RENDERER_BACKEND_RAYLIB ) ) == "raylib" );
        CHECK( std :: string( RendererConfig :: BackendName( RENDERER_BACKEND_NULL ) ) == "null" );
        CHECK( std :: string( RendererConfig :: BackendName( RENDERER_BACKEND_LAST ) ) == "unknown" );
    }
}

TEST_SUITE( "renderer/TileMapRenderer(RendererConfig)" )  {

    TEST_CASE( "A default config builds the same renderer the classic constructor does" )  {

        TileMapRenderer  classic( 800, 600, "SunLight", -1, true );
        TileMapRenderer  fromConfig( ( RendererConfig() ) );

        CHECK( fromConfig.GetWindowWidth()        == classic.GetWindowWidth() );
        CHECK( fromConfig.GetWindowHeight()       == classic.GetWindowHeight() );
        CHECK( fromConfig.GetTargetFPS()          == classic.GetTargetFPS() );
        CHECK( fromConfig.GetWindowResizeable()   == classic.GetWindowResizeable() );
        CHECK( fromConfig.GetDrawFPS()            == classic.GetDrawFPS() );
        CHECK( fromConfig.GetStretchToFill()      == classic.GetStretchToFill() );
        CHECK( fromConfig.GetExitKey()            == classic.GetExitKey() );
        CHECK( fromConfig.GetExitRequested()      == classic.GetExitRequested() );
    }

    TEST_CASE( "The classic constructor arguments land in the same places as the equivalent config" )  {

        MockWindowFixture  windowFixture;
        MockEngineFixture  engineFixture;
        TileMapRenderer    classic( 320, 240, "classic", 45, false );

        CHECK( classic.GetWindowWidth() == 320 );
        CHECK( classic.GetWindowHeight() == 240 );
        CHECK( classic.GetTargetFPS() == 45 );

        REQUIRE( classic.Start() == true );

        CHECK( windowFixture.window.nLastCreateWidth == 320 );
        CHECK( windowFixture.window.nLastCreateHeight == 240 );
        CHECK( windowFixture.window.strLastCreateTitle == "classic" );
        CHECK( windowFixture.window.nLastTargetFps == 45 );

        classic.Stop();
    }

    TEST_CASE( "Every scalar field of the config is applied to the renderer" )  {

        RendererConfig  config;

        config.fWidth         = 1260.0f;
        config.fHeight        = 920.0f;
        config.strTitle       = "configured";
        config.nTargetFps     = 60;
        config.bResizeable    = true;
        config.bDrawFPS       = true;
        config.bStretchToFill = true;
        config.exitKey        = SunLight :: Input :: KEY_NULL;

        TileMapRenderer  renderer( config );

        CHECK( renderer.GetWindowWidth() == 1260 );
        CHECK( renderer.GetWindowHeight() == 920 );
        CHECK( renderer.GetTargetFPS() == 60 );
        CHECK( renderer.GetWindowResizeable() == true );
        CHECK( renderer.GetDrawFPS() == true );
        CHECK( renderer.GetStretchToFill() == true );
        CHECK( renderer.GetExitKey() == SunLight :: Input :: KEY_NULL );
    }

    TEST_CASE( "Title, size and the resizeable choice reach window creation at Start" )  {

        MockWindowFixture  windowFixture;
        MockEngineFixture  engineFixture;
        RendererConfig     config;

        config.fWidth      = 1260.0f;
        config.fHeight     = 920.0f;
        config.strTitle    = "configured";
        config.bResizeable = true;

        TileMapRenderer  renderer( config );

        REQUIRE( renderer.Start() == true );

        CHECK( windowFixture.window.nLastCreateWidth == 1260 );
        CHECK( windowFixture.window.nLastCreateHeight == 920 );
        CHECK( windowFixture.window.strLastCreateTitle == "configured" );
        CHECK( windowFixture.window.bLastCreateResizeable == true );

        renderer.Stop();
    }

    TEST_CASE( "The configured exit key is applied at Start, and a key chosen before Start sticks" )  {

        MockWindowFixture  windowFixture;
        MockEngineFixture  engineFixture;

        // Via the config.
        RendererConfig  config;

        config.exitKey = SunLight :: Input :: KEY_NULL;

        TileMapRenderer  fromConfig( config );

        REQUIRE( fromConfig.Start() == true );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_NULL );
        CHECK( fromConfig.GetExitKey() == SunLight :: Input :: KEY_NULL );
        fromConfig.Stop();

        // Via SetExitKey before Start (used to be overwritten with ESC).
        TileMapRenderer  viaSetter( 800, 600, "test", -1, false );

        viaSetter.SetExitKey( SunLight :: Input :: KEY_Q );

        REQUIRE( viaSetter.Start() == true );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_Q );
        CHECK( viaSetter.GetExitKey() == SunLight :: Input :: KEY_Q );

        // ...and across a Stop()/Start() cycle.
        viaSetter.Stop();
        REQUIRE( viaSetter.Start() == true );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_Q );

        viaSetter.Stop();
    }

    TEST_CASE( "A key set after Start still wins, and is what the next Start applies" )  {

        MockWindowFixture  windowFixture;
        MockEngineFixture  engineFixture;
        TileMapRenderer    renderer( 800, 600, "test", -1, false );

        REQUIRE( renderer.Start() == true );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_ESCAPE );

        renderer.SetExitKey( SunLight :: Input :: KEY_NULL );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_NULL );

        renderer.Stop();
        REQUIRE( renderer.Start() == true );
        CHECK( windowFixture.window.lastExitKey == SunLight :: Input :: KEY_NULL );

        renderer.Stop();
    }

    TEST_CASE( "Viewport and zoom from the config match setting them by hand, in the classic order" )  {

        RendererConfig  config;

        config.viewport = MakeViewport( 10, 10, 1250, 910 );
        config.nZoomPos = 60u;

        TileMapRenderer  fromConfig( config );
        TileMapRenderer  byHand( 800, 600, "SunLight", -1, true );

        byHand.GetViewport().SetZoom( 60 );
        byHand.GetViewport().SetDimension2D( MakeViewport( 10, 10, 1250, 910 ) );

        SunLight :: TileMap :: stDimension2D  &configVp = fromConfig.GetViewport().GetDimension2D();
        SunLight :: TileMap :: stDimension2D  &handVp   = byHand.GetViewport().GetDimension2D();

        CHECK( configVp.pos.x == 10 );
        CHECK( configVp.pos.y == 10 );
        CHECK( configVp.size.nWidth == 1250 );
        CHECK( configVp.size.nHeight == 910 );
        CHECK( configVp.pos.x == handVp.pos.x );
        CHECK( configVp.size.nWidth == handVp.size.nWidth );
        CHECK( fromConfig.GetViewport().GetZoomProperties().nCurrentZoomPos ==
               byHand.GetViewport().GetZoomProperties().nCurrentZoomPos );
        CHECK( fromConfig.GetViewport().GetZoomProperties().fZoomFactor ==
               byHand.GetViewport().GetZoomProperties().fZoomFactor );
    }

    TEST_CASE( "Left unset, viewport and zoom keep the renderer's own defaults" )  {

        TileMapRenderer  fromConfig( ( RendererConfig() ) );
        TileMapRenderer  classic( 800, 600, "SunLight", -1, true );

        CHECK( fromConfig.GetViewport().GetZoomProperties().nCurrentZoomPos ==
               classic.GetViewport().GetZoomProperties().nCurrentZoomPos );
        CHECK( fromConfig.GetViewport().GetDimension2D().size.nWidth ==
               classic.GetViewport().GetDimension2D().size.nWidth );
    }
}
