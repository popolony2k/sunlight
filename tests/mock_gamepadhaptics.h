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

#ifndef __MOCK_GAMEPADHAPTICS_H__
#define __MOCK_GAMEPADHAPTICS_H__

#include "input/igamepadhaptics.h"
#include "input/gamepadhapticsfactory.h"
#include <memory>
#include <vector>

/**
 * @brief Test double for SunLight::Input::IGamepadHaptics (F7a of the master plan) - no real
 * vibrating device involved. Records every SetVibration/Stop call with its exact arguments, so
 * a test can assert a game's haptics calls reached it unchanged.
 */
class MockGamepadHaptics : public SunLight :: Input :: IGamepadHaptics  {

    public:

    struct stVibrationCall  {
        int    nGamePadId;
        float  fLeftMotor;
        float  fRightMotor;
        float  fDuration;
    };

    bool                               bSupportedResult = true;
    std :: vector<stVibrationCall>     vibrationCalls;
    std :: vector<int>                 stopCalls;
    int                                 nUpdateCalls = 0;

    bool IsSupported( int nGamePadId ) const override  {
        return bSupportedResult;
    }

    void SetVibration( int nGamePadId, float fLeftMotor, float fRightMotor, float fDuration ) override  {
        vibrationCalls.push_back( { nGamePadId, fLeftMotor, fRightMotor, fDuration } );
    }

    void Stop( int nGamePadId ) override  {
        stopCalls.push_back( nGamePadId );
    }

    void Update( void ) override  {
        nUpdateCalls++;
    }
};

/**
 * @brief RAII fixture that points GamepadHapticsFactory::CreateGamepadHaptics() at a freshly
 * created MockGamepadHaptics for the fixture's lifetime, restoring the default
 * (NullGamepadHaptics) on destruction.
 */
class MockGamepadHapticsFixture  {

    public:

    MockGamepadHaptics  *pLastCreated = nullptr;

    MockGamepadHapticsFixture( void )  {
        SunLight :: Input :: GamepadHapticsFactory :: SetCreator( [this]( void ) -> std :: unique_ptr<SunLight :: Input :: IGamepadHaptics>  {
            std :: unique_ptr<MockGamepadHaptics>  pMock = std :: make_unique<MockGamepadHaptics>();

            pLastCreated = pMock.get();

            return pMock;
        } );
    }

    ~MockGamepadHapticsFixture( void )  {
        SunLight :: Input :: GamepadHapticsFactory :: SetCreator( nullptr );
    }
};

#endif /* __MOCK_GAMEPADHAPTICS_H__ */
