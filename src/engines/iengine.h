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

#ifndef __IENGINE_H__
#define __IENGINE_H__

#include "base/color.h"
#include "base/primitives.h"


namespace SunLight  {
    namespace Engines  {

        /**
         * @brief Backend rendering engine generic interface.
         * Every graphics backend (raylib, SDL, ...) implements this using
         * its own native types internally, exposing only SunLight native
         * primitives here.
         */
        class IEngine  {

            public:

            virtual ~IEngine( void )  {}

            virtual SunLight :: Base :: TextureHandle LoadTexture( const char *szFileName,
                                                                    int& nWidth,
                                                                    int& nHeight ) = 0;
            virtual void UnloadTexture( SunLight :: Base :: TextureHandle hTexture ) = 0;

            virtual void SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color ) = 0;

            virtual void DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                      int nPosX,
                                      int nPosY,
                                      SunLight :: Base :: stColor tint ) = 0;

            virtual void DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                                           SunLight :: Base :: stRectangle source,
                                           SunLight :: Base :: stRectangle dest,
                                           SunLight :: Base :: stVector2D origin,
                                           float rotation,
                                           float scale,
                                           SunLight :: Base :: stColor tint ) = 0;
        };
    }
}

#endif  /* __IENGINE_H__ */
