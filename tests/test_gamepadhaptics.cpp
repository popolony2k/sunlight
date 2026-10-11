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
 * F7a of the master plan: IGamepadHaptics/GamepadHapticsFactory/NullGamepadHaptics.
 * F7b: SDLGamepadHaptics, the real backend (now GamepadHapticsFactory's build-time default).
 * Nothing here can be proven against a real vibrating device without one attached - these
 * tests cover what's provable with zero real gamepads (safe init/teardown, no crash, nothing
 * reads as supported), and test_gamepadidentitymatcher.cpp covers the actual device-matching
 * logic in full, since that part needs no real hardware at all.
 *
 * IMPORTANT - a "SDLGamepadHaptics" test case is NOT automatically hardware-safe just because
 * it is a unit test: MatchGamepadsByName's single-candidate shortcut (exactly one gamepad on
 * each side is always the same device - see gamepadidentitymatcher.h) means a mock reporting
 * exactly one available raylib-side pad WILL match a real SDL device if the machine running
 * the suite happens to have exactly one real controller connected at the time - confirmed live:
 * an earlier version of the "SetVibration and Stop never error for an unmatched pad" case did
 * exactly this, firing a real, full-strength rumble at a real Xbox controller mid test run.
 * Every case here that calls IsSupported/SetVibration/Stop with the intent of it being
 * unmatched must report at least two available raylib-side pads (bypassing the shortcut), never
 * just one.
 *
 * A second real-hardware finding from the same session: SetVibration's own duration never
 * auto-stopped at all - SDL2's internal rumble expiration only runs inside SDL_JoystickUpdate(),
 * which nothing else in this engine ever calls. IGamepadHaptics::Update() (new, must be called
 * once per frame by whoever owns the instance) is sunlight's own replacement, timed against
 * SunLight::General::Clock - see the "Update never errors..." case below, using MockClock to
 * cross a tracked deadline deterministically.
 */

#include <doctest/doctest.h>
#include "input/nullgamepadhaptics.h"
#include "input/gamepadhapticsfactory.h"
#include "backends/sdl/sdlgamepadhaptics.h"
#include "mock_gamepadhaptics.h"
#include "mock_input.h"
#include "mock_clock.h"

using namespace SunLight :: Input;

TEST_SUITE( "input/NullGamepadHaptics" )  {

    TEST_CASE( "IsSupported is always false" )  {

        NullGamepadHaptics  haptics;

        CHECK( haptics.IsSupported( 0 ) == false );
        CHECK( haptics.IsSupported( 1 ) == false );
    }

    TEST_CASE( "SetVibration, Stop and Update never error, for any argument" )  {

        NullGamepadHaptics  haptics;

        haptics.SetVibration( 0, 1.0f, 1.0f, 5.0f );
        haptics.SetVibration( 0, -1.0f, 2.0f, -5.0f );   // out-of-range strengths too
        haptics.Stop( 0 );
        haptics.Stop( 99 );   // a pad id nothing ever configured
        haptics.Update();

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

    TEST_CASE( "CreateGamepadHaptics returns the real SDLGamepadHaptics backend by default (F7b)" )  {

        MockInputHandler                    dummyInput;
        std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics( dummyInput );

        REQUIRE( pHaptics != nullptr );
        CHECK( dynamic_cast<SunLight :: Input :: SDL :: SDLGamepadHaptics*>( pHaptics.get() ) != nullptr );
    }

    TEST_CASE( "SetCreator swaps in a mock, and resets back to the real default on nullptr" )  {

        MockInputHandler  dummyInput;

        {
            MockGamepadHapticsFixture  fixture;

            std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics( dummyInput );

            CHECK( pHaptics.get() == fixture.pLastCreated );
        }

        std :: unique_ptr<IGamepadHaptics>  pRestored = GamepadHapticsFactory :: CreateGamepadHaptics( dummyInput );

        CHECK( dynamic_cast<SunLight :: Input :: SDL :: SDLGamepadHaptics*>( pRestored.get() ) != nullptr );
    }

    TEST_CASE( "Calls reach the mock with the exact arguments" )  {

        MockInputHandler                    dummyInput;
        MockGamepadHapticsFixture           fixture;
        std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics( dummyInput );

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

        MockInputHandler                    dummyInput;
        MockGamepadHapticsFixture           fixture;
        std :: unique_ptr<IGamepadHaptics>  pHaptics = GamepadHapticsFactory :: CreateGamepadHaptics( dummyInput );

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

TEST_SUITE( "backends/sdl/SDLGamepadHaptics" )  {

    TEST_CASE( "Construction and destruction never error, with zero real gamepads" )  {

        MockInputHandler                             dummyInput;
        SunLight :: Input :: SDL :: SDLGamepadHaptics haptics( dummyInput );

        CHECK( true );   // reaching here without a crash is the whole assertion
    }

    TEST_CASE( "IsSupported is false when the raylib-side pad isn't available at all" )  {

        MockInputHandler                             dummyInput;
        SunLight :: Input :: SDL :: SDLGamepadHaptics haptics( dummyInput );

        dummyInput.availableGamepads.clear();

        CHECK( haptics.IsSupported( 0 ) == false );
    }

    TEST_CASE( "IsSupported is false when the raylib-side pad is available but no real SDL device matches it" )  {

        MockInputHandler                             dummyInput;
        SunLight :: Input :: SDL :: SDLGamepadHaptics haptics( dummyInput );

        // Two mock pads, not one: MatchGamepadsByName's single-candidate shortcut (exactly one
        // on each side is always the same device, see gamepadidentitymatcher.h) must never
        // apply here, whatever real hardware happens to be plugged into the machine actually
        // running this test - a unit test must never be able to reach real hardware regardless
        // of what is connected (see below: this exact gap once fired a real, full-strength
        // rumble at a real controller connected during a live test run). Both report the mock's
        // own fixed name ("Mock Gamepad"), which no real device is ever named, so the
        // name-match fallback also can't accidentally succeed.
        dummyInput.availableGamepads.insert( 0 );
        dummyInput.availableGamepads.insert( 1 );

        CHECK( haptics.IsSupported( 0 ) == false );
    }

    TEST_CASE( "SetVibration and Stop never error for an unmatched pad, without ever reaching real hardware" )  {

        MockInputHandler                             dummyInput;
        SunLight :: Input :: SDL :: SDLGamepadHaptics haptics( dummyInput );

        // Two mock pads - see the previous test case for why this, not one, is required to
        // keep this call from ever matching a real connected device (confirmed live: with only
        // one, this exact call rumbled a real Xbox controller at full strength during a test
        // run while it happened to be connected - a unit test must never be able to do that).
        dummyInput.availableGamepads.insert( 0 );
        dummyInput.availableGamepads.insert( 1 );

        haptics.SetVibration( 0, 1.0f, 1.0f, 1.0f );
        haptics.Stop( 0 );

        CHECK( true );   // reaching here without a crash is the whole assertion
    }

    TEST_CASE( "Two instances can coexist and each tear down independently without error" )  {

        MockInputHandler                              dummyInputA;
        MockInputHandler                              dummyInputB;
        SunLight :: Input :: SDL :: SDLGamepadHaptics *pA = new SunLight :: Input :: SDL :: SDLGamepadHaptics( dummyInputA );
        SunLight :: Input :: SDL :: SDLGamepadHaptics *pB = new SunLight :: Input :: SDL :: SDLGamepadHaptics( dummyInputB );

        delete pA;

        CHECK( pB -> IsSupported( 0 ) == false );   // B's own subsystem reference is still live

        delete pB;

        CHECK( true );
    }

    TEST_CASE( "Update never errors before or after a timed vibration's own deadline, with zero real gamepads "
               "(F7b regression: SDL's own duration-based auto-stop never fires without this - confirmed live, "
               "a real controller kept rumbling indefinitely until this was added)" )  {

        MockClockFixture                              clockFixture;
        MockInputHandler                              dummyInput;
        SunLight :: Input :: SDL :: SDLGamepadHaptics haptics( dummyInput );

        dummyInput.availableGamepads.insert( 0 );
        dummyInput.availableGamepads.insert( 1 );   // two, not one - see this suite's own warning above

        haptics.SetVibration( 0, 1.0f, 1.0f, 0.2f );

        haptics.Update();   // before the deadline - must not crash

        clockFixture.clock.Advance( 300 );

        haptics.Update();   // after the deadline - must not crash, and must not re-fire on a
        haptics.Update();   // second call once already handled

        CHECK( true );   // reaching here without a crash is the whole assertion - the actual
                          // real-device stop timing can only be proven on real hardware (see
                          // the sample), the same honest limit every other F7b test accepts
    }

    TEST_CASE( "Update handles several pads with different deadlines, some expired and some not, "
               "without crashing" )  {

        // Exercises Update()'s own two-phase collect-then-stop loop (it must not erase from
        // m_VibrationDeadlines while iterating it directly - that would invalidate the
        // iterator) under AddressSanitizer, where a mistake here would actually show up.
        MockClockFixture                              clockFixture;
        MockInputHandler                              dummyInput;
        SunLight :: Input :: SDL :: SDLGamepadHaptics haptics( dummyInput );

        dummyInput.availableGamepads.insert( 0 );
        dummyInput.availableGamepads.insert( 1 );

        haptics.SetVibration( 0, 1.0f, 1.0f, 0.1f );
        haptics.SetVibration( 1, 1.0f, 1.0f, 0.5f );

        clockFixture.clock.Advance( 150 );   // only pad 0's own deadline has passed
        haptics.Update();

        clockFixture.clock.Advance( 500 );   // now pad 1's own deadline has passed too
        haptics.Update();

        CHECK( true );
    }
}
