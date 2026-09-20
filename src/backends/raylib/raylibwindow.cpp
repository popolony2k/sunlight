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
             */
            bool RaylibWindow :: Create( int nWidth, int nHeight, const char *szTitle, bool bResizeable )  {

                if( bResizeable )
                    ::SetConfigFlags( FLAG_WINDOW_RESIZABLE );

                ::InitWindow( nWidth, nHeight, szTitle );

                return ::IsWindowReady();
            }

            /**
             * @brief Destroy the window (and its GL context).
             */
            void RaylibWindow :: Close( void )  {

                ::CloseWindow();
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
            void RaylibWindow :: SetFullscreen( bool bFullscreen, SunLight :: Engines :: IEngine :: FullscreenStrategy strategy )  {

                if( strategy == SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED )  {
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
