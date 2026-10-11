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
 * F0a of the master plan: ControllerManager, extracted out of TileMapRenderer's own
 * HandleUserInput/HandleKeyEvent. These tests pin today's actual dispatch behavior
 * (confirmed against the pre-extraction source, not assumed), including the quirks later
 * phase items are meant to fix (F5's duplicate-pad dedup) - this item is a pure extraction
 * with no behavior change, so its own tests must match what the code already does, not what
 * it should eventually do.
 */

#include <doctest/doctest.h>
#include "input/controllermanager.h"
#include "mock_input.h"
#include <string>
#include <utility>
#include <vector>

using namespace SunLight :: Input;

TEST_SUITE( "input/ControllerManager" )  {

    TEST_CASE( "ACTIVE mode fires a key handler every frame while the key is held" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.SetInputDispatchMode( INPUT_DISPATCH_MODE_ACTIVE );
        manager.SetUserKeyEventHandler( KEY_W, [&]( ControllerType, int ) { nFireCount++; } );

        fixture.pLastCreated -> downKeys.insert( KEY_W );

        manager.Update();
        manager.Update();
        manager.Update();

        CHECK( nFireCount == 3 );
    }

    TEST_CASE( "ACTIVE mode does not fire a key handler once the key is released" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.SetUserKeyEventHandler( KEY_W, [&]( ControllerType, int ) { nFireCount++; } );

        fixture.pLastCreated -> downKeys.insert( KEY_W );
        manager.Update();
        fixture.pLastCreated -> downKeys.erase( KEY_W );
        manager.Update();

        CHECK( nFireCount == 1 );
    }

    TEST_CASE( "REACTIVE mode dispatches to the handler matching GetKeyPressed()'s current value" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nWFireCount = 0;
        int                      nAFireCount = 0;

        manager.SetInputDispatchMode( INPUT_DISPATCH_MODE_REACTIVE );
        manager.SetUserKeyEventHandler( KEY_W, [&]( ControllerType, int ) { nWFireCount++; } );
        manager.SetUserKeyEventHandler( KEY_A, [&]( ControllerType, int ) { nAFireCount++; } );

        fixture.pLastCreated -> keyPressedResult = KEY_W;
        manager.Update();

        CHECK( nWFireCount == 1 );
        CHECK( nAFireCount == 0 );
    }

    TEST_CASE( "KEY_NULL registers the no-input fallback, fired only when nothing else handled that frame" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nKeyFireCount      = 0;
        int                      nFallbackFireCount = 0;

        manager.SetUserKeyEventHandler( KEY_W, [&]( ControllerType, int ) { nKeyFireCount++; } );
        manager.SetUserKeyEventHandler( KEY_NULL, [&]( ControllerType, int ) { nFallbackFireCount++; } );

        // Nothing down: the fallback fires.
        manager.Update();
        CHECK( nKeyFireCount == 0 );
        CHECK( nFallbackFireCount == 1 );

        // KEY_W down: the real handler fires, not the fallback.
        fixture.pLastCreated -> downKeys.insert( KEY_W );
        manager.Update();
        CHECK( nKeyFireCount == 1 );
        CHECK( nFallbackFireCount == 1 );
    }

    TEST_CASE( "A gamepad analog stick beyond the deadzone dispatches the matching DPad-equivalent handler" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.AddGamePad( 1 );
        manager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_RIGHT, [&]( ControllerType, int ) { nFireCount++; } );

        fixture.pLastCreated -> axisValues[ GAMEPAD_AXIS_LEFT_X ] = 0.5f;
        manager.Update();

        CHECK( nFireCount == 1 );
    }

    TEST_CASE( "A gamepad analog stick within the deadzone does not dispatch" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.AddGamePad( 1 );
        manager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_RIGHT, [&]( ControllerType, int ) { nFireCount++; } );

        fixture.pLastCreated -> axisValues[ GAMEPAD_AXIS_LEFT_X ] = 0.05f;
        manager.Update();

        CHECK( nFireCount == 0 );
    }

    TEST_CASE( "A duplicate AddGamePad call double-fires - today's actual behavior, not yet deduplicated (F5)" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.AddGamePad( 1 );
        manager.AddGamePad( 1 );
        manager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_UP, [&]( ControllerType, int ) { nFireCount++; } );

        fixture.pLastCreated -> downButtons.insert( GAMEPAD_BUTTON_LEFT_FACE_UP );
        manager.Update();

        CHECK( nFireCount == 2 );
    }

    TEST_CASE( "Gamepad activity suppresses keyboard dispatch that frame, even with no gamepad handler registered" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nKeyFireCount = 0;

        manager.AddGamePad( 1 );
        manager.SetUserKeyEventHandler( KEY_W, [&]( ControllerType, int ) { nKeyFireCount++; } );

        fixture.pLastCreated -> downKeys.insert( KEY_W );
        fixture.pLastCreated -> axisValues[ GAMEPAD_AXIS_LEFT_X ] = 0.5f;   // no handler registered for this
        manager.Update();

        CHECK( nKeyFireCount == 0 );
    }

    TEST_CASE( "GetInputHandler returns the same handler instance used internally" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;

        CHECK( &manager.GetInputHandler() == fixture.pLastCreated );
    }

    TEST_CASE( "SetInputDispatchMode/GetInputDispatchMode round-trip, default is ACTIVE" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;

        CHECK( manager.GetInputDispatchMode() == INPUT_DISPATCH_MODE_ACTIVE );

        manager.SetInputDispatchMode( INPUT_DISPATCH_MODE_REACTIVE );
        CHECK( manager.GetInputDispatchMode() == INPUT_DISPATCH_MODE_REACTIVE );
    }

    // F1 of the master plan: device list and hot-plug events.

    TEST_CASE( "GetConnectedGamepads lists nothing by default" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;

        CHECK( manager.GetConnectedGamepads().empty() );
    }

    TEST_CASE( "GetConnectedGamepads lists every available id with its name, in id order" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;

        fixture.pLastCreated -> availableGamepads = { 5, 2 };

        std :: vector<stGamepadInfo>  connected = manager.GetConnectedGamepads();

        REQUIRE( connected.size() == 2 );
        CHECK( connected[ 0 ].nGamePadId == 2 );
        CHECK( connected[ 1 ].nGamePadId == 5 );
        CHECK( std :: string( connected[ 0 ].szName ) == "Mock Gamepad" );
    }

    TEST_CASE( "A scripted connect is reported exactly once through the hot-plug handler" )  {

        MockInputHandlerFixture               fixture;
        ControllerManager                     manager;
        std :: vector<std :: pair<int, bool>>  events;

        manager.SetGamepadConnectionHandler( [&]( int nGamePadId, bool bConnected ) {
            events.push_back( { nGamePadId, bConnected } );
        } );

        manager.Update();
        CHECK( events.empty() );

        fixture.pLastCreated -> availableGamepads.insert( 3 );
        manager.Update();

        REQUIRE( events.size() == 1 );
        CHECK( events[ 0 ].first == 3 );
        CHECK( events[ 0 ].second == true );

        // Still connected on the next frame: not reported again.
        manager.Update();
        CHECK( events.size() == 1 );
    }

    TEST_CASE( "A scripted disconnect is reported exactly once through the hot-plug handler" )  {

        MockInputHandlerFixture               fixture;
        ControllerManager                     manager;
        std :: vector<std :: pair<int, bool>>  events;

        fixture.pLastCreated -> availableGamepads.insert( 3 );
        manager.Update();   // establish id 3 as already-connected baseline

        manager.SetGamepadConnectionHandler( [&]( int nGamePadId, bool bConnected ) {
            events.push_back( { nGamePadId, bConnected } );
        } );

        fixture.pLastCreated -> availableGamepads.erase( 3 );
        manager.Update();

        REQUIRE( events.size() == 1 );
        CHECK( events[ 0 ].first == 3 );
        CHECK( events[ 0 ].second == false );

        manager.Update();
        CHECK( events.size() == 1 );
    }

    // F4 of the master plan: configurable dead zones and thresholds.

    TEST_CASE( "GetAxisDeadzone defaults match today's own value, per pad and per axis" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;

        CHECK( manager.GetAxisDeadzone( 0, GAMEPAD_AXIS_LEFT_X ) == DEFAULT_AXIS_DEADZONE );
        CHECK( manager.GetAxisDeadzone( 0, GAMEPAD_AXIS_LEFT_Y ) == DEFAULT_AXIS_DEADZONE );
        CHECK( manager.GetAxisDeadzone( 1, GAMEPAD_AXIS_RIGHT_X ) == DEFAULT_AXIS_DEADZONE );
        CHECK( manager.GetAxisDeadzone( 1, GAMEPAD_AXIS_RIGHT_Y ) == DEFAULT_AXIS_DEADZONE );
    }

    TEST_CASE( "SetAxisDeadzone configures exactly one (pad, axis) pair, leaving every other default" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;

        manager.SetAxisDeadzone( 1, GAMEPAD_AXIS_LEFT_X, 0.5f );

        CHECK( manager.GetAxisDeadzone( 1, GAMEPAD_AXIS_LEFT_X ) == 0.5f );
        CHECK( manager.GetAxisDeadzone( 1, GAMEPAD_AXIS_LEFT_Y ) == DEFAULT_AXIS_DEADZONE );
        CHECK( manager.GetAxisDeadzone( 0, GAMEPAD_AXIS_LEFT_X ) == DEFAULT_AXIS_DEADZONE );
    }

    TEST_CASE( "A value exactly at the deadzone boundary behaves as specified: it is a real value, not zeroed" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.AddGamePad( 1 );
        manager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_RIGHT, [&]( ControllerType, int ) { nFireCount++; } );

        // Exactly at the default deadzone's own boundary - not strictly inside it.
        fixture.pLastCreated -> axisValues[ GAMEPAD_AXIS_LEFT_X ] = DEFAULT_AXIS_DEADZONE;
        manager.Update();

        CHECK( nFireCount == 1 );
    }

    TEST_CASE( "A configured left-stick Y deadzone is the one consulted for the left stick's Y axis "
               "(regression: it used to read the right stick's Y deadzone instead)" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        int                      nFireCount = 0;

        manager.AddGamePad( 1 );
        manager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_DOWN, [&]( ControllerType, int ) { nFireCount++; } );

        // Left Y has a wide deadzone; right Y has none. A left-stick Y position inside the
        // left deadzone but outside the (irrelevant) right one must not dispatch.
        manager.SetAxisDeadzone( 1, GAMEPAD_AXIS_LEFT_Y, 0.5f );
        manager.SetAxisDeadzone( 1, GAMEPAD_AXIS_RIGHT_Y, 0.0f );

        fixture.pLastCreated -> axisValues[ GAMEPAD_AXIS_LEFT_Y ] = 0.3f;
        manager.Update();

        CHECK( nFireCount == 0 );
    }
}
