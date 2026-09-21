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

#include "backends/null/nullinputhandler.h"

// Highest keyboard code raylib tracks (its own MAX_KEYBOARD_KEYS, 512) - a
// key outside (0, this) is not a valid key, so it is never "up" either.
#define __MAX_KEYBOARD_KEYS   512

namespace SunLight  {
    namespace Input  {
        namespace Null  {

            /**
             * @brief Nothing is ever pressed.
             */
            SunLight :: Input :: KeyboardKey NullInputHandler :: GetKeyPressed( void )  {

                return SunLight :: Input :: KEY_NULL;
            }

            /**
             * @brief No key is ever down.
             */
            bool NullInputHandler :: IsKeyDown( SunLight :: Input :: KeyboardKey )  {

                return false;
            }

            /**
             * @brief Every valid key is up (nothing is ever down) - same
             * answer raylib gives with no key held; KEY_NULL and out-of-range
             * codes aren't real keys, so they're not "up" either.
             */
            bool NullInputHandler :: IsKeyUp( SunLight :: Input :: KeyboardKey key )  {

                return ( ( int ) key > 0 ) && ( ( int ) key < __MAX_KEYBOARD_KEYS );
            }

            /**
             * @brief No key is ever released (it would first have to be down).
             */
            bool NullInputHandler :: IsKeyReleased( SunLight :: Input :: KeyboardKey )  {

                return false;
            }

            /**
             * @brief Never nullptr, always empty.
             */
            const char* NullInputHandler :: GetGamepadName( int )  {

                return "";
            }

            /**
             * @brief No gamepad is ever connected.
             */
            bool NullInputHandler :: IsGamepadAvailable( int )  {

                return false;
            }

            /**
             * @brief Never down (there's no gamepad).
             */
            bool NullInputHandler :: IsGamepadButtonDown( int, SunLight :: Input :: GamepadButton )  {

                return false;
            }

            /**
             * @brief False, like raylib for an absent gamepad (its "up"
             * answer requires the gamepad to be ready).
             */
            bool NullInputHandler :: IsGamepadButtonUp( int, SunLight :: Input :: GamepadButton )  {

                return false;
            }

            /**
             * @brief No button was ever pressed - the "unknown" value.
             */
            int NullInputHandler :: GetGamepadButtonPressed( void )  {

                return ( int ) SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN;
            }

            /**
             * @brief No gamepad, so no axes.
             */
            int NullInputHandler :: GetGamepadAxisCount( int )  {

                return 0;
            }

            /**
             * @brief Always 0.0, for every axis. Deliberately including the
             * two trigger axes: raylib reports their "rest" value of -1.0
             * even with no gamepad connected, but a null handler has no
             * device and no rest position, and a headless run's input
             * polling must see exactly zero for anything to read as
             * "no input".
             */
            float NullInputHandler :: GetGamepadAxisMovement( int, SunLight :: Input :: GamepadAxis )  {

                return 0.0f;
            }
        }
    }
}
