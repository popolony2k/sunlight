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

#ifndef __NULLWINDOW_H__
#define __NULLWINDOW_H__

#include <chrono>
#include "window/iwindow.h"
#include "window/closehandlerlist.h"
#include "general/virtualclock.h"

namespace SunLight  {
    namespace Window  {
        namespace Null  {

            /**
             * @brief Window for windowless runs: nothing is displayed and
             * nothing can close it on its own - a headless run ends when the
             * renderer's RequestExit is called or its frame budget
             * (RendererConfig::nMaxFrames) runs out, never because of the
             * window.
             *
             * It also owns the run's notion of time. Each EndFrame() advances
             * the shared VirtualClock by one frame's worth (1 / target FPS),
             * so everything reading SunLight::General::Clock sees
             * frames x dt (counted, not summed - exact at every whole-second
             * boundary). GetElapsedTime() mirrors the real backend's
             * shape: 0.0 before Create(), counted from Create() and
             * restarting at zero on the next one, 0.0 again after Close().
             *
             * Pacing: with real-time pacing the loop is held to the target
             * FPS by sleeping in EndFrame (so wall-clock time also passes at
             * the game's own speed - what a run that must interact with
             * real-time things wants); without it, frames are produced as
             * fast as the game logic runs, compressing a long timeline into
             * a fraction of the time.
             */
            class NullWindow : public SunLight :: Window :: IWindow  {

                public:

                NullWindow( SunLight :: General :: VirtualClock &clock, bool bRealTimePacing );

                bool Create( int nWidth, int nHeight, const char *szTitle, bool bResizeable ) override;
                void Close( void ) override;
                bool ShouldClose( void ) override;
                void SetExitKey( SunLight :: Input :: KeyboardKey key ) override;
                int AddCloseHandler( const std :: function<void( void )> &handler ) override;
                void RemoveCloseHandler( int nId ) override;
                void BeginFrame( void ) override;
                void EndFrame( void ) override;

                void SetFullscreen( bool bFullscreen,
                                    SunLight :: Window :: FullscreenStrategy strategy =
                                        SunLight :: Window :: FULLSCREEN_STRATEGY_REAL ) override;
                bool GetFullscreen( void ) override;

                double GetElapsedTime( void ) override;

                void SetWindowResizeable( bool bResizeable ) override;
                void SetTargetFPS( int nTargetFps ) override;
                void SetWindowTitle( const char *szTitle ) override;

                int GetScreenWidth( void ) override;
                int GetScreenHeight( void ) override;

                private:

                typedef std :: chrono :: steady_clock  RealClock;

                SunLight :: General :: VirtualClock  &m_Clock;
                SunLight :: Window :: CloseHandlerList  m_CloseHandlers;
                bool                                 m_bRealTimePacing;
                bool                                 m_bCreated        = false;
                bool                                 m_bFullscreen     = false;
                int                                  m_nWidth          = 0;
                int                                  m_nHeight         = 0;
                int                                  m_nTargetFps      = 60;
                // This window's own timeline: frames since Create(), counted
                // (never summed) so GetElapsedTime is exact - see
                // VirtualClock. Separate from the shared m_Clock so a
                // restart's elapsed time is not a difference of two rounded
                // values.
                SunLight :: General :: VirtualClock  m_Elapsed;
                RealClock :: time_point              m_FrameDeadline;
            };
        }
    }
}
#endif  /* __NULLWINDOW_H__ */
