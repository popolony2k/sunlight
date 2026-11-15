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
 * SDLGamepadHaptics keeps each pad's controller open (see its doc comment: closing an SDL controller
 * right after a rumble blocked the calling thread for about 11 ms, measured live). These tests run
 * it against a fake ISDLControllerApi - no SDL device is ever opened, so unlike the cases in
 * test_gamepadhaptics.cpp they cannot reach a real controller, whatever is plugged in.
 */

#include <doctest/doctest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "backends/sdl/sdlgamepadhaptics.h"
#include "fake_sdlcontrollerapi.h"
#include "mock_input.h"
#include "mock_clock.h"

using SunLight :: Input :: SDL :: ISDLControllerApi;
using SunLight :: Input :: SDL :: SDLGamepadHaptics;

namespace  {

    const int  PAD = 0;

    using FakeSDL :: FakeLog;
    using FakeSDL :: FakeDevice;
    using FakeSDL :: FakeControllerApi;

    // One raylib-side pad (PAD, the mock's own name) and one SDL device: exactly one on each side,
    // so they are the same device (gamepadidentitymatcher.h's single-candidate rule).
    struct Rig  {
        FakeLog                               log;
        MockInputHandler                      input;
        FakeControllerApi                     *pApi = nullptr;      // owned by haptics
        std :: unique_ptr<SDLGamepadHaptics>  pHaptics;

        Rig( void )  {
            std :: unique_ptr<FakeControllerApi>  pFake = std :: make_unique<FakeControllerApi>( log );

            pApi = pFake.get();
            pApi -> devices.push_back( FakeDevice { "Xbox One S Controller", 7, true, true } );
            input.availableGamepads.insert( PAD );
            pHaptics = std :: make_unique<SDLGamepadHaptics>( input, std :: move( pFake ) );
        }
    };
}

TEST_SUITE( "backends/sdl/SDLGamepadHaptics controllers" )  {

    TEST_CASE( "A pad's controller is opened once and reused by every call; the destructor closes it" )  {

        MockClockFixture  clockFixture;
        Rig               rig;

        CHECK( rig.pHaptics -> IsSupported( PAD ) );
        rig.pHaptics -> SetVibration( PAD, 1.0f, 0.5f, 0.2f );
        rig.pHaptics -> Stop( PAD );
        CHECK( rig.pHaptics -> IsSupported( PAD ) );
        rig.pHaptics -> SetVibration( PAD, 0.25f, 0.25f, 0.0f );

        CHECK( rig.log.nOpens == 1 );
        CHECK( rig.log.nCloses == 0 );                   // never closed between calls

        REQUIRE( rig.log.rumbles.size() == 3 );
        CHECK( rig.log.rumbles[0].nInstanceId == 7 );
        CHECK( rig.log.rumbles[0].nLow == 65535 );
        CHECK( rig.log.rumbles[0].nHigh == 32767 );       // 0.5 x 65535, truncated
        CHECK( rig.log.rumbles[0].nDurationMs == 200 );
        CHECK( rig.log.rumbles[1].nLow == 0 );            // Stop: zero strengths
        CHECK( rig.log.rumbles[1].nHigh == 0 );
        CHECK( rig.log.rumbles[1].nDurationMs == 0 );

        rig.pHaptics.reset();
        CHECK( rig.log.nCloses == 1 );
    }

    TEST_CASE( "A rumble's deadline stops it on the same open controller, without closing it" )  {

        MockClockFixture  clockFixture;
        Rig               rig;

        rig.pHaptics -> SetVibration( PAD, 1.0f, 1.0f, 0.2f );
        clockFixture.clock.Advance( 199 );
        rig.pHaptics -> Update();
        CHECK( rig.log.rumbles.size() == 1 );             // not yet

        clockFixture.clock.Advance( 1 );
        rig.pHaptics -> Update();
        REQUIRE( rig.log.rumbles.size() == 2 );
        CHECK( rig.log.rumbles[1].nLow == 0 );
        CHECK( rig.log.nOpens == 1 );
        CHECK( rig.log.nCloses == 0 );
    }

    TEST_CASE( "A controller that was unplugged, or a different device now matching the pad, is closed and the matching one opened" )  {

        MockClockFixture  clockFixture;
        Rig               rig;

        REQUIRE( rig.pHaptics -> IsSupported( PAD ) );

        // The same device reported detached: the next call reopens it.
        rig.pApi -> devices[0].bAttached = false;
        rig.pHaptics -> SetVibration( PAD, 1.0f, 1.0f, 0.0f );
        CHECK( rig.log.nCloses == 1 );
        CHECK( rig.log.nOpens == 2 );
        rig.pApi -> devices[0].bAttached = true;

        // A different device now matches the pad while the open one is still attached: two SDL
        // devices, and only the new one (instance 9) has the pad's own name ("Mock Gamepad", see
        // mock_input.h), so the match moves to it. The open controller is closed, the new one opened.
        rig.pApi -> devices[0].strName = "Another Controller";
        rig.pApi -> devices.insert( rig.pApi -> devices.begin(), FakeDevice { "Mock Gamepad", 9, true, true } );
        rig.pHaptics -> SetVibration( PAD, 1.0f, 1.0f, 0.0f );
        CHECK( rig.log.nCloses == 2 );
        CHECK( rig.log.nOpens == 3 );
        CHECK( rig.log.rumbles.back().nInstanceId == 9 );

        rig.pHaptics -> SetVibration( PAD, 1.0f, 1.0f, 0.0f );
        CHECK( rig.log.nOpens == 3 );                     // reused again
    }

    TEST_CASE( "A pad that is no longer available has its controller closed, and nothing rumbles" )  {

        MockClockFixture  clockFixture;
        Rig               rig;

        REQUIRE( rig.pHaptics -> IsSupported( PAD ) );

        rig.input.availableGamepads.clear();

        CHECK( rig.pHaptics -> IsSupported( PAD ) == false );
        rig.pHaptics -> SetVibration( PAD, 1.0f, 1.0f, 0.2f );
        rig.pHaptics -> Stop( PAD );

        CHECK( rig.log.nCloses == 1 );
        CHECK( rig.log.rumbles.empty() );
    }

    TEST_CASE( "A controller with no rumble is not supported; one that cannot be opened is not either, and nothing is called on it" )  {

        MockClockFixture  clockFixture;

        {
            Rig  rig;

            rig.pApi -> devices[0].bHasRumble = false;
            CHECK( rig.pHaptics -> IsSupported( PAD ) == false );
        }

        {
            Rig  rig;

            rig.pApi -> bOpenFails = true;
            CHECK( rig.pHaptics -> IsSupported( PAD ) == false );
            rig.pHaptics -> SetVibration( PAD, 1.0f, 1.0f, 0.2f );
            CHECK( rig.log.rumbles.empty() );
            CHECK( rig.log.nOpens == 0 );
        }
    }
}
