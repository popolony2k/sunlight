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

/*
 * T6 of the master plan: the code points a TTF/OTF font is loaded with on the raylib backend.
 * raylib's ::LoadFont builds a TTF font with the 95 ASCII glyphs alone, so every accented letter
 * was drawn as '?'. Loading the font itself needs a window (a GPU atlas), so the sample `text`
 * checks the drawing; this pins the list the backend hands to ::LoadFontEx.
 */

#include <doctest/doctest.h>
#include <algorithm>
#include <vector>
#include "backends/raylib/raylibfontglyphs.h"

using SunLight :: Engines :: Raylib :: TtfCodepoints;

namespace  {

    bool Has( int nCodepoint )  {
        const std :: vector<int>  &codepoints = TtfCodepoints();

        return std :: binary_search( codepoints.begin(), codepoints.end(), nCodepoint );
    }
}

TEST_SUITE( "backends/raylib font glyphs" )  {

    TEST_CASE( "Every printable ASCII character is there, as raylib's ::LoadFont had them" )  {

        for( int nCodepoint = ' '; nCodepoint <= '~'; nCodepoint++ )
            CHECK( Has( nCodepoint ) );
    }

    TEST_CASE( "The accented letters the samples and Portuguese/Spanish text use are there" )  {

        // ç ã õ é ê á í ó ú â ñ ¡ ¿ ü - and Latin Extended-A: ő (Hungarian), ł (Polish), š (Czech)
        for( int nCodepoint : { 0xE7, 0xE3, 0xF5, 0xE9, 0xEA, 0xE1, 0xED, 0xF3, 0xFA, 0xE2, 0xF1, 0xA1, 0xBF, 0xFC,
                                0x151, 0x142, 0x161 } )
            CHECK( Has( nCodepoint ) );
    }

    TEST_CASE( "Exactly ASCII, Latin-1 and Latin Extended-A: no control codes, nothing beyond" )  {

        const std :: vector<int>  &codepoints = TtfCodepoints();

        CHECK( codepoints.size() == 95 + 96 + 128 );        // 32..126, 160..255, 256..383
        CHECK( codepoints.front() == ' ' );
        CHECK( codepoints.back() == 0x17F );                // ſ, the last of Latin Extended-A

        CHECK( !Has( 0x1F ) );                              // a C0 control code
        CHECK( !Has( 0x7F ) );                              // DEL
        CHECK( !Has( 0x9F ) );                              // a C1 control code
        CHECK( !Has( 0x180 ) );                             // the first of Latin Extended-B
    }

    TEST_CASE( "The list is ascending with no duplicates, and the same list on every call" )  {

        const std :: vector<int>  &codepoints = TtfCodepoints();

        CHECK( std :: is_sorted( codepoints.begin(), codepoints.end() ) );
        CHECK( std :: adjacent_find( codepoints.begin(), codepoints.end() ) == codepoints.end() );
        CHECK( &TtfCodepoints() == &codepoints );
    }

    TEST_CASE( "The glyphs are generated at raylib's own TTF size, so existing text keeps its look" )  {

        CHECK( SunLight :: Engines :: Raylib :: TTF_GLYPH_SIZE == 32 );
    }
}
