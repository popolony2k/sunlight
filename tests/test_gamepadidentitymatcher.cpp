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
 * F7b of the master plan: MatchGamepadsByName, the pure-logic core of correlating raylib's
 * own gamepad list against SDL's own, independently-enumerated one. This is the only part of
 * F7b fully testable without real hardware - SDLGamepadHaptics's actual SDL calls can only be
 * smoke-tested with zero real devices (see test_gamepadhaptics.cpp).
 */

#include <doctest/doctest.h>
#include "input/gamepadidentitymatcher.h"

using namespace SunLight :: Input;

TEST_SUITE( "input/MatchGamepadsByName" )  {

    TEST_CASE( "Empty inputs match nothing" )  {

        CHECK( MatchGamepadsByName( {}, {} ).empty() );
        CHECK( MatchGamepadsByName( { "Xbox Controller" }, {} ) == std :: vector<int>{ -1 } );
        CHECK( MatchGamepadsByName( {}, { "Xbox Controller" } ).empty() );
    }

    TEST_CASE( "A single exact name match pairs up" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "Xbox Controller" }, { "Xbox Controller" } );

        REQUIRE( matches.size() == 1 );
        CHECK( matches[ 0 ] == 0 );
    }

    TEST_CASE( "A name with no match reports -1, without disturbing other matches" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "Xbox Controller", "Totally Unknown Pad" },
                                                            { "Xbox Controller" } );

        REQUIRE( matches.size() == 2 );
        CHECK( matches[ 0 ] == 0 );
        CHECK( matches[ 1 ] == -1 );
    }

    TEST_CASE( "Names match independent of list order on either side" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "PS5 Controller", "Xbox Controller" },
                                                            { "Xbox Controller", "PS5 Controller" } );

        REQUIRE( matches.size() == 2 );
        CHECK( matches[ 0 ] == 1 );   // "PS5 Controller" is candidate index 1
        CHECK( matches[ 1 ] == 0 );   // "Xbox Controller" is candidate index 0
    }

    TEST_CASE( "Two identical-model controllers pair up by their own relative connection order" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "Xbox Controller", "Xbox Controller" },
                                                            { "Xbox Controller", "Xbox Controller" } );

        REQUIRE( matches.size() == 2 );
        CHECK( matches[ 0 ] == 0 );
        CHECK( matches[ 1 ] == 1 );
    }

    TEST_CASE( "A duplicate-name target does not steal the candidate a unique target already claimed" )  {

        // Only one real "Xbox Controller" candidate exists; two targets share that name
        // (one legitimately matching, one that must come back unmatched rather than
        // double-claiming the same candidate).
        std :: vector<int>  matches = MatchGamepadsByName( { "Xbox Controller", "Xbox Controller" },
                                                            { "Xbox Controller" } );

        REQUIRE( matches.size() == 2 );
        CHECK( matches[ 0 ] == 0 );
        CHECK( matches[ 1 ] == -1 );
    }

    TEST_CASE( "Each candidate is used at most once, even with three-way duplicates" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "Pad", "Pad", "Pad" }, { "Pad", "Pad" } );

        REQUIRE( matches.size() == 3 );
        CHECK( matches[ 0 ] == 0 );
        CHECK( matches[ 1 ] == 1 );
        CHECK( matches[ 2 ] == -1 );
    }

    TEST_CASE( "An empty name is a valid, matchable string like any other" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "" }, { "" } );

        REQUIRE( matches.size() == 1 );
        CHECK( matches[ 0 ] == 0 );
    }

    TEST_CASE( "Exactly one name on each side matches unconditionally, even when the names disagree "
               "(regression: a real Xbox One S controller reads as \"Xbox Wireless Controller\" "
               "through raylib/GLFW and \"Xbox One S Controller\" through SDL - confirmed live)" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "Xbox Wireless Controller" }, { "Xbox One S Controller" } );

        REQUIRE( matches.size() == 1 );
        CHECK( matches[ 0 ] == 0 );
    }

    TEST_CASE( "The single-candidate shortcut does not apply once there is more than one on either side" )  {

        std :: vector<int>  matches = MatchGamepadsByName( { "Xbox Wireless Controller", "PS5 Controller" },
                                                            { "Totally Different Name" } );

        REQUIRE( matches.size() == 2 );
        CHECK( matches[ 0 ] == -1 );
        CHECK( matches[ 1 ] == -1 );
    }
}
