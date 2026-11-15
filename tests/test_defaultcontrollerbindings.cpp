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
 * F0b of the master plan: DefaultControllerBindings, the opt-in preset replacing
 * TileMapRenderer's old auto-installed default handlers (removed in this same item). Wires
 * exactly the bindings InitalizeDefaultUserInputHandlers used to install, now external.
 */

#include <doctest/doctest.h>
#include "input/controllermanager.h"
#include "input/defaultcontrollerbindings.h"
#include "mock_input.h"
#include "mock_tilemap.h"

using namespace SunLight :: Input;

TEST_SUITE( "input/DefaultControllerBindings" )  {

    TEST_CASE( "Arrow keys are wired to the matching camera move" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        MockTileMap               map;

        DefaultControllerBindings :: Install( map, manager );

        fixture.pLastCreated -> downKeys.insert( KEY_UP );
        manager.Update();
        CHECK( map.nMoveCameraUpCalls == 1 );

        fixture.pLastCreated -> downKeys.clear();
        fixture.pLastCreated -> downKeys.insert( KEY_DOWN );
        manager.Update();
        CHECK( map.nMoveCameraDownCalls == 1 );

        fixture.pLastCreated -> downKeys.clear();
        fixture.pLastCreated -> downKeys.insert( KEY_LEFT );
        manager.Update();
        CHECK( map.nMoveCameraLeftCalls == 1 );

        fixture.pLastCreated -> downKeys.clear();
        fixture.pLastCreated -> downKeys.insert( KEY_RIGHT );
        manager.Update();
        CHECK( map.nMoveCameraRightCalls == 1 );
    }

    TEST_CASE( "PageUp/PageDown/Home/End are wired to zoom/reset" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        MockTileMap               map;

        DefaultControllerBindings :: Install( map, manager );

        fixture.pLastCreated -> downKeys.insert( KEY_PAGE_UP );
        manager.Update();
        CHECK( map.nZoomInCalls == 1 );

        fixture.pLastCreated -> downKeys.clear();
        fixture.pLastCreated -> downKeys.insert( KEY_PAGE_DOWN );
        manager.Update();
        CHECK( map.nZoomOutCalls == 1 );

        fixture.pLastCreated -> downKeys.clear();
        fixture.pLastCreated -> downKeys.insert( KEY_HOME );
        manager.Update();
        CHECK( map.nResetZoomCalls == 1 );

        fixture.pLastCreated -> downKeys.clear();
        fixture.pLastCreated -> downKeys.insert( KEY_END );
        manager.Update();
        CHECK( map.nResetCameraCalls == 1 );
    }

    TEST_CASE( "A gamepad's DPad and right-face buttons are wired the same way as the keyboard" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager        manager;
        MockTileMap               map;

        DefaultControllerBindings :: Install( map, manager );
        manager.AddGamePad( 1 );

        fixture.pLastCreated -> downButtons.insert( GAMEPAD_BUTTON_LEFT_FACE_UP );
        manager.Update();
        CHECK( map.nMoveCameraUpCalls == 1 );

        fixture.pLastCreated -> downButtons.clear();
        fixture.pLastCreated -> downButtons.insert( GAMEPAD_BUTTON_RIGHT_FACE_UP );
        manager.Update();
        CHECK( map.nZoomInCalls == 1 );

        fixture.pLastCreated -> downButtons.clear();
        fixture.pLastCreated -> downButtons.insert( GAMEPAD_BUTTON_RIGHT_FACE_LEFT );
        manager.Update();
        CHECK( map.nResetZoomCalls == 1 );

        fixture.pLastCreated -> downButtons.clear();
        fixture.pLastCreated -> downButtons.insert( GAMEPAD_BUTTON_RIGHT_FACE_RIGHT );
        manager.Update();
        CHECK( map.nResetCameraCalls == 1 );
    }
}
