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

#ifndef __MOCK_INPUT_H__
#define __MOCK_INPUT_H__

#include "input/iinputhandler.h"
#include "input/inputhandlerfactory.h"
#include <array>
#include <memory>
#include <set>

/**
 * @brief Test double for SunLight::Input::IInputHandler - no real keyboard/gamepad device
 * involved. Keys/buttons are "down" when present in the matching set; GetKeyPressed() returns
 * a separately scripted value, since a real backend's "pressed" (one-shot, REACTIVE mode) and
 * "down" (continuous, ACTIVE mode) are two different queries, not derived from one another.
 */
class MockInputHandler : public SunLight :: Input :: IInputHandler  {

    public:

    SunLight :: Input :: KeyboardKey  keyPressedResult = SunLight :: Input :: KEY_NULL;
    std :: set<SunLight :: Input :: KeyboardKey>     downKeys;
    std :: set<SunLight :: Input :: GamepadButton>   downButtons;
    std :: array<float, 6>            axisValues { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    bool                               bGamepadAvailableResult = true;

    SunLight :: Input :: KeyboardKey GetKeyPressed( void ) override  {
        return keyPressedResult;
    }

    bool IsKeyDown( SunLight :: Input :: KeyboardKey key ) override  {
        return downKeys.count( key ) > 0;
    }

    bool IsKeyUp( SunLight :: Input :: KeyboardKey key ) override  {
        return downKeys.count( key ) == 0;
    }

    bool IsKeyReleased( SunLight :: Input :: KeyboardKey key ) override  {
        return false;
    }

    const char* GetGamepadName( int nGamePadId ) override  {
        return "Mock Gamepad";
    }

    bool IsGamepadAvailable( int nGamePadId ) override  {
        return bGamepadAvailableResult;
    }

    bool IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button ) override  {
        return downButtons.count( button ) > 0;
    }

    bool IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button ) override  {
        return downButtons.count( button ) == 0;
    }

    int GetGamepadButtonPressed( void ) override  {
        return SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN;
    }

    int GetGamepadAxisCount( int nGamePadId ) override  {
        return ( int ) axisValues.size();
    }

    float GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis ) override  {
        return axisValues[ ( size_t ) axis ];
    }
};

/**
 * @brief RAII fixture that points InputHandlerFactory :: CreateInputHandler() at a freshly
 * created MockInputHandler for the fixture's lifetime, restoring the default (real) backend on
 * destruction. pLastCreated tracks the most recently created mock - unlike MockSoundFixture's
 * own equivalent, this one does not dangle early: whatever owns the IInputHandler (a
 * ControllerManager, in every known caller) keeps it for its own full lifetime, never
 * discarding it the way SoundManager can discard a failed/unloaded ISound.
 */
class MockInputHandlerFixture  {

    public:

    MockInputHandler  *pLastCreated = nullptr;

    MockInputHandlerFixture( void )  {
        SunLight :: Input :: InputHandlerFactory :: SetCreator( [this]( void ) -> std :: unique_ptr<SunLight :: Input :: IInputHandler>  {
            std :: unique_ptr<MockInputHandler>  pMock = std :: make_unique<MockInputHandler>();

            pLastCreated = pMock.get();

            return pMock;
        } );
    }

    ~MockInputHandlerFixture( void )  {
        SunLight :: Input :: InputHandlerFactory :: SetCreator( nullptr );
    }
};

#endif /* __MOCK_INPUT_H__ */
