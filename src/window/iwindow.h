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

#ifndef __IWINDOW_H__
#define __IWINDOW_H__

#include <functional>
#include "input/iinputhandler.h"


namespace SunLight  {
    namespace Window  {

        /**
         * @brief Fullscreen strategy selectable via IWindow::SetFullscreen
         * (and IDrawSurface::SetFullscreen). Declared here, with the
         * window it configures; IEngine keeps same-type aliases under the
         * old IEngine:: names for source compatibility (deprecated).
         */
        enum FullscreenStrategy  {
            FULLSCREEN_STRATEGY_REAL               = 0,  // genuine OS-level fullscreen space (default)
            FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED = 1  // ordinary window resized to the monitor's native resolution
        };

        /**
         * @brief Backend window/frame-loop generic interface - everything
         * about the OS window itself and the per-frame present cycle:
         * creating/closing it, the close request (exit key / close
         * button), the begin/end-frame bracket the game loop draws
         * inside, and window-level state (size, title, fullscreen,
         * resizeability, target FPS, elapsed time). Split out from
         * @see SunLight::Engines::IEngine, which keeps everything that
         * draws or loads (textures, render targets, text, primitives):
         * IEngine acts on whatever is currently being drawn into, IWindow
         * acts on the window that ultimately shows it.
         *
         * A window and an engine are always chosen TOGETHER, as one
         * backend set, via the same build-time switch (see
         * WindowFactory) - an IWindow from one backend paired with an
         * IEngine from another isn't a supported combination, since a
         * backend's engine typically depends on state its own window
         * created (e.g. raylib's GL context).
         *
         * "The window is about to close" is a window event (see @see
         * AddCloseHandler): an engine that keeps state tied to the
         * window's render context subscribes to it and releases that state
         * itself, instead of the window (or the renderer) having to know
         * about, or reach into, the engine.
         */
        class IWindow  {

            public:

            virtual ~IWindow( void )  {}

            /**
             * @brief Must be implemented to create the application window
             * (and whatever render context the backend's IEngine draws
             * into) - must succeed before anything on IEngine that draws
             * or allocates a render target is used.
             *
             * @param nWidth Window width, in pixels;
             * @param nHeight Window height, in pixels;
             * @param szTitle The initial window title;
             * @param bResizeable Whether the user may resize the window,
             * applied at creation - the only point at which a backend can
             * set this without a live toggle (see @see
             * SetWindowResizeable for changing it afterward);
             * @param bFullscreen Whether the window must come up already
             * fullscreen, with no visible windowed phase first (the point of
             * asking here instead of calling @see SetFullscreen right after
             * Create: a backend can hide the window until the mode is
             * applied). Behaves exactly like @see SetFullscreen( true,
             * strategy ) called on the created window - same strategy, same
             * resulting state, @see GetFullscreen true afterwards - just
             * without the flash of an ordinary window. A backend without a
             * real window (the null backend) accepts and ignores it;
             * @param strategy Which fullscreen strategy to use when
             * bFullscreen is true (ignored otherwise);
             * @return true if the window was created and is usable, false
             * if creation failed;
             */
            virtual bool Create( int nWidth, int nHeight, const char *szTitle, bool bResizeable,
                                 bool bFullscreen = false,
                                 FullscreenStrategy strategy = FULLSCREEN_STRATEGY_REAL ) = 0;

            /**
             * @brief Must be implemented to destroy the window created by
             * @see Create, releasing the render context with it. Fires the
             * registered close handlers first (see @see AddCloseHandler),
             * while the context is still valid. Callers must not use any
             * IEngine draw/resource call afterward until a new window
             * exists again.
             */
            virtual void Close( void ) = 0;

            /**
             * @brief Must be implemented to report whether the window has
             * been asked to close - the configured exit key was pressed
             * (see @see SetExitKey) or the window's own close button was
             * clicked. Polled once per frame by the game loop; does NOT
             * include a programmatic exit request, which the renderer
             * tracks itself (see IDrawSurface::RequestExit).
             *
             * @return true if the window should close, false otherwise;
             */
            virtual bool ShouldClose( void ) = 0;

            /**
             * @brief Must be implemented to set which key, when pressed,
             * makes @see ShouldClose report true. KEY_NULL disables the
             * exit key entirely. Implementations must tolerate this being
             * called before @see Create (the raylib backend just records
             * the value in its own global input state, which has no window
             * dependency).
             *
             * @param key The exit key, or KEY_NULL to disable it;
             */
            virtual void SetExitKey( SunLight :: Input :: KeyboardKey key ) = 0;

            /**
             * @brief Must be implemented to register a handler fired every
             * time the window is about to be closed by @see Close - right
             * BEFORE the window/render context is destroyed, so the
             * handler can still safely release anything tied to that
             * context (e.g. a backend engine's custom font). Handlers
             * persist across Create()/Close() cycles and fire on each
             * Close(), in registration order; see CloseHandlerList for
             * the exact firing semantics every backend shares (handlers
             * may Add/Remove during a fire and must not throw).
             * The handler (and whatever it captures) must stay valid until
             * it is removed with @see RemoveCloseHandler.
             *
             * @param handler The handler to call;
             * @return An id to pass to RemoveCloseHandler (always > 0);
             */
            virtual int AddCloseHandler( const std :: function<void( void )> &handler ) = 0;

            /**
             * @brief Must be implemented to unregister a handler added by
             * @see AddCloseHandler. Unknown/already-removed ids are ignored.
             *
             * @param nId The id AddCloseHandler returned;
             */
            virtual void RemoveCloseHandler( int nId ) = 0;

            /**
             * @brief Must be implemented to begin a frame: everything the
             * game loop draws on the window itself (as opposed to inside an
             * IEngine render target) goes between this and @see EndFrame.
             */
            virtual void BeginFrame( void ) = 0;

            /**
             * @brief Must be implemented to end a frame: present what was
             * drawn since @see BeginFrame, process pending window/input
             * events, and pace the loop to the configured target FPS.
             */
            virtual void EndFrame( void ) = 0;

            /**
             * @brief Must be implemented to enter or leave fullscreen on
             * chosen target backend, using the requested strategy
             * (defaulting to FULLSCREEN_STRATEGY_REAL - see
             * RaylibWindow::SetFullscreen for why that's the default and
             * when FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED is worth
             * falling back to instead). Callers should only rely on @see
             * GetFullscreen reflecting the resulting state.
             *
             * Switching strategy while already fullscreen in the other one
             * is unsupported - call SetFullscreen( false ) first, then
             * re-enter fullscreen with the new strategy.
             *
             * @param bFullscreen true to enter fullscreen, false to return
             * to windowed mode;
             * @param strategy Which fullscreen strategy to use when
             * entering fullscreen (ignored when bFullscreen is false);
             */
            virtual void SetFullscreen( bool bFullscreen,
                                        FullscreenStrategy strategy = FULLSCREEN_STRATEGY_REAL ) = 0;

            /**
             * @brief Must be implemented to report whether the window is
             * currently fullscreen, regardless of which strategy is active
             * (see @see SetFullscreen).
             *
             * @return true if the window is fullscreen, false if windowed;
             */
            virtual bool GetFullscreen( void ) = 0;

            /**
             * @brief Must be implemented to report which fullscreen
             * strategy is in effect. Meaningful only while @see
             * GetFullscreen is true; while windowed it answers
             * FULLSCREEN_STRATEGY_REAL, the default.
             */
            virtual FullscreenStrategy GetFullscreenStrategy( void ) = 0;

            /**
             * @brief Must be implemented to report elapsed time in
             * seconds since the window backend's own timer started (for
             * raylib, its GetTime() - counted from GLFW's own
             * initialisation, which happens inside window creation, so 0.0
             * before @see Create and again after @see Close, and it
             * restarts from zero on every new window). High-resolution,
             * monotonic, and independent of the fixed per-tick dt the game
             * loop otherwise runs on, so it keeps advancing at real-world
             * speed even when the loop can't sustain its target FPS. Only
             * meaningful while the window exists.
             *
             * @return Elapsed time, in seconds;
             */
            virtual double GetElapsedTime( void ) = 0;

            /**
             * @brief Must be implemented to allow or disallow the user
             * resizing the window by dragging it's edges/corners, on an
             * already-created window. Only meaningful to call once the
             * window exists - callers are responsible for not calling this
             * before that (see TileMapRenderer::SetWindowResizeable, which
             * uses the bResizeable argument of @see Create instead for the
             * initial state).
             *
             * @param bResizeable true to allow resizing, false to disallow it;
             */
            virtual void SetWindowResizeable( bool bResizeable ) = 0;

            /**
             * @brief Must be implemented to set the renderer's own target
             * frame rate (the cap @see EndFrame paces the game loop
             * against, not the current measured FPS). Only meaningful to
             * call once the window exists - callers are responsible for
             * not calling this before that (see
             * TileMapRenderer::SetTargetFPS, which uses it's own
             * constructor-parameter path for the initial value). No getter
             * exists on this interface - the underlying backend (raylib)
             * has no API to read back a previously-set target FPS, so
             * TileMapRenderer::GetTargetFPS reads back it's own cached
             * value instead.
             *
             * @param nTargetFps The new target frame rate, in frames per second;
             */
            virtual void SetTargetFPS( int nTargetFps ) = 0;

            /**
             * @brief Must be implemented to set the application window's
             * title, replacing whatever title it was created with. Only
             * meaningful to call once the window exists - callers are
             * responsible for not calling this before that (see
             * TileMapRenderer::SetWindowTitle, which guards the same way
             * SetWindowResizeable's live-toggle path does).
             *
             * @param szTitle The new window title;
             */
            virtual void SetWindowTitle( const char *szTitle ) = 0;

            /**
             * @brief Must be implemented to return the current width, in
             * pixels, of the actual window/screen on chosen target backend -
             * as opposed to any fixed internal rendering resolution a
             * caller may be using, this always reflects the real, current
             * (and possibly just resized) window size.
             *
             * @return Current window/screen width, in pixels;
             */
            virtual int GetScreenWidth( void ) = 0;

            /**
             * @brief Must be implemented to return the current height, in
             * pixels, of the actual window/screen on chosen target backend
             * (see @see GetScreenWidth).
             *
             * @return Current window/screen height, in pixels;
             */
            virtual int GetScreenHeight( void ) = 0;
        };
    }
}

#endif  /* __IWINDOW_H__ */
