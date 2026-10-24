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
#include "backends/raylib/raylibfont.h"

#include <memory>
#include <vector>

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief Raylib engine provider implementation.
             * All specific raylib calls are implemented here.
             */
            class RaylibEngine : public SunLight :: Engines :: IEngine  {

                public:

                RaylibEngine( void );
                ~RaylibEngine( void );

                SunLight :: Base :: TextureHandle LoadTexture( const char* szFileName,
                                                                int& nWidth,
                                                                int& nHeight ) override;
                void UnloadTexture( SunLight :: Base :: TextureHandle hTexture ) override;

                void DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                   int nPosX,
                                   int nPosY,
                                   SunLight :: Base :: stColor tint ) override;

                void DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                                       SunLight :: Base :: stRectangle source,
                                       SunLight :: Base :: stRectangle dest,
                                       SunLight :: Base :: stVector2D origin,
                                       float fRotation,
                                       float fScale,
                                       SunLight :: Base :: stColor tint ) override;

                void DrawFilledRectangle( int nPosX,
                                          int nPosY,
                                          int nWidth,
                                          int nHeight,
                                          SunLight :: Base :: stColor color ) override;

                void DrawLine( float fX0,
                               float fY0,
                               float fX1,
                               float fY1,
                               float fThickness,
                               SunLight :: Base :: stColor color ) override;

                void DrawEllipseOutline( float fCenterX,
                                         float fCenterY,
                                         float fRadiusX,
                                         float fRadiusY,
                                         SunLight :: Base :: stColor color ) override;

                bool SetFont( std :: shared_ptr<SunLight :: Font :: IFont> pFont ) override;
                std :: shared_ptr<SunLight :: Font :: IFont> GetFont( void ) override;

                std :: shared_ptr<SunLight :: Font :: IFont> LoadFont( const char* szFilePath ) override;

                void BeginClip( SunLight :: Base :: stRectangle rect ) override;
                void EndClip( void ) override;

                void DrawText( const char* szText,
                               int nPosX,
                               int nPosY,
                               int nFontSize,
                               SunLight :: Base :: stColor color ) override;

                int MeasureText( const char* szText, int nFontSize ) override;

                std :: string GetApplicationDirectory( void ) override;

                void ClearBackground( SunLight :: Base :: stColor color ) override;
                void DrawFPS( int nPosX, int nPosY ) override;

                SunLight :: Base :: TextureHandle LoadRenderTarget( int nWidth, int nHeight ) override;
                void UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) override;
                void BeginRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) override;
                void EndRenderTarget( void ) override;
                void SetBlendMode( SunLight :: Base :: BlendMode mode ) override;
                SunLight :: Base :: BlendMode GetBlendMode( void ) override;
                SunLight :: Base :: TextureHandle GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget ) override;

                void DrawTextureScaled( SunLight :: Base :: TextureHandle hTexture,
                                        SunLight :: Base :: stRectangle source,
                                        SunLight :: Base :: stRectangle dest,
                                        SunLight :: Base :: stColor tint ) override;

                void DrawTextureRotated( SunLight :: Base :: TextureHandle hTexture,
                                         SunLight :: Base :: stRectangle source,
                                         SunLight :: Base :: stRectangle dest,
                                         SunLight :: Base :: stVector2D origin,
                                         float rotation,
                                         SunLight :: Base :: stColor tint ) override;

                void DrawTextRotated( const char* szText,
                                      int nPosX,
                                      int nPosY,
                                      int nFontSize,
                                      float rotation,
                                      SunLight :: Base :: stColor color ) override;

                private:

                // Releases this class's own GPU-context-tied state - the
                // active custom font and every font LoadFont still holds.
                // Registered as a close handler on the backend's window (see
                // the constructor), so it runs right before the window/context
                // is destroyed.
                void ReleaseWindowState( void );

                // Id of this engine's close handler on the default
                // backend window (see IWindow::AddCloseHandler).
                int   m_nCloseHandlerId   = 0;

                // The font SetFont selected, while it is still valid; nullptr
                // when the built-in font is to be used (I3).
                SunLight :: Font :: IFont* ActiveFont( void );

                // Every font LoadFont handed out that may still be alive. The
                // engine holds them weakly: each font owns its own state, and
                // the window's close handler marks the live ones invalid, so a
                // font destroyed afterwards does not release them twice.
                std :: vector<std :: weak_ptr<__stRaylibFont>>  m_LiveFonts;

                // Clips in effect, innermost last. The scissor always matches the last one (see ApplyClip).
                std :: vector<SunLight :: Base :: stRectangle>  m_ClipStack;

                void ApplyClip( SunLight :: Base :: stRectangle rect );

                // The font SetFont selected (I3), shared with whoever else holds
                // it; nullptr selects raylib's own built-in GetFontDefault(),
                // which this class never owns and must never Unload.
                std :: shared_ptr<SunLight :: Font :: IFont>  m_pActiveFont;

                // The mode SetBlendMode set last (I2).
                SunLight :: Base :: BlendMode  m_BlendMode = SunLight :: Base :: BLEND_MODE_NORMAL;

                // BLEND_MODE_MULTIPLY's shader, which premultiplies the source colour by its alpha
                // (see SetBlendMode). Loaded on the first multiply, as it needs the GL context, and
                // released with the window (ReleaseWindowState). m_bMultiplyShaderTried keeps a GL
                // without GLSL 330 from retrying (and warning) on every multiply.
                ::Shader  m_MultiplyShader         {};
                bool      m_bMultiplyShaderLoaded  = false;
                bool      m_bMultiplyShaderTried   = false;

                // Whether m_MultiplyShader is the active shader now.
                bool      m_bMultiplyShaderActive  = false;

                bool LoadMultiplyShader( void );
            };
        }
    }
}
#endif /* __RAYLIBENGINE_H__ */

