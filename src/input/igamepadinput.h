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

#ifndef __IGAMEPADINPUT_H__
#define __IGAMEPADINPUT_H__

#include "input/iinputhandler.h"


namespace SunLight  {
    namespace Input  {

        /**
         * @brief Where an input handler reads its gamepads from (I25 of the master plan): the
         * gamepad half of IInputHandler, with the same meaning for every method, plus the
         * per-frame Update(). Split off so that the pads can come from another library than the
         * keyboard and mouse - SDL2 (SDLGamepadInput) while raylib keeps the keyboard and mouse,
         * or raylib/GLFW itself (RayLibGamepadInput). GamepadInputFactory picks one.
         */
        class IGamepadInput  {

            public:

            virtual ~IGamepadInput( void )  {}

            /**
             * @brief Reads the pads' state for the coming frame (see IInputHandler::Update).
             */
            virtual void Update( void ) = 0;

            virtual const char* GetGamepadName( int nGamePadId ) = 0;
            virtual bool IsGamepadAvailable( int nGamePadId ) = 0;
            virtual bool IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button ) = 0;
            virtual bool IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button ) = 0;
            virtual int GetGamepadButtonPressed( void ) = 0;
            virtual int GetGamepadAxisCount( int nGamePadId ) = 0;
            virtual float GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis ) = 0;
        };
    }
}

#endif /* __IGAMEPADINPUT_H__ */
