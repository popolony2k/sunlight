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

#include "raylibfontglyphs.h"


namespace  {

    // The Unicode blocks a TTF/OTF font is loaded with, each inclusive. Latin-1 starts at 160
    // (no-break space): 127..159 are DEL and the C1 control codes, which have no glyph.
    const int  __ASCII_FIRST           = 32;
    const int  __ASCII_LAST            = 126;
    const int  __LATIN1_FIRST          = 160;
    const int  __LATIN1_LAST           = 255;
    const int  __LATIN_EXTENDED_A_FIRST = 256;
    const int  __LATIN_EXTENDED_A_LAST  = 383;

    std :: vector<int> BuildCodepoints( void )  {

        const int           ranges[][2] = { { __ASCII_FIRST, __ASCII_LAST },
                                            { __LATIN1_FIRST, __LATIN1_LAST },
                                            { __LATIN_EXTENDED_A_FIRST, __LATIN_EXTENDED_A_LAST } };
        std :: vector<int>  codepoints;

        for( const auto &range : ranges )
            for( int nCodepoint = range[0]; nCodepoint <= range[1]; nCodepoint++ )
                codepoints.push_back( nCodepoint );

        return codepoints;
    }
}

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            const std :: vector<int>& TtfCodepoints( void )  {

                static const std :: vector<int>  codepoints = BuildCodepoints();

                return codepoints;
            }
        }
    }
}
