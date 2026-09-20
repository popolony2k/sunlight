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

#ifndef __MOCK_WINDOW_H__
#define __MOCK_WINDOW_H__

#include "window/iwindow.h"
#include "window/windowfactory.h"
#include <string>

/**
 * @brief Test double for SunLight::Window::IWindow - records call counts/
 * arguments and returns per-instance, configurable results, with no real
 * window/display involved. Together with MockEngine this makes
 * TileMapRenderer's Start()/Run()/Stop() unit-testable end to end.
 */
class MockWindow : public SunLight :: Window :: IWindow  {

    public:

    // Scripted behavior.
    bool                                bCreateResult              = true;
    // ShouldClose() starts reporting true once this many frames have
    // ended (EndFrame calls) - so Run() executes exactly that many frames.
    // -1 (default) means it never reports true on its own.
    int                                 nFramesUntilShouldClose    = -1;
    bool                                bFullscreen                = false;
    double                              dElapsedTimeResult         = 0.0;
    int                                 nScreenWidthResult         = 0;
    int                                 nScreenHeightResult        = 0;

    // Recorded calls.
    int                                 nCreateCalls               = 0;
    int                                 nCloseCalls                = 0;
    int                                 nShouldCloseCalls          = 0;
    int                                 nSetExitKeyCalls           = 0;
    int                                 nBeginFrameCalls           = 0;
    int                                 nEndFrameCalls             = 0;
    int                                 nSetFullscreenCalls        = 0;
    int                                 nSetWindowResizeableCalls  = 0;
    int                                 nSetTargetFPSCalls         = 0;
    int                                 nSetWindowTitleCalls       = 0;

    int                                 nLastCreateWidth           = 0;
    int                                 nLastCreateHeight          = 0;
    std :: string                       strLastCreateTitle;
    bool                                bLastCreateResizeable      = false;
    SunLight :: Input :: KeyboardKey    lastExitKey                = SunLight :: Input :: KEY_NULL;
    SunLight :: Engines :: IEngine :: FullscreenStrategy lastFullscreenStrategy =
        SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL;
    bool                                bWindowResizeable          = false;
    int                                 nLastTargetFps             = 0;
    std :: string                       strLastWindowTitle;

    // If set, Close() snapshots this counter (typically a MockEngine's
    // nOnWindowClosingCalls) so a test can prove the engine's own
    // pre-close hook ran BEFORE the window was closed.
    const int                           *pEngineOnWindowClosingCalls = nullptr;
    int                                 nEngineOnWindowClosingSeenAtClose = -1;

    bool Create( int nWidth, int nHeight, const char *szTitle, bool bResizeable )  {
        nCreateCalls++;
        nLastCreateWidth      = nWidth;
        nLastCreateHeight     = nHeight;
        strLastCreateTitle    = szTitle;
        bLastCreateResizeable = bResizeable;
        return bCreateResult;
    }

    void Close( void )  {
        nCloseCalls++;

        if( pEngineOnWindowClosingCalls )
            nEngineOnWindowClosingSeenAtClose = *pEngineOnWindowClosingCalls;
    }

    bool ShouldClose( void )  {
        nShouldCloseCalls++;

        return ( nFramesUntilShouldClose >= 0 ) && ( nEndFrameCalls >= nFramesUntilShouldClose );
    }

    void SetExitKey( SunLight :: Input :: KeyboardKey key )  {
        nSetExitKeyCalls++;
        lastExitKey = key;
    }

    void BeginFrame( void )  {
        nBeginFrameCalls++;
    }

    void EndFrame( void )  {
        nEndFrameCalls++;
    }

    void SetFullscreen( bool bValue, SunLight :: Engines :: IEngine :: FullscreenStrategy strategy = SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL )  {
        nSetFullscreenCalls++;
        bFullscreen = bValue;
        lastFullscreenStrategy = strategy;
    }

    bool GetFullscreen( void )  {
        return bFullscreen;
    }

    // Settable (set dElapsedTimeResult directly, or AdvanceElapsedTime) so
    // elapsed-time-dependent logic can be exercised deterministically.
    double GetElapsedTime( void )  {
        return dElapsedTimeResult;
    }

    void AdvanceElapsedTime( double dSeconds )  {
        dElapsedTimeResult += dSeconds;
    }

    void SetWindowResizeable( bool bValue )  {
        nSetWindowResizeableCalls++;
        bWindowResizeable = bValue;
    }

    void SetTargetFPS( int nTargetFps )  {
        nSetTargetFPSCalls++;
        nLastTargetFps = nTargetFps;
    }

    void SetWindowTitle( const char *szTitle )  {
        nSetWindowTitleCalls++;
        strLastWindowTitle = szTitle;
    }

    int GetScreenWidth( void )  {
        return nScreenWidthResult;
    }

    int GetScreenHeight( void )  {
        return nScreenHeightResult;
    }
};

/**
 * @brief RAII fixture that points WindowFactory :: GetWindow() at a single
 * MockWindow instance for the fixture's lifetime, restoring the default
 * (real) backend on destruction.
 */
class MockWindowFixture  {

    public:

    MockWindow  window;

    MockWindowFixture( void )  {
        SunLight :: Window :: WindowFactory :: SetWindow( &window );
    }

    ~MockWindowFixture( void )  {
        SunLight :: Window :: WindowFactory :: SetWindow( nullptr );
    }
};

#endif /* __MOCK_WINDOW_H__ */
