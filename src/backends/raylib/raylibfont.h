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

#ifndef __RAYLIBFONT_H__
#define __RAYLIBFONT_H__

#include "raylib.h"
#include "engines/ifont.h"

#include <memory>

// Extra spacing (in pixels) DrawTextEx adds between characters, on top of
// whatever a font's own glyph metrics already provide - 0 means "use the
// font as authored, no extra letter-spacing", the correct default for a
// generic engine primitive with no per-call spacing parameter of it's own.
#define __DEFAULT_TEXT_SPACING   0.0f

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief The state a RaylibFont owns: the raylib font, and whether
             * it still exists. The engine keeps weak references to these, so
             * the window's close handler can release every live font while
             * its context is valid, and mark it invalid, without touching a
             * font that is already gone.
             */
            struct __stRaylibFont  {
                ::Font  font   {};
                bool    bValid = true;
            };

            /**
             * @brief A font LoadFont returned (see @see IFont), backed by a
             * raylib Font. Its methods are no-ops once the window has closed
             * and the engine released it.
             */
            class RaylibFont : public SunLight :: Engines :: IFont  {

                public:

                explicit RaylibFont( std :: shared_ptr<__stRaylibFont> pState );

                ~RaylibFont( void ) override;

                bool IsValid( void ) const override;

                int MeasureText( const char *szText, int nFontSize ) override;

                void DrawText( const char *szText,
                               int nPosX,
                               int nPosY,
                               int nFontSize,
                               SunLight :: Base :: stColor color ) override;

                private:

                std :: shared_ptr<__stRaylibFont>  m_pState;
            };
        }
    }
}

#endif /* __RAYLIBFONT_H__ */
