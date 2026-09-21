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

#ifndef __NULLENGINE_H__
#define __NULLENGINE_H__

#include "engines/iengine.h"

namespace SunLight  {
    namespace Engines  {
        namespace Null  {

            /**
             * @brief Engine that draws nothing and needs no GPU, window or
             * third-party library - the drawing half of the null backend.
             * Every draw call is a successful no-op; what has real,
             * deterministic behavior is what game logic reads back:
             *
             *  - LoadTexture returns the image's REAL pixel size, read from
             *    its header (PNG/JPEG, by magic bytes - see ReadImageSize)
             *    through SunLight::FileSystem, so archives and the read
             *    filter work headless too. Sprite bounds, collision insets
             *    and centering all derive from that size, so a 0x0 stub
             *    would silently break gameplay logic.
             *  - MeasureText is a fixed metric (not pixel-accurate to any
             *    real font): width = characters x fontSize / 2. Games lay
             *    text out from it, so it must be deterministic.
             *  - SetFont succeeds iff the font file exists.
             *  - Handles are tiny heap records (their size), so
             *    load/unload pairs behave like the real thing.
             */
            class NullEngine : public SunLight :: Engines :: IEngine  {

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

                std :: string GetApplicationDirectory( void ) override;

                void ClearBackground( SunLight :: Base :: stColor color ) override;
                void DrawFPS( int nPosX, int nPosY ) override;

                SunLight :: Base :: TextureHandle LoadRenderTarget( int nWidth, int nHeight ) override;
                void UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) override;
                void BeginRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) override;
                void EndRenderTarget( void ) override;
                SunLight :: Base :: TextureHandle GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget ) override;

                void DrawTextureScaled( SunLight :: Base :: TextureHandle hTexture,
                                        SunLight :: Base :: stRectangle source,
                                        SunLight :: Base :: stRectangle dest,
                                        SunLight :: Base :: stColor tint ) override;
            };
        }
    }
}
#endif  /* __NULLENGINE_H__ */
