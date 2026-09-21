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

#ifndef __NULLINPUTHANDLER_H__
#define __NULLINPUTHANDLER_H__

#include "input/iinputhandler.h"

namespace SunLight  {
    namespace Input  {
        namespace Null  {

            /**
             * @brief Input handler for windowless runs: nothing is ever
             * pressed, no gamepad is ever connected. Every query answers
             * the neutral value, and none of them can fail or return
             * nullptr - a script polling every input every frame just sees
             * silence. Mirrors what the raylib handler reports with no key
             * held and no gamepad present, with one deliberate difference
             * (see GetGamepadAxisMovement).
             */
            class NullInputHandler : public SunLight :: Input :: IInputHandler  {

                public:

                SunLight :: Input :: KeyboardKey GetKeyPressed( void ) override;
                bool IsKeyDown( SunLight :: Input :: KeyboardKey key ) override;
                bool IsKeyUp( SunLight :: Input :: KeyboardKey key ) override;
                bool IsKeyReleased( SunLight :: Input :: KeyboardKey key ) override;

                const char* GetGamepadName( int nGamePadId ) override;
                bool IsGamepadAvailable( int nGamePadId ) override;
                bool IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button ) override;
                bool IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button ) override;
                int GetGamepadButtonPressed( void ) override;
                int GetGamepadAxisCount( int nGamePadId ) override;
                float GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis ) override;
            };
        }
    }
}
#endif  /* __NULLINPUTHANDLER_H__ */
