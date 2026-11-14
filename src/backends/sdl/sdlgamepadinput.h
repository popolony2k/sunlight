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

#ifndef __SDLGAMEPADINPUT_H__
#define __SDLGAMEPADINPUT_H__

#include "input/igamepadinput.h"
#include "backends/sdl/sdlgamepaddevices.h"
#include <memory>


namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief Gamepads read through SDL2's GameController API (I25 of the master plan), on
             * the shared SDLGamepadDevices - so a pad id here is the same device SDLGamepadHaptics
             * rumbles. SDL's drivers do what GLFW's cannot: a Switch-mode pad over USB needs a
             * handshake written to it before it reports anything (SDL_hidapi_switch.c, BTrySetupUSB;
             * GLFW's macOS joystick code never writes to a device), and over Bluetooth it is moved to
             * its full report mode.
             *
             * Every answer means what raylib's does (rcore.c, rcore_desktop_glfw.c), so a game sees
             * no difference in kind: buttons by position on an Xbox-style layout, sticks in -1..1,
             * triggers in -1..1 resting at -1, the second triggers pressed past __TRIGGER_THRESHOLD,
             * and GetGamepadButtonPressed the last button held at the last Update.
             */
            class SDLGamepadInput : public SunLight :: Input :: IGamepadInput  {

                std :: shared_ptr<SDLGamepadDevices>  m_pDevices;
                int                                   m_nLastButtonPressed;

                public:

                // Updates once, so the pads already plugged in are there from the first frame, as
                // raylib's are once its window is open.
                explicit SDLGamepadInput( std :: shared_ptr<SDLGamepadDevices> pDevices );
                ~SDLGamepadInput( void ) override;

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

#endif /* __SDLGAMEPADINPUT_H__ */
