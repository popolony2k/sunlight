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

#include "backends/sdl/sdlcontrollerapi.h"
#include <SDL.h>

namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            SDLControllerApi :: SDLControllerApi( void )  {

                ::SDL_InitSubSystem( SDL_INIT_GAMECONTROLLER );
            }

            SDLControllerApi :: ~SDLControllerApi( void )  {

                ::SDL_QuitSubSystem( SDL_INIT_GAMECONTROLLER );
            }

            int SDLControllerApi :: NumJoysticks( void )  {

                return ::SDL_NumJoysticks();
            }

            bool SDLControllerApi :: IsGameController( int nIndex )  {

                return ::SDL_IsGameController( nIndex ) == SDL_TRUE;
            }

            const char* SDLControllerApi :: NameForIndex( int nIndex )  {

                return ::SDL_GameControllerNameForIndex( nIndex );
            }

            int32_t SDLControllerApi :: InstanceIdForIndex( int nIndex )  {

                return ::SDL_JoystickGetDeviceInstanceID( nIndex );
            }

            ISDLControllerApi :: ControllerHandle SDLControllerApi :: Open( int nIndex )  {

                return ::SDL_GameControllerOpen( nIndex );
            }

            void SDLControllerApi :: Close( ControllerHandle hController )  {

                ::SDL_GameControllerClose( static_cast<SDL_GameController*>( hController ) );
            }

            bool SDLControllerApi :: IsAttached( ControllerHandle hController )  {

                return ::SDL_GameControllerGetAttached( static_cast<SDL_GameController*>( hController ) ) == SDL_TRUE;
            }

            int32_t SDLControllerApi :: InstanceId( ControllerHandle hController )  {

                return ::SDL_JoystickInstanceID( ::SDL_GameControllerGetJoystick( static_cast<SDL_GameController*>( hController ) ) );
            }

            bool SDLControllerApi :: HasRumble( ControllerHandle hController )  {

                return ::SDL_GameControllerHasRumble( static_cast<SDL_GameController*>( hController ) ) == SDL_TRUE;
            }

            void SDLControllerApi :: Rumble( ControllerHandle hController, uint16_t nLowFrequency, uint16_t nHighFrequency, uint32_t nDurationMs )  {

                ::SDL_GameControllerRumble( static_cast<SDL_GameController*>( hController ), nLowFrequency, nHighFrequency, nDurationMs );
            }
        }
    }
}
