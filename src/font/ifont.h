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

#ifndef __IFONT_H__
#define __IFONT_H__

#include "base/color.h"

namespace SunLight {

    namespace Font {

        /**
         * @brief A font an @see IEngine loaded with LoadFont. Its size is passed
         * on every call, not fixed when the font loads, so one font serves every
         * size a caller asks for. Backends whose native font is sized at load
         * time (SDL_ttf, for one) keep one native font per size behind this.
         *
         * Ownership: the caller holds it through a unique_ptr. Destroying it
         * releases the font, unless the window has already closed - then the
         * engine has released it, and the object is simply invalid (IsValid is
         * false, and its drawing methods do nothing). A caller never gets a
         * freed font, whatever the order of teardown.
         */
        class IFont  {

            public:

            virtual ~IFont( void )  {}

            /**
             * @brief Whether the font can still be drawn with. False once the
             * window has closed, after which every method is a no-op.
             */
            virtual bool IsValid( void ) const = 0;

            /**
             * @brief Width of a line of text in this font at a size, in pixels.
             *
             * @param szText The text to measure;
             * @param nFontSize Font size, in pixels;
             */
            virtual int MeasureText( const char *szText, int nFontSize ) = 0;

            /**
             * @brief Draw a line of text in screen space with this font.
             *
             * @param szText The text to draw;
             * @param nPosX X coordinate to draw at;
             * @param nPosY Y coordinate to draw at;
             * @param nFontSize Font size, in pixels;
             * @param color Text color;
             */
            virtual void DrawText( const char *szText,
                                   int nPosX,
                                   int nPosY,
                                   int nFontSize,
                                   SunLight :: Base :: stColor color ) = 0;
        };
    }
}

#endif /* __IFONT_H__ */
