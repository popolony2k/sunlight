/*
 * raylibinputhandler.h
 *
 *  Created on: Apr 06, 2022
 *      Author: popolony2k
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
            };
        }
    }
}

#endif  /* __RAYLIBINPUTHANDLER_H__ */
