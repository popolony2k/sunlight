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
#include "general/helper.h"

using namespace SunLight :: General;

TEST_SUITE( "general/Helper" )  {

    TEST_CASE( "Random stays within the requested inclusive range" )  {

        for( int nCount = 0; nCount < 1000; nCount++ )  {
            int nValue = Helper :: Random( 1, 10 );

            CHECK( nValue >= 1 );
            CHECK( nValue <= 10 );
        }
    }

    TEST_CASE( "Random with equal bounds always returns that value" )  {

        for( int nCount = 0; nCount < 20; nCount++ )  {
            CHECK( Helper :: Random( 5, 5 ) == 5 );
        }
    }

    TEST_CASE( "RoundHalfUp rounds ties toward +infinity on both sides of zero, unlike std::round" )  {

        // The whole reason this exists instead of std::round: std::round ties AWAY from
        // zero, so a negative and a positive half-integer round in opposite directions
        // relative to each other - exactly what opened a real 1px gap between two otherwise
        // contiguous screen-space tiles (see tests/test_pixelsnapping.cpp).
        CHECK( Helper :: RoundHalfUp( -30.5f ) == -30.0f );     // std::round would give -31
        CHECK( Helper :: RoundHalfUp( 91.5f ) == 92.0f );        // same as std::round here
        CHECK( Helper :: RoundHalfUp( -0.5f ) == 0.0f );         // std::round would give -1
        CHECK( Helper :: RoundHalfUp( 0.5f ) == 1.0f );
        CHECK( Helper :: RoundHalfUp( 2.4f ) == 2.0f );
        CHECK( Helper :: RoundHalfUp( -2.4f ) == -2.0f );
    }
}
