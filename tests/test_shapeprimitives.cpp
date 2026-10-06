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
#include "renderer/shapeprimitives.h"

using namespace SunLight :: Renderer :: ShapePrimitives;

TEST_SUITE( "Shape primitives" )  {

    TEST_CASE( "A line's screen width is its width times the zoom, rounded half-up" )  {

        CHECK( ScreenLineWidth( 1.0, 1.0 ) == 1 );
        CHECK( ScreenLineWidth( 3.0, 2.0 ) == 6 );      // exact
        CHECK( ScreenLineWidth( 3.0, 1.5 ) == 5 );      // 4.5 rounds up
        CHECK( ScreenLineWidth( 2.5, 1.0 ) == 3 );      // 2.5 rounds up
        CHECK( ScreenLineWidth( 2.4, 1.0 ) == 2 );      // below the half rounds down
    }

    TEST_CASE( "A line is never narrower than one pixel" )  {

        CHECK( ScreenLineWidth( 0.25, 1.0 ) == 1 );     // 0.25 rounds to 0, so 1
        CHECK( ScreenLineWidth( 1.0, 0.5 ) == 1 );      // 0.5 rounds up to 1
        CHECK( ScreenLineWidth( 0.0, 1.0 ) == 1 );
    }
}
