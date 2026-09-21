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

#include "backends/raylib/raylibwindow.h"

namespace SunLight  {
    namespace Window  {
        namespace Raylib  {

            /**
             * @brief Create the application window. SetConfigFlags only
             * takes effect if set before InitWindow, so the initial
             * resizeable state is applied here (later changes go through
             * SetWindowResizeable's live SetWindowState/ClearWindowState
             * path instead). Success is judged by IsWindowReady() rather
             * than InitWindow itself, which returns nothing.
             *
             * A window that must come up fullscreen is switched with exactly the
             * call SetFullscreen makes - so each strategy ends in the same state
             * as switching a window that was already open - INSIDE this call,
             * before it returns and so before any frame is drawn. That is the same
             * moment Caravellius' Display.init() switches today (after the window
             * exists, before the first frame), which is known to work on macOS.
             *
             * The two strategies differ in one thing: whether the window is hidden
             * while it switches, to avoid a windowed flash.
             *  - BORDERLESS_WINDOWED is created hidden (FLAG_WINDOW_HIDDEN is
             *    honoured at creation: GLFW's visible hint), switched, then shown, so
             *    the first thing on screen is the borderless window. Confirmed on
             *    macOS by hand: it comes up correctly.
             *  - REAL is NOT hidden. A window that entered macOS's real fullscreen
             *    while hidden was seen (macOS, tested by hand by the project owner
             *    through Scarab) to come up as an enlarged mirror of the desktop that
             *    never rendered anything; the very same call on a visible window works.
             *    So for REAL the window is created visible and switched immediately,
             *    which costs at most a brief windowed window before the switch.
             *
             * Raylib's own creation-time flags are not used for this on purpose:
             * FLAG_FULLSCREEN_MODE at InitWindow picks its own video mode (the
             * closest one at least as large as the requested size) instead of
             * going through the same window-size switch SetFullscreen does, and
             * FLAG_BORDERLESS_WINDOWED_MODE is not applied at creation at all.
             */
            bool RaylibWindow :: Create( int nWidth, int nHeight, const char *szTitle, bool bResizeable,
                                         bool bFullscreen, SunLight :: Window :: FullscreenStrategy strategy )  {

                bool          bHideWhileSwitching = ( bFullscreen &&
                                                      ( strategy == SunLight :: Window :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED ) );
                unsigned int  nFlags              = 0;

                if( bResizeable )
                    nFlags |= FLAG_WINDOW_RESIZABLE;

                if( bHideWhileSwitching )
                    nFlags |= FLAG_WINDOW_HIDDEN;

                if( nFlags != 0 )
                    ::SetConfigFlags( nFlags );

                ::InitWindow( nWidth, nHeight, szTitle );

                if( !::IsWindowReady() )
                    return false;

                if( bFullscreen )  {
                    SetFullscreen( true, strategy );

                    if( bHideWhileSwitching )
                        ::ClearWindowState( FLAG_WINDOW_HIDDEN );
                }

                return true;
            }

            /**
             * @brief Destroy the window (and its GL context). Fires the
             * registered close handlers first, while the context is still
             * fully valid (CloseWindow() itself is what tears it down), so
             * a handler can safely release context-tied state.
             */
            void RaylibWindow :: Close( void )  {

                m_CloseHandlers.Fire();

                ::CloseWindow();
            }

            /**
             * @brief Register a handler fired by Close() (see
             * IWindow::AddCloseHandler).
             */
            int RaylibWindow :: AddCloseHandler( const std :: function<void( void )> &handler )  {

                return m_CloseHandlers.Add( handler );
            }

            /**
             * @brief Unregister a handler (see IWindow::RemoveCloseHandler).
             */
            void RaylibWindow :: RemoveCloseHandler( int nId )  {

                m_CloseHandlers.Remove( nId );
            }

            /**
             * @brief True once the exit key was pressed or the window's
             * close button clicked (raylib's own WindowShouldClose).
             */
            bool RaylibWindow :: ShouldClose( void )  {

                return ::WindowShouldClose();
            }

            /**
             * @brief Set the exit key. raylib's ::SetExitKey is an
             * unconditional write to its own global input state
             * (CORE.Input.Keyboard.exitKey, confirmed in rcore.c - no
             * window/GL dependency), so this is safe to call before
             * Create() too.
             */
            void RaylibWindow :: SetExitKey( SunLight :: Input :: KeyboardKey key )  {

                ::SetExitKey( ( int ) key );
            }

            /**
             * @brief Begin a frame (raylib's BeginDrawing).
             */
            void RaylibWindow :: BeginFrame( void )  {

                ::BeginDrawing();
            }

            /**
             * @brief End a frame (raylib's EndDrawing: swaps buffers,
             * polls input events, and sleeps to hold the target FPS).
             */
            void RaylibWindow :: EndFrame( void )  {

                ::EndDrawing();
            }

            /**
             * @brief Enter or leave fullscreen. Defaults to raylib's real,
             * exclusive ToggleFullscreen() (which changes the monitor's
             * own video mode) rather than the borderless-windowed mode
             * this used to use unconditionally - switched 2026-08-26 after
             * a real, live-confirmed problem with borderless-windowed on
             * macOS: since it's an ordinary window merely resized to
             * cover the full screen (not a genuine OS-level fullscreen
             * space), the macOS Dock - when set to always show rather
             * than auto-hide - still draws on top of it, visibly covering
             * the bottom of the window. True ToggleFullscreen() enters a
             * real fullscreen space, which macOS itself hides the
             * Dock/menu bar behind automatically, matching what a player
             * actually expects from a fullscreen game. Confirmed via a
             * live A/B test that this genuinely fixes the Dock overlap
             * with no visible mode-switch flicker or resolution/scaling
             * artifacts on the platform this was tested on.
             *
             * FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED is kept available as
             * a fallback: unlike real ToggleFullscreen() (which switches
             * the monitor's actual video mode to the window's current
             * size, per GLFW's own glfwSetWindowMonitor() documentation),
             * borderless-windowed explicitly resizes to the monitor's
             * native resolution instead - safer on any platform/window
             * manager where a true video-mode switch misbehaves,
             * especially if the window's own size doesn't already match
             * the player's monitor.
             *
             * Switching strategy while already fullscreen in the other
             * one is unsupported (see IWindow::SetFullscreen) - both
             * raylib toggles assume theirs is the only active fullscreen
             * strategy, so mixing them without returning to windowed mode
             * first can leave stale window flags set.
             * @param bFullscreen true to enter fullscreen, false for windowed;
             * @param strategy Which fullscreen strategy to use when
             * entering fullscreen (ignored when bFullscreen is false);
             */
            void RaylibWindow :: SetFullscreen( bool bFullscreen, SunLight :: Window :: FullscreenStrategy strategy )  {

                if( strategy == SunLight :: Window :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED )  {
                    if( bFullscreen != ::IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE ) )
                        ::ToggleBorderlessWindowed();
                }
                else  {
                    if( bFullscreen != ::IsWindowState( FLAG_FULLSCREEN_MODE ) )
                        ::ToggleFullscreen();
                }
            }

            /**
             * @brief Query whether the window is currently fullscreen,
             * regardless of which strategy is active (see @see
             * SetFullscreen).
             */
            bool RaylibWindow :: GetFullscreen( void )  {

                return ::IsWindowState( FLAG_FULLSCREEN_MODE ) || ::IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE );
            }

            /**
             * @brief The strategy in effect, read from the same window
             * state flags GetFullscreen reads: borderless-windowed when that
             * flag is set, real fullscreen otherwise (also the answer while
             * windowed - only meaningful while fullscreen).
             */
            SunLight :: Window :: FullscreenStrategy RaylibWindow :: GetFullscreenStrategy( void )  {

                return ::IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE ) ? SunLight :: Window :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED :
                                                                          SunLight :: Window :: FULLSCREEN_STRATEGY_REAL;
            }

            /**
             * @brief Report real, wall-clock elapsed time in seconds (see
             * @see IWindow::GetElapsedTime). ::GetTime is raylib's own
             * high-resolution monotonic timer, counted from window
             * initialisation.
             */
            double RaylibWindow :: GetElapsedTime( void )  {

                return ::GetTime();
            }

            /**
             * @brief Allow or disallow live window resizing on an
             * already-created window (see @see IWindow::SetWindowResizeable).
             * SetWindowState/ClearWindowState (unlike SetConfigFlags, which
             * only takes effect if set before InitWindow) act directly on
             * the live window handle, so this works in either direction at
             * any point after the window already exists.
             */
            void RaylibWindow :: SetWindowResizeable( bool bResizeable )  {

                if( bResizeable )
                    ::SetWindowState( FLAG_WINDOW_RESIZABLE );
                else
                    ::ClearWindowState( FLAG_WINDOW_RESIZABLE );
            }

            /**
             * @brief Set the renderer's own target frame rate (see @see
             * IWindow::SetTargetFPS). ::SetTargetFPS is a plain runtime
             * setter (not a SetConfigFlags-before-InitWindow one-time
             * value), so this works at any point after the window already
             * exists, same as SetWindowResizeable above.
             */
            void RaylibWindow :: SetTargetFPS( int nTargetFps )  {

                ::SetTargetFPS( nTargetFps );
            }

            /**
             * @brief Set the application window's title (see @see
             * IWindow::SetWindowTitle). ::SetWindowTitle acts on the live
             * window handle (glfwSetWindowTitle underneath, on the GLFW
             * desktop backend), so this is only meaningful once the
             * window already exists.
             */
            void RaylibWindow :: SetWindowTitle( const char *szTitle )  {

                ::SetWindowTitle( szTitle );
            }

            /**
             * @brief Current window/screen width, in pixels.
             */
            int RaylibWindow :: GetScreenWidth( void )  {

                return ::GetScreenWidth();
            }

            /**
             * @brief Current window/screen height, in pixels.
             */
            int RaylibWindow :: GetScreenHeight( void )  {

                return ::GetScreenHeight();
            }
        }
    }
}
