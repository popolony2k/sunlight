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
 * F7a of the master plan: IGamepadHaptics/GamepadHapticsFactory/NullGamepadHaptics. Nothing
 * here is wired to a real device (F7b stays open until a vibrating platform is chosen).
 */

#include <doctest/doctest.h>
#include "input/nullgamepadhaptics.h"
#include "input/gamepadhapticsfactory.h"
#include "mock_gamepadhaptics.h"

using namespace SunLight :: Input;

TEST_SUITE( "input/NullGamepadHaptics" )  {

    TEST_CASE( "IsSupported is always false" )  {

        NullGamepadHaptics  haptics;

        CHECK( haptics.IsSupported( 0 ) == false );
        CHECK( haptics.IsSupported( 1 ) == false );
    }

    TEST_CASE( "SetVibration and Stop never error, for any argument" )  {

        NullGamepadHaptics  haptics;

        haptics.SetVibration( 0, 1.0f, 1.0f, 5.0f );
        haptics.SetVibration( 0, -1.0f, 2.0f, -5.0f );   // out-of-range strengths too
        haptics.Stop( 0 );
        haptics.Stop( 99 );   // a pad id nothing ever configured

        CHECK( true );   // reaching here without a crash is the whole assertion
    }

    TEST_CASE( "A game that checks IsSupported can skip an effect entirely" )  {

        NullGamepadHaptics  haptics;
        bool                bEffectPlayed = false;

        if( haptics.IsSupported( 0 ) )  {
            haptics.SetVibration( 0, 1.0f, 1.0f, 1.0f );
            bEffectPlayed = true;
        }

        CHECK( bEffectPlayed == false );
    }
}

TEST_SUITE( "input/GamepadHapticsFactory" )  {

    TEST_CASE( "CreateGamepadHaptics returns a NullGamepadHaptics by default" )  {

        std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics();

        REQUIRE( pHaptics != nullptr );
        CHECK( dynamic_cast<NullGamepadHaptics*>( pHaptics.get() ) != nullptr );
    }

    TEST_CASE( "SetCreator swaps in a mock, and resets back to the default on nullptr" )  {

        {
            MockGamepadHapticsFixture  fixture;

            std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics();

            CHECK( pHaptics.get() == fixture.pLastCreated );
        }

        std :: unique_ptr<IGamepadHaptics>  pRestored = GamepadHapticsFactory :: CreateGamepadHaptics();

        CHECK( dynamic_cast<NullGamepadHaptics*>( pRestored.get() ) != nullptr );
    }

    TEST_CASE( "Calls reach the mock with the exact arguments" )  {

        MockGamepadHapticsFixture           fixture;
        std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics();

        pHaptics -> SetVibration( 3, 0.25f, 0.75f, 1.5f );
        pHaptics -> Stop( 3 );

        REQUIRE( fixture.pLastCreated -> vibrationCalls.size() == 1 );
        CHECK( fixture.pLastCreated -> vibrationCalls[ 0 ].nGamePadId == 3 );
        CHECK( fixture.pLastCreated -> vibrationCalls[ 0 ].fLeftMotor == 0.25f );
        CHECK( fixture.pLastCreated -> vibrationCalls[ 0 ].fRightMotor == 0.75f );
        CHECK( fixture.pLastCreated -> vibrationCalls[ 0 ].fDuration == 1.5f );

        REQUIRE( fixture.pLastCreated -> stopCalls.size() == 1 );
        CHECK( fixture.pLastCreated -> stopCalls[ 0 ] == 3 );
    }

    TEST_CASE( "A game that checks IsSupported through the mock can skip an effect" )  {

        MockGamepadHapticsFixture           fixture;
        std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics();

        fixture.pLastCreated -> bSupportedResult = false;

        bool  bEffectPlayed = false;

        if( pHaptics -> IsSupported( 0 ) )  {
            pHaptics -> SetVibration( 0, 1.0f, 1.0f, 1.0f );
            bEffectPlayed = true;
        }

        CHECK( bEffectPlayed == false );
        CHECK( fixture.pLastCreated -> vibrationCalls.empty() );
    }
}
