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

            /**
             * @brief Must be implemented to provide texture loading from disk based on
             * chosen target engine;
             *
             * @param szFileName The texture file name to load;
             * @param nWidth Output parameter receiving the loaded texture width;
             * @param nHeight Output parameter receiving the loaded texture height;
             * @return Opaque handle to the loaded texture, or nullptr if operation failed;
             */
            virtual SunLight :: Base :: TextureHandle LoadTexture( const char *szFileName,
                                                                    int& nWidth,
                                                                    int& nHeight ) = 0;

            /**
             * @brief Must be implemented to unload previous texture data loaded by
             * @see LoadTexture() method on chosen target engine;
             *
             * @param hTexture The texture handle to unload;
             */
            virtual void UnloadTexture( SunLight :: Base :: TextureHandle hTexture ) = 0;

            /**
             * @brief Must be implemented to draw a pixel according the specified
             * position on chosen target engine;
             *
             * @param nPosX The X coordinate to plot pixel;
             * @param nPosY The Y coordinate to plot pixel;
             * @param color Color of pixel;
             */
            virtual void SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color ) = 0;

            /**
             * @brief Must be implemented to draw a texture at the specified position
             * on chosen target engine;
             *
             * @param hTexture The texture handle to draw;
             * @param nPosX X coordinate to draw texture;
             * @param nPosY Y coordinate to draw texture;
             * @param tint Color tint applied to texture;
             */
            virtual void DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                      int nPosX,
                                      int nPosY,
                                      SunLight :: Base :: stColor tint ) = 0;

            /**
             * @brief Must be implemented to draw part of a texture (defined by a
             * rectangle) with rotation and scale tiled into dest on chosen target
             * engine;
             *
             * @param hTexture The texture handle to draw;
             * @param source Source rectangle defining the texture area to be drawn;
             * @param dest Destination rectangle where texture will be tiled into;
             * @param origin Origin point used as rotation/scale reference;
             * @param rotation Rotation angle to be applied;
             * @param scale Scale factor to be applied;
             * @param tint Color tint applied to texture;
             */
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
