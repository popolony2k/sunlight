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

#include <doctest/doctest.h>
#include "base/color.h"
#include "base/primitives.h"

using namespace SunLight :: Base;

TEST_SUITE( "base/primitives" )  {

    TEST_CASE( "stColor predefined constants hold expected channel values" )  {

        stColor white = WHITE_COLOR;
        stColor black = BLACK_COLOR;

        CHECK( white.nRed   == 255 );
        CHECK( white.nGreen == 255 );
        CHECK( white.nBlue  == 255 );
        CHECK( white.nAlpha == 255 );

        CHECK( black.nRed   == 0 );
        CHECK( black.nGreen == 0 );
        CHECK( black.nBlue  == 0 );
        CHECK( black.nAlpha == 255 );
    }

    TEST_CASE( "stRectangle stores its fields as given" )  {

        stRectangle rect { 1.0f, 2.0f, 3.0f, 4.0f };

        CHECK( rect.x      == doctest :: Approx( 1.0f ) );
        CHECK( rect.y      == doctest :: Approx( 2.0f ) );
        CHECK( rect.width  == doctest :: Approx( 3.0f ) );
        CHECK( rect.height == doctest :: Approx( 4.0f ) );
    }

    TEST_CASE( "stVector2D stores its fields as given" )  {

        stVector2D vec { 5.0f, 6.0f };

        CHECK( vec.x == doctest :: Approx( 5.0f ) );
        CHECK( vec.y == doctest :: Approx( 6.0f ) );
    }

    TEST_CASE( "TextureHandle defaults are comparable to nullptr" )  {

        TextureHandle hTexture = nullptr;

        CHECK( hTexture == nullptr );
    }
}
