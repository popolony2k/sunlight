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
 * F2 of the master plan: PlayerSlotManager maps a game's own player slot numbers to gamepad
 * device ids, independent of any live IInputHandler/IControllerManager connectivity state.
 */

#include <doctest/doctest.h>
#include "input/playerslotmanager.h"
#include "mock_input.h"
#include "input/controllermanager.h"

using namespace SunLight :: Input;

TEST_SUITE( "input/PlayerSlotManager" )  {

    TEST_CASE( "A claimed slot starts unclaimed and reports NO_SLOT_ASSOCIATION both ways" )  {

        PlayerSlotManager  slots;

        CHECK( slots.GetGamepadForSlot( 0 ) == NO_SLOT_ASSOCIATION );
        CHECK( slots.GetSlotForGamepad( 0 ) == NO_SLOT_ASSOCIATION );
    }

    TEST_CASE( "ClaimSlot succeeds for a free gamepad and round-trips both lookups" )  {

        PlayerSlotManager  slots;

        CHECK( slots.ClaimSlot( 0, 2 ) == true );
        CHECK( slots.GetGamepadForSlot( 0 ) == 2 );
        CHECK( slots.GetSlotForGamepad( 2 ) == 0 );
    }

    TEST_CASE( "Claiming an already claimed pad is refused" )  {

        PlayerSlotManager  slots;

        slots.ClaimSlot( 0, 2 );

        CHECK( slots.ClaimSlot( 1, 2 ) == false );
        CHECK( slots.GetGamepadForSlot( 1 ) == NO_SLOT_ASSOCIATION );
        CHECK( slots.GetSlotForGamepad( 2 ) == 0 );
    }

    TEST_CASE( "Releasing a slot frees the pad for another slot to claim" )  {

        PlayerSlotManager  slots;

        slots.ClaimSlot( 0, 2 );
        slots.ReleaseSlot( 0 );

        CHECK( slots.GetGamepadForSlot( 0 ) == NO_SLOT_ASSOCIATION );
        CHECK( slots.GetSlotForGamepad( 2 ) == NO_SLOT_ASSOCIATION );
        CHECK( slots.ClaimSlot( 1, 2 ) == true );
        CHECK( slots.GetSlotForGamepad( 2 ) == 1 );
    }

    TEST_CASE( "Releasing an unclaimed slot is a harmless no-op" )  {

        PlayerSlotManager  slots;

        slots.ReleaseSlot( 0 );

        CHECK( slots.GetGamepadForSlot( 0 ) == NO_SLOT_ASSOCIATION );
    }

    TEST_CASE( "Re-claiming the same pad for the same slot is an idempotent success" )  {

        PlayerSlotManager  slots;

        slots.ClaimSlot( 0, 2 );

        CHECK( slots.ClaimSlot( 0, 2 ) == true );
        CHECK( slots.GetGamepadForSlot( 0 ) == 2 );
    }

    TEST_CASE( "Claiming a different pad for a slot that already holds one replaces it, freeing the old one" )  {

        PlayerSlotManager  slots;

        slots.ClaimSlot( 0, 2 );

        CHECK( slots.ClaimSlot( 0, 5 ) == true );
        CHECK( slots.GetGamepadForSlot( 0 ) == 5 );
        CHECK( slots.GetSlotForGamepad( 2 ) == NO_SLOT_ASSOCIATION );
        CHECK( slots.GetSlotForGamepad( 5 ) == 0 );
    }

    TEST_CASE( "Unplug and replug keeps the slot: a claim is untouched by the gamepad's own connectivity" )  {

        MockInputHandlerFixture  fixture;
        ControllerManager         controller;
        PlayerSlotManager         slots;

        fixture.pLastCreated -> availableGamepads.insert( 2 );
        controller.Update();
        slots.ClaimSlot( 0, 2 );

        // Unplug: the device disappears, but nothing was ever told to release the slot.
        fixture.pLastCreated -> availableGamepads.erase( 2 );
        controller.Update();
        CHECK( slots.GetGamepadForSlot( 0 ) == 2 );

        // Replug: the same id comes back - the slot was never lost in the first place.
        fixture.pLastCreated -> availableGamepads.insert( 2 );
        controller.Update();
        CHECK( slots.GetGamepadForSlot( 0 ) == 2 );
        CHECK( slots.GetSlotForGamepad( 2 ) == 0 );
    }
}
