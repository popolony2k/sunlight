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
 * The null backend: input handler, engine, window, virtual clock, and - the
 * point of it all - a whole TileMapRenderer running on it with no display,
 * no GPU and no real time. Assets are served by a small in-memory
 * IFileSystem installed through FileSystemFactory::SetFileSystem.
 */

#include <doctest/doctest.h>
#include <map>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "tilemap/itilemaplistener.h"
#include "backends/null/nullengine.h"
#include "backends/null/nullwindow.h"
#include "backends/null/nullinputhandler.h"
#include "backends/null/nullbackend.h"
#include "filesystem/filesystemfactory.h"
#include "engines/enginefactory.h"
#include "window/windowfactory.h"
#include "input/inputhandlerfactory.h"
#include "general/virtualclock.h"
#include "scripting/scriptprocessor.h"

using namespace SunLight :: Renderer;

namespace  {

    typedef std :: vector<unsigned char>  Bytes;

    /**
     * @brief In-memory IFileSystem: a map of virtual path -> bytes.
     */
    class MemoryFileSystem : public SunLight :: FileSystem :: IFileSystem  {

        public:

        std :: map<std :: string, Bytes>  files;

        void SetReadFilter( ReadFilterCallback ) override {}
        bool Init( const char * ) override { return true; }
        void Shutdown( void ) override {}
        bool Mount( const std :: string &, const std :: string &, bool ) override { return true; }
        bool Exists( const std :: string &strPath ) override { return files.count( strPath ) > 0; }

        bool ReadFile( const std :: string &strPath, Bytes &out ) override {
            auto  it = files.find( strPath );

            if( it == files.end() )
                return false;

            out = it -> second;
            return true;
        }
    };

    /**
     * @brief Installs a MemoryFileSystem as the process filesystem for its lifetime.
     */
    class MemoryFileSystemFixture  {

        public:

        MemoryFileSystem  fs;

        MemoryFileSystemFixture( void )  { SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( &fs ); }
        ~MemoryFileSystemFixture( void ) { SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( nullptr ); }
    };

    Bytes MakePng( unsigned nWidth, unsigned nHeight )  {

        Bytes  b = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 'I', 'H', 'D', 'R' };

        for( unsigned n : { nWidth, nHeight } )  {
            b.push_back( ( n >> 24 ) & 0xFF ); b.push_back( ( n >> 16 ) & 0xFF );
            b.push_back( ( n >> 8 ) & 0xFF );  b.push_back( n & 0xFF );
        }
        for( int n = 0; n < 5; n++ )
            b.push_back( 0 );

        return b;
    }

    Bytes MakeJpeg( unsigned nWidth, unsigned nHeight )  {

        Bytes  b = { 0xFF, 0xD8, 0xFF, 0xC0, 0, 17, 8,
                     ( unsigned char ) ( nHeight >> 8 ), ( unsigned char ) nHeight,
                     ( unsigned char ) ( nWidth >> 8 ),  ( unsigned char ) nWidth };

        for( int n = 0; n < 9; n++ )
            b.push_back( 0 );

        return b;
    }

    class UpdateCounter : public SunLight :: TileMap :: ITileMapListener  {

        public:

        int                          nUpdates = 0;
        std :: function<void( int )> onUpdate;

        void OnUpdate( SunLight :: TileMap :: ITileMap & )  {
            nUpdates++;

            if( onUpdate )
                onUpdate( nUpdates );
        }

        void OnStop( void )  {}
    };

    class WaitRecorder : public SunLight :: Scripting :: IScriptListener  {

        public:

        int  nCommands = 0;

        void OnCommand( SunLight :: Scripting :: Commands, uint16_t )  { nCommands++; }
        void OnError( std :: string )  {}
    };

    RendererConfig NullConfig( unsigned nMaxFrames = 0 )  {

        RendererConfig  config;

        config.backend      = RENDERER_BACKEND_NULL;
        config.fWidth       = 320.0f;
        config.fHeight      = 240.0f;
        config.nTargetFps   = 60;
        config.framePacing  = FRAME_PACING_UNLIMITED;   // tests never wait on the wall clock
        config.nMaxFrames   = nMaxFrames;

        return config;
    }
}

TEST_SUITE( "backends/null/NullInputHandler" )  {

    TEST_CASE( "Every query answers the neutral value and never fails" )  {

        SunLight :: Input :: Null :: NullInputHandler  input;

        CHECK( input.GetKeyPressed() == SunLight :: Input :: KEY_NULL );
        CHECK( input.IsKeyDown( SunLight :: Input :: KEY_ESCAPE ) == false );
        CHECK( input.IsKeyDown( SunLight :: Input :: KEY_A ) == false );
        CHECK( input.IsKeyReleased( SunLight :: Input :: KEY_A ) == false );

        CHECK( input.IsGamepadAvailable( 0 ) == false );
        CHECK( input.IsGamepadButtonDown( 0, SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == false );
        CHECK( input.IsGamepadButtonUp( 0, SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == false );
        CHECK( input.GetGamepadButtonPressed() == SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN );
        CHECK( input.GetGamepadAxisCount( 0 ) == 0 );

        REQUIRE( input.GetGamepadName( 0 ) != nullptr );
        CHECK( std :: string( input.GetGamepadName( 0 ) ) == "" );
    }

    TEST_CASE( "Every gamepad axis reads exactly 0.0, triggers included" )  {

        SunLight :: Input :: Null :: NullInputHandler  input;

        for( int nAxis = SunLight :: Input :: GAMEPAD_AXIS_LEFT_X; nAxis <= SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER; nAxis++ )
            CHECK( input.GetGamepadAxisMovement( 0, ( SunLight :: Input :: GamepadAxis ) nAxis ) == 0.0f );
    }

    TEST_CASE( "Valid keys are 'up' (nothing is ever down); KEY_NULL and junk codes are not keys" )  {

        SunLight :: Input :: Null :: NullInputHandler  input;

        CHECK( input.IsKeyUp( SunLight :: Input :: KEY_SPACE ) == true );
        CHECK( input.IsKeyUp( SunLight :: Input :: KEY_ESCAPE ) == true );
        CHECK( input.IsKeyUp( SunLight :: Input :: KEY_NULL ) == false );
        CHECK( input.IsKeyUp( ( SunLight :: Input :: KeyboardKey ) 100000 ) == false );
        CHECK( input.IsKeyUp( ( SunLight :: Input :: KeyboardKey ) -3 ) == false );
    }
}

TEST_SUITE( "backends/null/NullEngine" )  {

    TEST_CASE( "LoadTexture returns the real PNG and JPEG sizes, by magic bytes not extension" )  {

        MemoryFileSystemFixture                   fixture;
        SunLight :: Engines :: Null :: NullEngine  engine;
        int                                        w = 0, h = 0;

        fixture.fs.files["a.png"]      = MakePng( 96, 64 );
        fixture.fs.files["real.jpg"]   = MakeJpeg( 640, 480 );
        fixture.fs.files["fake.png"]   = MakeJpeg( 1260, 920 );   // JPEG data with a .png name, like a real asset

        SunLight :: Base :: TextureHandle  hPng  = engine.LoadTexture( "a.png", w, h );
        REQUIRE( hPng != nullptr );
        CHECK( w == 96 );
        CHECK( h == 64 );

        SunLight :: Base :: TextureHandle  hJpg  = engine.LoadTexture( "real.jpg", w, h );
        REQUIRE( hJpg != nullptr );
        CHECK( w == 640 );
        CHECK( h == 480 );

        SunLight :: Base :: TextureHandle  hFake = engine.LoadTexture( "fake.png", w, h );
        REQUIRE( hFake != nullptr );
        CHECK( w == 1260 );
        CHECK( h == 920 );

        engine.UnloadTexture( hPng );
        engine.UnloadTexture( hJpg );
        engine.UnloadTexture( hFake );
    }

    TEST_CASE( "LoadTexture fails with 0x0 for a missing file or an unrecognised image" )  {

        MemoryFileSystemFixture                   fixture;
        SunLight :: Engines :: Null :: NullEngine  engine;
        int                                        w = 5, h = 5;

        fixture.fs.files["text.png"] = Bytes { 'h', 'e', 'l', 'l', 'o' };

        CHECK( engine.LoadTexture( "nope.png", w, h ) == nullptr );
        CHECK( w == 0 );
        CHECK( h == 0 );

        w = 5; h = 5;
        CHECK( engine.LoadTexture( "text.png", w, h ) == nullptr );
        CHECK( w == 0 );
        CHECK( h == 0 );
    }

    TEST_CASE( "LoadTexture reads through the virtual-path space (./ and backslashes normalised)" )  {

        MemoryFileSystemFixture                   fixture;
        SunLight :: Engines :: Null :: NullEngine  engine;
        int                                        w = 0, h = 0;

        fixture.fs.files["images/tile.png"] = MakePng( 16, 16 );

        SunLight :: Base :: TextureHandle  h1 = engine.LoadTexture( "./images/tile.png", w, h );
        REQUIRE( h1 != nullptr );
        CHECK( w == 16 );

        SunLight :: Base :: TextureHandle  h2 = engine.LoadTexture( "images\\tile.png", w, h );
        REQUIRE( h2 != nullptr );

        engine.UnloadTexture( h1 );
        engine.UnloadTexture( h2 );
    }

    TEST_CASE( "MeasureText is deterministic: characters x size / 2, counting UTF-8 code points" )  {

        SunLight :: Engines :: Null :: NullEngine  engine;

        CHECK( engine.MeasureText( "", 16 ) == 0 );
        CHECK( engine.MeasureText( "abcd", 16 ) == 32 );
        CHECK( engine.MeasureText( "abcd", 16 ) == engine.MeasureText( "abcd", 16 ) );
        CHECK( engine.MeasureText( "abc", 10 ) == 15 );

        // 3 code points, 6 bytes.
        CHECK( engine.MeasureText( "\xC3\xA9\xC3\xA9\xC3\xA9", 10 ) == 15 );

        CHECK( engine.MeasureText( nullptr, 16 ) == 0 );
    }

    TEST_CASE( "SetFont succeeds only for a font file that exists" )  {

        MemoryFileSystemFixture                   fixture;
        SunLight :: Engines :: Null :: NullEngine  engine;

        fixture.fs.files["fonts/f.ttf"] = Bytes { 1, 2, 3 };

        CHECK( engine.SetFont( "fonts/f.ttf" ) == true );
        CHECK( engine.SetFont( "fonts/missing.ttf" ) == false );
        CHECK( engine.SetFont( "" ) == false );
        CHECK( engine.SetFont( nullptr ) == false );
    }

    TEST_CASE( "Render targets and their textures are stable handles, and every draw call is a harmless no-op" )  {

        SunLight :: Engines :: Null :: NullEngine  engine;
        SunLight :: Base :: stColor                 color { 1, 2, 3, 4 };
        SunLight :: Base :: stRectangle             rect { 0, 0, 10, 10 };

        SunLight :: Base :: TextureHandle  hTarget = engine.LoadRenderTarget( 320, 240 );
        REQUIRE( hTarget != nullptr );
        CHECK( engine.GetRenderTargetTexture( hTarget ) == hTarget );

        engine.BeginRenderTarget( hTarget );
        engine.ClearBackground( color );
        engine.SetPixel( 1, 1, color );
        engine.DrawFilledRectangle( 0, 0, 5, 5, color );
        engine.DrawText( "x", 0, 0, 10, color );
        engine.DrawFPS( 0, 0 );
        engine.DrawTexture( hTarget, 0, 0, color );
        engine.DrawTextureScaled( hTarget, rect, rect, color );
        engine.EndRenderTarget();

        engine.UnloadRenderTarget( hTarget );
    }

    TEST_CASE( "GetApplicationDirectory returns this executable's own directory with a trailing separator" )  {

        SunLight :: Engines :: Null :: NullEngine  engine;
        std :: string                              strDir = engine.GetApplicationDirectory();

        REQUIRE( strDir.empty() == false );

        char  cLast = strDir[ strDir.size() - 1 ];
        bool  bEndsWithSeparator = ( cLast == '/' ) || ( cLast == '\\' );

        CHECK( bEndsWithSeparator );
    }
}

TEST_SUITE( "general/VirtualClock" )  {

    TEST_CASE( "Starts at zero and only moves when advanced, never backwards" )  {

        SunLight :: General :: VirtualClock  clock;

        CHECK( clock.NowMilliseconds() == 0 );
        CHECK( clock.GetSeconds() == 0.0 );

        clock.Advance( 1.5 );
        CHECK( clock.NowMilliseconds() == 1500 );

        clock.Advance( -10.0 );
        clock.Advance( 0.0 );
        CHECK( clock.NowMilliseconds() == 1500 );
    }

    TEST_CASE( "Whole frames of 1/60 s read back as intuitive milliseconds, with no drift over 18000 frames" )  {

        SunLight :: General :: VirtualClock  clock;

        for( int nFrame = 0; nFrame < 3; nFrame++ )
            clock.Advance( 1.0 / 60.0 );

        CHECK( clock.NowMilliseconds() == 50 );   // rounds, so 3 frames read 50, not 49

        for( int nFrame = 3; nFrame < 18000; nFrame++ )
            clock.Advance( 1.0 / 60.0 );

        CHECK( clock.GetSeconds() == doctest :: Approx( 300.0 ).epsilon( 1e-9 ) );
        CHECK( clock.NowMilliseconds() == 300000 );
    }
}

TEST_SUITE( "backends/null/NullWindow" )  {

    TEST_CASE( "Create always succeeds, reports the configured size, and ShouldClose never fires by itself" )  {

        SunLight :: General :: VirtualClock         clock;
        SunLight :: Window :: Null :: NullWindow    window( clock, false );

        CHECK( window.Create( 1260, 920, "t", true ) == true );
        CHECK( window.GetScreenWidth() == 1260 );
        CHECK( window.GetScreenHeight() == 920 );
        CHECK( window.ShouldClose() == false );

        window.EndFrame();
        window.EndFrame();
        CHECK( window.ShouldClose() == false );
    }

    TEST_CASE( "EndFrame advances the virtual clock by exactly 1/targetFps" )  {

        SunLight :: General :: VirtualClock         clock;
        SunLight :: Window :: Null :: NullWindow    window( clock, false );

        window.Create( 100, 100, "t", false );
        window.SetTargetFPS( 60 );

        for( int nFrame = 0; nFrame < 60; nFrame++ )
            window.EndFrame();

        CHECK( clock.GetSeconds() == doctest :: Approx( 1.0 ).epsilon( 1e-9 ) );

        // A different rate changes the step.
        window.SetTargetFPS( 10 );
        window.EndFrame();
        CHECK( clock.GetSeconds() == doctest :: Approx( 1.1 ).epsilon( 1e-9 ) );

        // A non-positive rate can't divide by zero: it falls back to 60.
        window.SetTargetFPS( 0 );
        window.EndFrame();
        CHECK( clock.GetSeconds() == doctest :: Approx( 1.1 + 1.0 / 60.0 ).epsilon( 1e-9 ) );
    }

    TEST_CASE( "GetElapsedTime is 0 before Create, counts virtual time from Create, restarts on the next Create, and is 0 after Close" )  {

        SunLight :: General :: VirtualClock         clock;
        SunLight :: Window :: Null :: NullWindow    window( clock, false );

        window.SetTargetFPS( 10 );
        CHECK( window.GetElapsedTime() == 0.0 );

        // Time passing before Create() isn't counted.
        clock.Advance( 5.0 );
        CHECK( window.GetElapsedTime() == 0.0 );

        window.Create( 100, 100, "t", false );
        CHECK( window.GetElapsedTime() == doctest :: Approx( 0.0 ) );

        for( int nFrame = 0; nFrame < 20; nFrame++ )
            window.EndFrame();

        CHECK( window.GetElapsedTime() == doctest :: Approx( 2.0 ).epsilon( 1e-9 ) );

        window.Close();
        CHECK( window.GetElapsedTime() == 0.0 );

        window.Create( 100, 100, "t", false );
        CHECK( window.GetElapsedTime() == doctest :: Approx( 0.0 ) );

        window.EndFrame();
        CHECK( window.GetElapsedTime() == doctest :: Approx( 0.1 ).epsilon( 1e-9 ) );

        // The shared clock itself never restarted.
        CHECK( clock.GetSeconds() == doctest :: Approx( 7.1 ).epsilon( 1e-9 ) );
    }

    TEST_CASE( "Close fires the close handlers, and fullscreen state round-trips" )  {

        SunLight :: General :: VirtualClock         clock;
        SunLight :: Window :: Null :: NullWindow    window( clock, false );
        int                                         nCalls = 0;

        window.Create( 100, 100, "t", false );
        window.AddCloseHandler( [&]( void ) { nCalls++; } );
        window.Close();
        CHECK( nCalls == 1 );

        CHECK( window.GetFullscreen() == false );
        window.SetFullscreen( true );
        CHECK( window.GetFullscreen() == true );
        window.SetFullscreen( false );
        CHECK( window.GetFullscreen() == false );
    }

    TEST_CASE( "Real-time pacing really waits: 3 frames at 100 FPS take at least ~25 ms; unlimited pacing does not" )  {

        SunLight :: General :: VirtualClock         clockPaced;
        SunLight :: Window :: Null :: NullWindow    paced( clockPaced, true );

        paced.Create( 100, 100, "t", false );
        paced.SetTargetFPS( 100 );

        auto  tStart = std :: chrono :: steady_clock :: now();

        for( int nFrame = 0; nFrame < 3; nFrame++ )
            paced.EndFrame();

        auto  nPacedMs = std :: chrono :: duration_cast<std :: chrono :: milliseconds>( std :: chrono :: steady_clock :: now() - tStart ).count();

        // 3 frames x 10 ms; allow slack below for timer granularity only.
        CHECK( nPacedMs >= 25 );

        SunLight :: General :: VirtualClock         clockFast;
        SunLight :: Window :: Null :: NullWindow    fast( clockFast, false );

        fast.Create( 100, 100, "t", false );
        fast.SetTargetFPS( 1 );   // would be 1 s per frame if it paced

        tStart = std :: chrono :: steady_clock :: now();

        for( int nFrame = 0; nFrame < 100; nFrame++ )
            fast.EndFrame();

        auto  nFastMs = std :: chrono :: duration_cast<std :: chrono :: milliseconds>( std :: chrono :: steady_clock :: now() - tStart ).count();

        CHECK( nFastMs < 500 );
        CHECK( clockFast.GetSeconds() == doctest :: Approx( 100.0 ) );   // 100 virtual seconds in a blink
    }
}

TEST_SUITE( "backends/null/NullBackend + renderer" )  {

    TEST_CASE( "Create with the null backend succeeds and installs the whole set; releasing it restores the defaults" )  {

        REQUIRE( SunLight :: Backends :: Null :: NullBackend :: IsActive() == false );

        {
            std :: string                       strError;
            std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig(), &strError );

            REQUIRE( pRenderer != nullptr );
            CHECK( strError.empty() );
            CHECK( SunLight :: Backends :: Null :: NullBackend :: IsActive() == true );

            // The whole set is what the process-global access points hand out.
            CHECK( dynamic_cast<SunLight :: Engines :: Null :: NullEngine*>( &SunLight :: Engines :: EngineFactory :: GetEngine() ) != nullptr );
            CHECK( dynamic_cast<SunLight :: Window :: Null :: NullWindow*>( &SunLight :: Window :: WindowFactory :: GetWindow() ) != nullptr );

            // The renderer's own input handler is the null one.
            CHECK( pRenderer -> GetInputHandler().IsKeyDown( SunLight :: Input :: KEY_ESCAPE ) == false );
            CHECK( pRenderer -> GetInputHandler().GetGamepadName( 0 ) != nullptr );
        }

        CHECK( SunLight :: Backends :: Null :: NullBackend :: IsActive() == false );

        // ...and the build's own defaults are back: no null engine/window.
        CHECK( dynamic_cast<SunLight :: Engines :: Null :: NullEngine*>( &SunLight :: Engines :: EngineFactory :: GetEngine() ) == nullptr );
        CHECK( dynamic_cast<SunLight :: Window :: Null :: NullWindow*>( &SunLight :: Window :: WindowFactory :: GetWindow() ) == nullptr );
    }

    TEST_CASE( "A renderer of another backend can't be created while a null one is live" )  {

        std :: unique_ptr<TileMapRenderer>  pNull = TileMapRenderer :: Create( NullConfig(), nullptr );
        REQUIRE( pNull != nullptr );

        RendererConfig  other;
        std :: string   strError;

        other.backend = RENDERER_BACKEND_RAYLIB;

        CHECK( TileMapRenderer :: Create( other, &strError ) == nullptr );
        CHECK( strError.find( "null-backend renderer is already active" ) != std :: string :: npos );

        // A second NULL renderer is fine - it shares the live backend.
        std :: unique_ptr<TileMapRenderer>  pSecond = TileMapRenderer :: Create( NullConfig(), nullptr );
        CHECK( pSecond != nullptr );
    }

    TEST_CASE( "A null renderer can't be created while a renderer on the default backend is live, and can once it's gone" )  {

        std :: string  strError;

        {
            RendererConfig                      real;
            std :: unique_ptr<TileMapRenderer>  pReal = TileMapRenderer :: Create( real, nullptr );

            REQUIRE( pReal != nullptr );

            CHECK( TileMapRenderer :: Create( NullConfig(), &strError ) == nullptr );
            CHECK( strError.find( "default backend is already active" ) != std :: string :: npos );
            CHECK( SunLight :: Backends :: Null :: NullBackend :: IsActive() == false );
        }

        // The classic constructor counts too.
        {
            TileMapRenderer  classic( 320, 240, "t", -1, false );

            CHECK( TileMapRenderer :: Create( NullConfig(), &strError ) == nullptr );
        }

        CHECK( TileMapRenderer :: Create( NullConfig(), &strError ) != nullptr );
    }

    TEST_CASE( "The set stays installed until the LAST null renderer goes away" )  {

        std :: unique_ptr<TileMapRenderer>  pFirst  = TileMapRenderer :: Create( NullConfig(), nullptr );
        std :: unique_ptr<TileMapRenderer>  pSecond = TileMapRenderer :: Create( NullConfig(), nullptr );

        REQUIRE( pFirst != nullptr );
        REQUIRE( pSecond != nullptr );

        pFirst.reset();
        CHECK( SunLight :: Backends :: Null :: NullBackend :: IsActive() == true );

        pSecond.reset();
        CHECK( SunLight :: Backends :: Null :: NullBackend :: IsActive() == false );
    }

    TEST_CASE( "Start/Run/Stop work headless: nMaxFrames ends the run cleanly, exactly N frames, no display" )  {

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig( 25 ), nullptr );
        UpdateCounter                       listener;

        REQUIRE( pRenderer != nullptr );
        pRenderer -> AddTileMapListener( &listener );

        REQUIRE( pRenderer -> Start() == true );
        CHECK( pRenderer -> Run() == true );

        CHECK( listener.nUpdates == 25 );

        // The budget is not an exit request.
        CHECK( pRenderer -> GetExitRequested() == false );

        pRenderer -> Stop();
    }

    TEST_CASE( "nMaxFrames counts frames since Start: a restarted renderer gets a fresh budget" )  {

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig( 5 ), nullptr );
        UpdateCounter                       listener;

        REQUIRE( pRenderer != nullptr );
        pRenderer -> AddTileMapListener( &listener );

        REQUIRE( pRenderer -> Start() == true );
        pRenderer -> Run();
        pRenderer -> Stop();
        CHECK( listener.nUpdates == 5 );

        REQUIRE( pRenderer -> Start() == true );
        pRenderer -> Run();
        pRenderer -> Stop();
        CHECK( listener.nUpdates == 10 );
    }

    TEST_CASE( "RequestExit ends a headless run before its frame budget, and finishes the frame it was made in" )  {

        // A generous budget only as a safety net: if RequestExit stopped
        // working this test would fail (wrong frame count), not hang.
        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig( 1000 ), nullptr );
        UpdateCounter                       listener;
        TileMapRenderer                     *pRaw = pRenderer.get();

        REQUIRE( pRenderer != nullptr );

        listener.onUpdate = [pRaw]( int nUpdate )  {
            if( nUpdate == 7 )
                pRaw -> RequestExit();
        };

        pRenderer -> AddTileMapListener( &listener );

        REQUIRE( pRenderer -> Start() == true );
        CHECK( pRenderer -> Run() == true );

        CHECK( listener.nUpdates == 7 );
        CHECK( pRenderer -> GetExitRequested() == true );

        pRenderer -> Stop();
    }

    TEST_CASE( "A TMX map loads and runs headless with no viewport configured (the default viewport is the render area)" )  {

        MemoryFileSystemFixture  fixture;
        const std :: string      strTmx =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"3\" tilewidth=\"16\" tileheight=\"16\">"
            "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"3\"><data encoding=\"csv\">0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
            "</map>";

        fixture.fs.files["maps/tiny.tmx"] = Bytes( strTmx.begin(), strTmx.end() );

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig( 10 ), nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        // Used to crash: LoadMap divided by the (zero) default viewport height.
        REQUIRE( pRenderer -> LoadMap( "maps/tiny.tmx" ) == true );

        SunLight :: TileMap :: stMapInfo  info {};

        REQUIRE( pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.mapSize.nWidth == 4 );
        CHECK( info.mapSize.nHeight == 3 );
        CHECK( info.tileSize.nWidth == 16 );
        CHECK( info.tileSize.nHeight == 16 );

        CHECK( pRenderer -> Run() == true );

        pRenderer -> Stop();
    }

    TEST_CASE( "The draw surface reports the configured size and a virtual elapsed time of frames x dt" )  {

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig( 120 ), nullptr );

        REQUIRE( pRenderer != nullptr );

        CHECK( pRenderer -> GetWindowWidth() == 320 );
        CHECK( pRenderer -> GetWindowHeight() == 240 );
        CHECK( pRenderer -> GetElapsedTime() == 0.0 );   // before Start

        REQUIRE( pRenderer -> Start() == true );
        CHECK( pRenderer -> GetElapsedTime() == doctest :: Approx( 0.0 ) );

        pRenderer -> Run();

        // 120 frames at 60 FPS = exactly 2 virtual seconds.
        CHECK( pRenderer -> GetElapsedTime() == doctest :: Approx( 2.0 ).epsilon( 1e-9 ) );

        pRenderer -> Stop();
        CHECK( pRenderer -> GetElapsedTime() == 0.0 );   // after Stop, like the real backend

        // Window-state setters work and read back.
        pRenderer -> SetDrawFPS( true );
        CHECK( pRenderer -> GetDrawFPS() == true );
    }

    TEST_CASE( "A long script wait completes in virtual time: 60 s of waiting takes 3600 frames and no real time" )  {

        // Budget = safety net (the wait should end the run at ~3600).
        std :: unique_ptr<TileMapRenderer>   pRenderer = TileMapRenderer :: Create( NullConfig( 20000 ), nullptr );
        SunLight :: Scripting :: ScriptProcessor  script;
        WaitRecorder                         recorder;
        UpdateCounter                        listener;
        int                                  nFrameOfCompletion = -1;

        REQUIRE( pRenderer != nullptr );

        // Compile the script with the null backend's clock already
        // installed, so the wait is measured on virtual time.
        script.AddOneParmCmd( SunLight :: Scripting :: WAIT_CMD, 60000 );
        script.AddOneParmCmd( SunLight :: Scripting :: PLAY_SONG_CMD, 1 );
        script.AddScriptListener( &recorder );
        script.Compile();

        TileMapRenderer  *pRaw = pRenderer.get();

        listener.onUpdate = [&]( int nUpdate )  {
            script.Run();

            if( ( recorder.nCommands > 0 ) && ( nFrameOfCompletion < 0 ) )  {
                nFrameOfCompletion = nUpdate;
                pRaw -> RequestExit();
            }
        };

        pRenderer -> AddTileMapListener( &listener );
        REQUIRE( pRenderer -> Start() == true );

        auto  tStart = std :: chrono :: steady_clock :: now();
        pRenderer -> Run();
        auto  nRealMs = std :: chrono :: duration_cast<std :: chrono :: milliseconds>( std :: chrono :: steady_clock :: now() - tStart ).count();

        // 60 s at 60 FPS: the wait releases once 3600 frames of virtual
        // time have passed (frame N sees N-1 frames elapsed).
        CHECK( nFrameOfCompletion >= 3600 );
        CHECK( nFrameOfCompletion <= 3603 );
        CHECK( nRealMs < 10000 );   // nowhere near a real minute

        pRenderer -> Stop();
    }

    TEST_CASE( "The full acceptance shape: 18000 frames = 300 virtual seconds at 60 FPS, run to the frame budget" )  {

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( NullConfig( 24000 ), nullptr );
        UpdateCounter                       listener;
        double                              dTimeAtFrame1 = -1.0, dTimeAtFrame9 = -1.0;
        TileMapRenderer                     *pRaw = pRenderer.get();

        REQUIRE( pRenderer != nullptr );

        listener.onUpdate = [&]( int nUpdate )  {
            if( nUpdate == 1 )
                dTimeAtFrame1 = pRaw -> GetElapsedTime();

            if( nUpdate == 5041 )
                dTimeAtFrame9 = pRaw -> GetElapsedTime();

            if( nUpdate == 18000 )
                pRaw -> RequestExit();
        };

        pRenderer -> AddTileMapListener( &listener );
        REQUIRE( pRenderer -> Start() == true );
        pRenderer -> Run();

        CHECK( listener.nUpdates == 18000 );
        CHECK( pRenderer -> GetElapsedTime() == doctest :: Approx( 300.0 ).epsilon( 1e-6 ) );
        CHECK( ( dTimeAtFrame9 - dTimeAtFrame1 ) == doctest :: Approx( 84.0 ).epsilon( 1e-6 ) );

        pRenderer -> Stop();
    }
}
