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

#ifndef __ISDLCONTROLLERAPI_H__
#define __ISDLCONTROLLERAPI_H__

#include <cstdint>


namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief The SDL2 GameController calls SDLGamepadHaptics makes, behind an interface of
             * its own so its logic (which controller it opens, when it reuses or reopens one) can
             * be tested with a fake and no real device (SDLControllerApi is the real one). Internal
             * to the SDL backend: no SDL type crosses it, a controller is an opaque handle.
             */
            class ISDLControllerApi  {

                public:

                typedef void*  ControllerHandle;    // an SDL_GameController*, opaque outside the real API

                virtual ~ISDLControllerApi( void )  {}

                // The device list (SDL_NumJoysticks, SDL_IsGameController, SDL_GameControllerNameForIndex,
                // SDL_JoystickGetDeviceInstanceID).
                virtual int NumJoysticks( void ) = 0;
                virtual bool IsGameController( int nIndex ) = 0;
                virtual const char* NameForIndex( int nIndex ) = 0;
                virtual int32_t InstanceIdForIndex( int nIndex ) = 0;

                // One opened controller (SDL_GameControllerOpen/Close, SDL_GameControllerGetAttached,
                // SDL_JoystickInstanceID, SDL_GameControllerHasRumble, SDL_GameControllerRumble).
                virtual ControllerHandle Open( int nIndex ) = 0;
                virtual void Close( ControllerHandle hController ) = 0;
                virtual bool IsAttached( ControllerHandle hController ) = 0;
                virtual int32_t InstanceId( ControllerHandle hController ) = 0;
                virtual bool HasRumble( ControllerHandle hController ) = 0;
                virtual void Rumble( ControllerHandle hController, uint16_t nLowFrequency, uint16_t nHighFrequency, uint32_t nDurationMs ) = 0;
            };
        }
    }
}

#endif /* __ISDLCONTROLLERAPI_H__ */
