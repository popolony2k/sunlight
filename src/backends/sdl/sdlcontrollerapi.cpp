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


#define __SDL_HINT_OFF                 "0"
#define __SDL_INIT_SUCCESS             0

// ISDLControllerApi's Button and Axis are SDL's own values (see their doc comments): checked here,
// the one place both are visible, so a cast between them is always the identity.
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_A == ( int ) SDL_CONTROLLER_BUTTON_A, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_B == ( int ) SDL_CONTROLLER_BUTTON_B, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_X == ( int ) SDL_CONTROLLER_BUTTON_X, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_Y == ( int ) SDL_CONTROLLER_BUTTON_Y, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_BACK == ( int ) SDL_CONTROLLER_BUTTON_BACK, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_GUIDE == ( int ) SDL_CONTROLLER_BUTTON_GUIDE, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_START == ( int ) SDL_CONTROLLER_BUTTON_START, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_LEFTSTICK == ( int ) SDL_CONTROLLER_BUTTON_LEFTSTICK, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_RIGHTSTICK == ( int ) SDL_CONTROLLER_BUTTON_RIGHTSTICK, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_LEFTSHOULDER == ( int ) SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_RIGHTSHOULDER == ( int ) SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_DPAD_UP == ( int ) SDL_CONTROLLER_BUTTON_DPAD_UP, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_DPAD_DOWN == ( int ) SDL_CONTROLLER_BUTTON_DPAD_DOWN, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_DPAD_LEFT == ( int ) SDL_CONTROLLER_BUTTON_DPAD_LEFT, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: BUTTON_DPAD_RIGHT == ( int ) SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "SDL button mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: AXIS_LEFTX == ( int ) SDL_CONTROLLER_AXIS_LEFTX, "SDL axis mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: AXIS_LEFTY == ( int ) SDL_CONTROLLER_AXIS_LEFTY, "SDL axis mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: AXIS_RIGHTX == ( int ) SDL_CONTROLLER_AXIS_RIGHTX, "SDL axis mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: AXIS_RIGHTY == ( int ) SDL_CONTROLLER_AXIS_RIGHTY, "SDL axis mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: AXIS_TRIGGERLEFT == ( int ) SDL_CONTROLLER_AXIS_TRIGGERLEFT, "SDL axis mismatch" );
static_assert( ( int ) SunLight :: Input :: SDL :: ISDLControllerApi :: AXIS_TRIGGERRIGHT == ( int ) SDL_CONTROLLER_AXIS_TRIGGERRIGHT, "SDL axis mismatch" );

namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief Starts SDL's game-controller subsystem (by reference count, see the class's
             * doc comment). Before that, SDL_GAMECONTROLLER_USE_BUTTON_LABELS is turned off (I25 of
             * the master plan): SDL2 reports a Nintendo pad's face buttons by their printed label by
             * default, so its A (on the right) would be SDL's A, the bottom one on an Xbox pad;
             * with the hint off they are reported by position, as GLFW does and as sunlight's
             * GAMEPAD_BUTTON_RIGHT_FACE_* say - read in SDL_hidapi_switch.c (RemapButton; pads with
             * no diamond layout, such as the NES and N64 ones, always use labels: AlwaysUsesLabels).
             * The hint is process-wide, and the Switch driver follows a later change too (it
             * registers a hint callback), so it is set before any controller is opened.
             */
            SDLControllerApi :: SDLControllerApi( void )  {

                ::SDL_SetHint( SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, __SDL_HINT_OFF );
                m_bReady = ( ::SDL_InitSubSystem( SDL_INIT_GAMECONTROLLER ) == __SDL_INIT_SUCCESS );
            }

            /**
             * @brief Releases this instance's reference on the subsystem, if it got one.
             */
            SDLControllerApi :: ~SDLControllerApi( void )  {

                if( m_bReady )
                    ::SDL_QuitSubSystem( SDL_INIT_GAMECONTROLLER );
            }

            /**
             * @brief Whether the constructor's SDL_InitSubSystem succeeded.
             */
            bool SDLControllerApi :: IsReady( void )  {

                return m_bReady;
            }

            /**
             * @brief SDL_GameControllerUpdate - which is SDL_JoystickUpdate (SDL_gamecontroller.c):
             * every open pad's driver reads its state, and every joystick driver's Detect() runs,
             * so pads plugged in or removed are found with no video subsystem and no event loop
             * (SDL_joystick.c, SDL_JoystickUpdate).
             */
            void SDLControllerApi :: Update( void )  {

                if( m_bReady )
                    ::SDL_GameControllerUpdate();
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

            /**
             * @brief SDL_GameControllerGetButton, as of the last Update.
             */
            bool SDLControllerApi :: GetButton( ControllerHandle hController, Button button )  {

                return ::SDL_GameControllerGetButton( static_cast<SDL_GameController*>( hController ),
                                                      static_cast<SDL_GameControllerButton>( button ) ) != 0;
            }

            /**
             * @brief SDL_GameControllerGetAxis, as of the last Update.
             */
            int16_t SDLControllerApi :: GetAxis( ControllerHandle hController, Axis axis )  {

                return ::SDL_GameControllerGetAxis( static_cast<SDL_GameController*>( hController ),
                                                    static_cast<SDL_GameControllerAxis>( axis ) );
            }
        }
    }
}
