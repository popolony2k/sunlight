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

#include "backends/raylib/raylibgamepadinput.h"
#include "backends/raylib/raylibcodemap.h"
#include <raylib.h>


#define __PAD_BUTTON_CODE_COUNT        18         // one past the highest button (GAMEPAD_BUTTON_RIGHT_THUMB)
#define __PAD_AXIS_CODE_COUNT          6          // one past the highest axis (GAMEPAD_AXIS_RIGHT_TRIGGER)

namespace  {

    constexpr SunLight :: Input :: RayLib :: CodePair kPadButtonPairs[] = {
        { SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN, ::GAMEPAD_BUTTON_UNKNOWN },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_UP, ::GAMEPAD_BUTTON_LEFT_FACE_UP },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT, ::GAMEPAD_BUTTON_LEFT_FACE_RIGHT },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_DOWN, ::GAMEPAD_BUTTON_LEFT_FACE_DOWN },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_LEFT, ::GAMEPAD_BUTTON_LEFT_FACE_LEFT },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_UP, ::GAMEPAD_BUTTON_RIGHT_FACE_UP },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, ::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN, ::GAMEPAD_BUTTON_RIGHT_FACE_DOWN },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT, ::GAMEPAD_BUTTON_RIGHT_FACE_LEFT },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_1, ::GAMEPAD_BUTTON_LEFT_TRIGGER_1 },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_2, ::GAMEPAD_BUTTON_LEFT_TRIGGER_2 },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1, ::GAMEPAD_BUTTON_RIGHT_TRIGGER_1 },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_2, ::GAMEPAD_BUTTON_RIGHT_TRIGGER_2 },
        { SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_LEFT, ::GAMEPAD_BUTTON_MIDDLE_LEFT },
        { SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE, ::GAMEPAD_BUTTON_MIDDLE },
        { SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_RIGHT, ::GAMEPAD_BUTTON_MIDDLE_RIGHT },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_THUMB, ::GAMEPAD_BUTTON_LEFT_THUMB },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_THUMB, ::GAMEPAD_BUTTON_RIGHT_THUMB },
    };

    constexpr SunLight :: Input :: RayLib :: CodePair kPadAxisPairs[] = {
        { SunLight :: Input :: GAMEPAD_AXIS_LEFT_X, ::GAMEPAD_AXIS_LEFT_X },
        { SunLight :: Input :: GAMEPAD_AXIS_LEFT_Y, ::GAMEPAD_AXIS_LEFT_Y },
        { SunLight :: Input :: GAMEPAD_AXIS_RIGHT_X, ::GAMEPAD_AXIS_RIGHT_X },
        { SunLight :: Input :: GAMEPAD_AXIS_RIGHT_Y, ::GAMEPAD_AXIS_RIGHT_Y },
        { SunLight :: Input :: GAMEPAD_AXIS_LEFT_TRIGGER, ::GAMEPAD_AXIS_LEFT_TRIGGER },
        { SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER, ::GAMEPAD_AXIS_RIGHT_TRIGGER },
    };
    constexpr SunLight :: Input :: RayLib :: CodeMap<__PAD_BUTTON_CODE_COUNT>  kPadButtons = SunLight :: Input :: RayLib :: BuildCodeMap<__PAD_BUTTON_CODE_COUNT>( kPadButtonPairs );
    constexpr SunLight :: Input :: RayLib :: CodeMap<__PAD_AXIS_CODE_COUNT>    kPadAxes    = SunLight :: Input :: RayLib :: BuildCodeMap<__PAD_AXIS_CODE_COUNT>( kPadAxisPairs );
}


namespace SunLight  {
    namespace Input  {
        namespace RayLib  {

            /**
             * @brief Nothing to do: raylib reads the pads in EndDrawing.
             */
            void RayLibGamepadInput :: Update( void )  {

            }

            /**
             * @brief Get the Gamepad Name for requested Game Pad Id;
             * @param nGamePadId The game pad id to check;
             */
            const char* RayLibGamepadInput :: GetGamepadName( int nGamePadId )  {

                return ::GetGamepadName( nGamePadId );
            }

            /**
             * @brief Check if the requested gamepad is available;
             * @param nGamePadId The game pad id to check;
             */
            bool RayLibGamepadInput :: IsGamepadAvailable( int nGamePadId )  {

                return ::IsGamepadAvailable( nGamePadId );
            }

            /**
             * @brief Check if the specified button is in down state;
             * @param nGamePadId The game pad id to check;
             * @param button Button code to check;
             */
            bool RayLibGamepadInput :: IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                return ::IsGamepadButtonDown( nGamePadId, ( ::GamepadButton ) ToRaylib( kPadButtons, button ) );
            }

            /**
             * @brief Check if the specified button is in up state;
             * @param nGamePadId The game pad id to check;
             * @param button Button code to check;
             */
            bool RayLibGamepadInput :: IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                return ::IsGamepadButtonUp( nGamePadId, ( ::GamepadButton ) ToRaylib( kPadButtons, button ) );
            }

            /**
             * @brief The last gamepad button held, as raylib reports it.
             */
            int RayLibGamepadInput :: GetGamepadButtonPressed( void )  {

                return FromRaylib( kPadButtons, ::GetGamepadButtonPressed() );
            }

            /**
             * @brief Get the Gamepad Axis count;
             * @param nGamePadId The game pad id to check;
             */
            int RayLibGamepadInput :: GetGamepadAxisCount( int nGamePadId )  {

                return ::GetGamepadAxisCount( nGamePadId );
            }

            /**
             * @brief Get the Gamepad Axis Movement position
             * @param nGamePadId The game pad id to check;
             * @param axis The axis to check;
             */
            float RayLibGamepadInput :: GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis )  {

                return ::GetGamepadAxisMovement( nGamePadId, ( ::GamepadAxis ) ToRaylib( kPadAxes, axis ) );
            }
        }
    }
}
