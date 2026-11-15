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

#ifndef __RAYLIBFONTGLYPHS_H__
#define __RAYLIBFONTGLYPHS_H__

#include <vector>


namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief The size, in pixels, a TTF/OTF font's glyphs are generated at - raylib's own
             * FONT_TTF_DEFAULT_SIZE (rtext.c), the size its ::LoadFont uses, so a font loaded with
             * TtfCodepoints() is drawn exactly as before for every glyph it had then.
             */
            const int  TTF_GLYPH_SIZE = 32;

            /**
             * @brief The Unicode code points a TTF/OTF font is loaded with: ASCII (32..126), the
             * Latin-1 supplement (160..255) and Latin Extended-A (256..383) - the letters of the
             * Western and Central European languages (Portuguese, Spanish, French, German,
             * Polish, Czech, ...). raylib's ::LoadFont builds a TTF/OTF font with the 95 ASCII
             * glyphs alone (FONT_TTF_DEFAULT_NUMCHARS, rtext.c), so every other character in the
             * font file - `ç`, `ã`, `ñ`, `¿` - was drawn as `?`; a bitmap (.fnt) font is not
             * affected, it brings its own glyph set. Each code point is a glyph rasterized when
             * the font loads, so the list is kept to these blocks rather than the whole font.
             * @return The code points, ascending and without duplicates;
             */
            const std :: vector<int>& TtfCodepoints( void );
        }
    }
}

#endif /* __RAYLIBFONTGLYPHS_H__ */
