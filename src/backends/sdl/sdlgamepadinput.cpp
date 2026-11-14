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

#include "backends/sdl/sdlgamepadinput.h"
#include <algorithm>


#define __AXIS_COUNT                   6          // raylib's: GLFW_GAMEPAD_AXIS_LAST + 1
#define __TRIGGER_THRESHOLD            0.1f       // raylib's second-trigger "pressed" level (rcore_desktop_glfw.c)
#define __AXIS_MAX                     32767.0f   // SDL_JOYSTICK_AXIS_MAX
#define __TRIGGER_SPAN                 2.0f       // a trigger's 0..1 stretched to -1..1
#define __STICK_REST                   0.0f
#define __TRIGGER_REST                 -1.0f
#define __AXIS_MIN_VALUE               -1.0f
#define __AXIS_MAX_VALUE               1.0f
#define __EMPTY_NAME                   ""

namespace  {

    typedef SunLight :: Input :: SDL :: ISDLControllerApi  Api;

    // GetGamepadAxisMovement casts sunlight's axis straight to SDL's: the same order, checked.
    static_assert( ( int ) SunLight :: Input :: GAMEPAD_AXIS_LEFT_X == ( int ) Api :: AXIS_LEFTX, "axis order" );
    static_assert( ( int ) SunLight :: Input :: GAMEPAD_AXIS_LEFT_Y == ( int ) Api :: AXIS_LEFTY, "axis order" );
    static_assert( ( int ) SunLight :: Input :: GAMEPAD_AXIS_RIGHT_X == ( int ) Api :: AXIS_RIGHTX, "axis order" );
    static_assert( ( int ) SunLight :: Input :: GAMEPAD_AXIS_RIGHT_Y == ( int ) Api :: AXIS_RIGHTY, "axis order" );
    static_assert( ( int ) SunLight :: Input :: GAMEPAD_AXIS_LEFT_TRIGGER == ( int ) Api :: AXIS_TRIGGERLEFT, "axis order" );
    static_assert( ( int ) SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER == ( int ) Api :: AXIS_TRIGGERRIGHT, "axis order" );

    // Where one of sunlight's buttons is read from: an SDL button, or (the second triggers) an
    // SDL trigger axis, pressed past __TRIGGER_THRESHOLD - GLFW has no trigger buttons either, and
    // raylib makes them from the axes the same way (rcore_desktop_glfw.c, PollInputEvents).
    struct stButtonSource  {
        SunLight :: Input :: GamepadButton  button;
        bool                                bTriggerAxis;
        int                                 nCode;        // an Api :: Button, or an Api :: Axis
    };

    // Every sunlight button but GAMEPAD_BUTTON_UNKNOWN, as raylib maps GLFW's (which uses SDL's
    // own layout and names: GLFW's gamepad mappings are SDL's database format).
    const stButtonSource  kButtonSources[] = {
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_UP,     false, Api :: BUTTON_DPAD_UP },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT,  false, Api :: BUTTON_DPAD_RIGHT },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_DOWN,   false, Api :: BUTTON_DPAD_DOWN },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_LEFT,   false, Api :: BUTTON_DPAD_LEFT },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_UP,    false, Api :: BUTTON_Y },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, false, Api :: BUTTON_B },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN,  false, Api :: BUTTON_A },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT,  false, Api :: BUTTON_X },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_1,   false, Api :: BUTTON_LEFTSHOULDER },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_2,   true,  Api :: AXIS_TRIGGERLEFT },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1,  false, Api :: BUTTON_RIGHTSHOULDER },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_2,  true,  Api :: AXIS_TRIGGERRIGHT },
        { SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_LEFT,      false, Api :: BUTTON_BACK },
        { SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE,           false, Api :: BUTTON_GUIDE },
        { SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_RIGHT,     false, Api :: BUTTON_START },
        { SunLight :: Input :: GAMEPAD_BUTTON_LEFT_THUMB,       false, Api :: BUTTON_LEFTSTICK },
        { SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_THUMB,      false, Api :: BUTTON_RIGHTSTICK },
    };

    // The buttons GetGamepadButtonPressed can answer, in the order raylib walks GLFW's (GLFW's
    // button order: A, B, X, Y, the bumpers, back, start, guide, the thumbs, then the d-pad up,
    // right, down, left) - the LAST one held wins, as in raylib, which also never reports the
    // second triggers there (rcore_desktop_glfw.c, PollInputEvents).
    const SunLight :: Input :: GamepadButton  kPressedOrder[] = {
        SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
        SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
        SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_UP,
        SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_1,
        SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
        SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_LEFT,
        SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_RIGHT,
        SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE,
        SunLight :: Input :: GAMEPAD_BUTTON_LEFT_THUMB,
        SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_THUMB,
        SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_UP,
        SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
        SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_LEFT,
    };

    // A trigger's SDL value (0..32767) on raylib's scale: -1 released, 1 fully pulled - what GLFW
    // reports (its gamepad axes are -1..1 for triggers too).
    float TriggerValue( int16_t nValue )  {

        return ( ( __TRIGGER_SPAN * ( float ) nValue ) / __AXIS_MAX ) + __TRIGGER_REST;
    }

    // A stick's SDL value (-32768..32767) in -1..1; -32768 is one step past -1 and is clamped.
    float StickValue( int16_t nValue )  {

        return std :: clamp( ( float ) nValue / __AXIS_MAX, __AXIS_MIN_VALUE, __AXIS_MAX_VALUE );
    }

    bool IsTriggerAxis( SunLight :: Input :: GamepadAxis axis )  {

        return ( axis == SunLight :: Input :: GAMEPAD_AXIS_LEFT_TRIGGER ) || ( axis == SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER );
    }

    const stButtonSource* SourceOf( SunLight :: Input :: GamepadButton button )  {

        for( const stButtonSource& source : kButtonSources )  {
            if( source.button == button )
                return &source;
        }

        return nullptr;
    }
}


namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief Holds the devices and the fallback, and takes the first snapshot.
             * @param pDevices The device set to read the pads from;
             * @param pFallback The backend to use when SDL could not start, or nullptr;
             */
            SDLGamepadInput :: SDLGamepadInput( std :: shared_ptr<SDLGamepadDevices> pDevices, std :: unique_ptr<SunLight :: Input :: IGamepadInput> pFallback )
                : m_pDevices( std :: move( pDevices ) ),
                  m_pFallback( std :: move( pFallback ) ),
                  m_Frame(),
                  m_bUseFallback( false ),
                  m_nLastButtonPressed( SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN )  {

                Update();
            }

            /**
             * @brief Lets go of the device set: the last holder stops its thread.
             */
            SDLGamepadInput :: ~SDLGamepadInput( void )  {

            }

            /**
             * @brief Takes the devices' latest snapshot (or, once SDL failed, updates the
             * fallback), then finds the last button held, as raylib's PollInputEvents does: reset,
             * then each pad in id order and each button in GLFW's order, the last one down kept.
             */
            void SDLGamepadInput :: Update( void )  {

                m_bUseFallback = ( m_pFallback != nullptr ) && ( m_pDevices -> GetState() == SDLGamepadDevices :: STATE_FAILED );

                if( m_bUseFallback )  {
                    m_pFallback -> Update();
                    return;
                }

                m_pDevices -> Read( m_Frame );
                m_nLastButtonPressed = SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN;

                for( int nGamePadId = 0; nGamePadId < MAX_GAMEPAD_COUNT; nGamePadId++ )  {
                    if( !m_Frame[ nGamePadId ].bAvailable )
                        continue;

                    for( SunLight :: Input :: GamepadButton button : kPressedOrder )  {
                        if( IsGamepadButtonDown( nGamePadId, button ) )
                            m_nLastButtonPressed = button;
                    }
                }
            }

            /**
             * @brief The pad's name from SDL's controller database, "" for no pad - valid until
             * the next Update.
             * @param nGamePadId The game pad id to check;
             */
            const char* SDLGamepadInput :: GetGamepadName( int nGamePadId )  {

                if( m_bUseFallback )
                    return m_pFallback -> GetGamepadName( nGamePadId );

                return IsGamepadAvailable( nGamePadId ) ? m_Frame[ nGamePadId ].strName.c_str() : __EMPTY_NAME;
            }

            /**
             * @brief Whether the pad was there at the last Update.
             * @param nGamePadId The game pad id to check;
             */
            bool SDLGamepadInput :: IsGamepadAvailable( int nGamePadId )  {

                if( m_bUseFallback )
                    return m_pFallback -> IsGamepadAvailable( nGamePadId );

                return ( nGamePadId >= 0 ) && ( nGamePadId < MAX_GAMEPAD_COUNT ) && m_Frame[ nGamePadId ].bAvailable;
            }

            /**
             * @brief Whether the button was held at the last Update; false for no pad and for
             * GAMEPAD_BUTTON_UNKNOWN.
             * @param nGamePadId The game pad id to check;
             * @param button Button code to check;
             */
            bool SDLGamepadInput :: IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                if( m_bUseFallback )
                    return m_pFallback -> IsGamepadButtonDown( nGamePadId, button );

                const stButtonSource  *pSource = SourceOf( button );

                if( !IsGamepadAvailable( nGamePadId ) || ( pSource == nullptr ) )
                    return false;

                const SDLGamepadDevices :: stPadState  &pad = m_Frame[ nGamePadId ];

                if( pSource -> bTriggerAxis )
                    return TriggerValue( pad.axes[ pSource -> nCode ] ) > __TRIGGER_THRESHOLD;

                return pad.buttons[ pSource -> nCode ];
            }

            /**
             * @brief Whether the button was up at the last Update - false for no pad, as raylib's
             * IsGamepadButtonUp answers for a pad that is not ready.
             * @param nGamePadId The game pad id to check;
             * @param button Button code to check;
             */
            bool SDLGamepadInput :: IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                if( m_bUseFallback )
                    return m_pFallback -> IsGamepadButtonUp( nGamePadId, button );

                return IsGamepadAvailable( nGamePadId ) && !IsGamepadButtonDown( nGamePadId, button );
            }

            /**
             * @brief The last button held at the last Update (see Update), or
             * GAMEPAD_BUTTON_UNKNOWN.
             */
            int SDLGamepadInput :: GetGamepadButtonPressed( void )  {

                if( m_bUseFallback )
                    return m_pFallback -> GetGamepadButtonPressed();

                return m_nLastButtonPressed;
            }

            /**
             * @brief Six for a pad (the two sticks and the two triggers), 0 for none.
             * @param nGamePadId The game pad id to check;
             */
            int SDLGamepadInput :: GetGamepadAxisCount( int nGamePadId )  {

                if( m_bUseFallback )
                    return m_pFallback -> GetGamepadAxisCount( nGamePadId );

                return IsGamepadAvailable( nGamePadId ) ? __AXIS_COUNT : 0;
            }

            /**
             * @brief The axis on raylib's scale (see the class's doc comment); with no pad, a
             * stick's rest (0) or a trigger's (-1), as raylib's GetGamepadAxisMovement answers.
             * @param nGamePadId The game pad id to check;
             * @param axis The axis to check;
             */
            float SDLGamepadInput :: GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis )  {

                if( m_bUseFallback )
                    return m_pFallback -> GetGamepadAxisMovement( nGamePadId, axis );

                bool  bTrigger = IsTriggerAxis( axis );

                if( !IsGamepadAvailable( nGamePadId ) || ( axis < SunLight :: Input :: GAMEPAD_AXIS_LEFT_X ) ||
                    ( axis > SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER ) )
                    return bTrigger ? __TRIGGER_REST : __STICK_REST;

                // sunlight's axes are SDL's, in the same order (LEFT_X .. RIGHT_TRIGGER = LEFTX ..
                // TRIGGERRIGHT), as raylib's are GLFW's.
                int16_t  nValue = m_Frame[ nGamePadId ].axes[ ( size_t ) axis ];

                return bTrigger ? TriggerValue( nValue ) : StickValue( nValue );
            }
        }
    }
}
