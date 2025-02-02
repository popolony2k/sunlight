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

#ifndef __RAYLIBINPUTHANDLER_H__
#define __RAYLIBINPUTHANDLER_H__

#include "iinputhandler.h"
#include "base/object.h"

namespace SunLight {
    namespace Input  {
        namespace RayLib  {
            /**
             * @brief Keyboard input handler implementation based on 
             * RayLib library.
             */
            class RayLibInputHandler : public SunLight :: Base :: Object, public IInputHandler  {

                public:

                RayLibInputHandler( void );
                virtual ~RayLibInputHandler( void );

                // Keyboard handling

                SunLight :: Input :: KeyboardKey GetKeyPressed( void );
                bool IsKeyDown( SunLight :: Input :: KeyboardKey key );
                bool IsKeyUp( SunLight :: Input :: KeyboardKey key );
                bool IsKeyReleased( SunLight :: Input :: KeyboardKey key );

                // Gamepad handling

                const char* GetGamepadName( int nGamePadId );
                bool IsGamepadAvailable( int nGamePadId );
                bool IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button );
                bool IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button );
                int GetGamepadButtonPressed( void );
                int GetGamepadAxisCount( int nGamePadId );
                float GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis );
                void Calibrate( int nGamePadId, float fAxisX, float fAxisY );
                stGamePadCalibration& GetGamePadCalibration( int nGamePadId );
            };
        }
    }
}

#endif  /* __RAYLIBINPUTHANDLER_H__ */
