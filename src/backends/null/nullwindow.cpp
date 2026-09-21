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

#include "backends/null/nullwindow.h"
#include <thread>

// Frame rate assumed when a caller sets a non-positive target FPS (which
// would otherwise divide by zero).
#define __FALLBACK_FPS   60

namespace SunLight  {
    namespace Window  {
        namespace Null  {

            /**
             * @brief Constructor.
             * @param clock The virtual clock this window advances each frame;
             * @param bRealTimePacing true to hold the loop to the target FPS
             * in real time, false to run frames as fast as possible;
             */
            NullWindow :: NullWindow( SunLight :: General :: VirtualClock &clock, bool bRealTimePacing ) :
                                      m_Clock( clock ),
                                      m_bRealTimePacing( bRealTimePacing )  {
            }

            /**
             * @brief "Create" the window: it just records its size and
             * starts its elapsed-time count. Always succeeds.
             */
            bool NullWindow :: Create( int nWidth, int nHeight, const char *, bool )  {

                m_nWidth         = nWidth;
                m_nHeight        = nHeight;
                m_dCreateSeconds = m_Clock.GetSeconds();
                m_FrameDeadline  = RealClock :: now();
                m_bCreated       = true;

                return true;
            }

            /**
             * @brief "Close" the window: fires the close handlers (same
             * contract as every backend - before anything is torn down),
             * then stops the elapsed-time count.
             */
            void NullWindow :: Close( void )  {

                m_CloseHandlers.Fire();
                m_bCreated = false;
            }

            /**
             * @brief Never asks to close by itself - there's no exit key
             * press or close button here; the renderer's own exit request
             * and frame budget are what end a run.
             */
            bool NullWindow :: ShouldClose( void )  {

                return false;
            }

            /**
             * @brief No keyboard, nothing to configure.
             */
            void NullWindow :: SetExitKey( SunLight :: Input :: KeyboardKey )  {
            }

            int NullWindow :: AddCloseHandler( const std :: function<void( void )> &handler )  {

                return m_CloseHandlers.Add( handler );
            }

            void NullWindow :: RemoveCloseHandler( int nId )  {

                m_CloseHandlers.Remove( nId );
            }

            /**
             * @brief Nothing to begin.
             */
            void NullWindow :: BeginFrame( void )  {
            }

            /**
             * @brief End a frame: advance virtual time by one frame, and -
             * with real-time pacing - sleep until this frame's real-time
             * deadline. The deadline advances by a fixed step (no drift); if
             * the loop is running behind it is reset to "now" rather than
             * trying to catch up with a burst of unpaced frames.
             */
            void NullWindow :: EndFrame( void )  {

                int     nFps = ( m_nTargetFps > 0 ) ? m_nTargetFps : __FALLBACK_FPS;
                double  dDt  = 1.0 / ( double ) nFps;

                m_Clock.Advance( dDt );

                if( !m_bRealTimePacing )
                    return;

                m_FrameDeadline += std :: chrono :: duration_cast<RealClock :: duration>( std :: chrono :: duration<double>( dDt ) );

                RealClock :: time_point  now = RealClock :: now();

                if( m_FrameDeadline > now )
                    std :: this_thread :: sleep_until( m_FrameDeadline );
                else
                    m_FrameDeadline = now;
            }

            void NullWindow :: SetFullscreen( bool bFullscreen, SunLight :: Window :: FullscreenStrategy )  {

                m_bFullscreen = bFullscreen;
            }

            bool NullWindow :: GetFullscreen( void )  {

                return m_bFullscreen;
            }

            /**
             * @brief Virtual seconds since Create(); 0.0 before it and after Close().
             */
            double NullWindow :: GetElapsedTime( void )  {

                return m_bCreated ? ( m_Clock.GetSeconds() - m_dCreateSeconds ) : 0.0;
            }

            /**
             * @brief Nothing to resize.
             */
            void NullWindow :: SetWindowResizeable( bool )  {
            }

            /**
             * @brief Sets the frame step (dt = 1 / FPS) and, with real-time
             * pacing, the real-time rate.
             */
            void NullWindow :: SetTargetFPS( int nTargetFps )  {

                m_nTargetFps = nTargetFps;
            }

            void NullWindow :: SetWindowTitle( const char * )  {
            }

            /**
             * @brief The size the window was "created" with.
             */
            int NullWindow :: GetScreenWidth( void )  {

                return m_nWidth;
            }

            int NullWindow :: GetScreenHeight( void )  {

                return m_nHeight;
            }
        }
    }
}
