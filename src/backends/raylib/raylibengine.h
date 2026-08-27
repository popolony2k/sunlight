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

#ifndef __RAYLIBENGINE_H__
#define __RAYLIBENGINE_H__

#include "raylib.h"
#include "engines/iengine.h"

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief Raylib engine provider implementation.
             * All specific raylib calls are implemented here.
             */
            class RaylibEngine : public SunLight :: Engines :: IEngine  {

                public:

                SunLight :: Base :: TextureHandle LoadTexture( const char *szFileName,
                                                                int& nWidth,
                                                                int& nHeight ) override;
                void UnloadTexture( SunLight :: Base :: TextureHandle hTexture ) override;

                void SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color ) override;

                void DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                   int nPosX,
                                   int nPosY,
                                   SunLight :: Base :: stColor tint ) override;

                void DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                                       SunLight :: Base :: stRectangle source,
                                       SunLight :: Base :: stRectangle dest,
                                       SunLight :: Base :: stVector2D origin,
                                       float rotation,
                                       float scale,
                                       SunLight :: Base :: stColor tint ) override;

                void DrawFilledRectangle( int nPosX,
                                          int nPosY,
                                          int nWidth,
                                          int nHeight,
                                          SunLight :: Base :: stColor color ) override;

                bool SetFont( const char *szFilePath ) override;

                void DrawText( const char *szText,
                               int nPosX,
                               int nPosY,
                               int nFontSize,
                               SunLight :: Base :: stColor color ) override;

                int MeasureText( const char *szText, int nFontSize ) override;

                void OnWindowClosing( void ) override;

                std :: string GetApplicationDirectory( void ) override;

                void SetFullscreen( bool bFullscreen, FullscreenStrategy strategy = FULLSCREEN_STRATEGY_REAL ) override;
                bool GetFullscreen( void ) override;

                void SetWindowResizeable( bool bResizeable ) override;
                void SetWindowTitle( const char *szTitle ) override;

                int GetScreenWidth( void ) override;
                int GetScreenHeight( void ) override;

                SunLight :: Base :: TextureHandle LoadRenderTarget( int nWidth, int nHeight ) override;
                void UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) override;
                void BeginRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) override;
                void EndRenderTarget( void ) override;
                SunLight :: Base :: TextureHandle GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget ) override;

                void DrawTextureScaled( SunLight :: Base :: TextureHandle hTexture,
                                        SunLight :: Base :: stRectangle source,
                                        SunLight :: Base :: stRectangle dest,
                                        SunLight :: Base :: stColor tint ) override;

                private:

                // Shared by DrawText/MeasureText - the font either of them
                // should use right now (see m_CurrentFont's own comment).
                Font  GetActiveFont( void );

                // Font currently used by DrawText, loaded via SetFont - only
                // valid (and only ever Unload'd) when m_bCustomFontLoaded is
                // true; otherwise DrawText falls back to raylib's own
                // built-in GetFontDefault(), which this class never owns
                // and must never Unload.
                Font  m_CurrentFont       {};
                bool  m_bCustomFontLoaded = false;
            };
        }
    }
}
#endif  /* __RAYLIBENGINE_H__ */

