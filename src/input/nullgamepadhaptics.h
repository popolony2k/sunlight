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

#ifndef __NULLGAMEPADHAPTICS_H__
#define __NULLGAMEPADHAPTICS_H__

#include "input/igamepadhaptics.h"


namespace SunLight  {
    namespace Input  {

        /**
         * @brief GamepadHapticsFactory's own default (F7a of the master plan): no gamepad ever
         * supports vibration, and SetVibration/Stop are harmless no-ops. Unlike
         * backends/null/NullInputHandler (a headless substitute for a real backend, installed
         * only while a RENDERER_BACKEND_NULL renderer is live), this is the *only* existing
         * implementation - every build's permanent default until F7b picks a real backend - so
         * it lives alongside IGamepadHaptics in src/input/, not under backends/null/.
         */
        class NullGamepadHaptics : public IGamepadHaptics  {

            public:

            bool IsSupported( int nGamePadId ) const override;
            void SetVibration( int nGamePadId, float fLeftMotor, float fRightMotor, float fDuration ) override;
            void Stop( int nGamePadId ) override;
            void Update( void ) override;
        };
    }
}

#endif /* __NULLGAMEPADHAPTICS_H__ */
