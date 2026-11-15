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

#ifndef __RAYLIBGAMEPADINPUT_H__
#define __RAYLIBGAMEPADINPUT_H__

#include "input/igamepadinput.h"


namespace SunLight  {
    namespace Input  {
        namespace RayLib  {

            /**
             * @brief Gamepads read through raylib, which reads them through GLFW - sunlight's only
             * gamepad backend before I25 of the master plan, kept as DEFAULT_GAMEPAD_ENGINE 2 and as
             * the fallback when SDL's game-controller subsystem cannot start. raylib reads the pads
             * itself, in EndDrawing (PollInputEvents), so Update() does nothing; it sees at most
             * raylib's MAX_GAMEPADS (4) pads.
             */
            class RayLibGamepadInput : public SunLight :: Input :: IGamepadInput  {

                public:

                void Update( void ) override;

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

#endif /* __RAYLIBGAMEPADINPUT_H__ */
