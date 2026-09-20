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

#ifndef __RAYLIBWINDOW_H__
#define __RAYLIBWINDOW_H__

#include "raylib.h"
#include "window/iwindow.h"
#include "window/closehandlerlist.h"

namespace SunLight  {
    namespace Window  {
        namespace Raylib  {

            /**
             * @brief Raylib window provider implementation.
             * All window/frame-loop specific raylib calls are implemented
             * here; drawing and loading stay in @see
             * SunLight::Engines::Raylib::RaylibEngine.
             */
            class RaylibWindow : public SunLight :: Window :: IWindow  {

                public:

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

                SunLight :: Window :: CloseHandlerList  m_CloseHandlers;
            };
        }
    }
}
#endif  /* __RAYLIBWINDOW_H__ */
