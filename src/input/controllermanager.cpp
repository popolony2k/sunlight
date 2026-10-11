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

#include "input/controllermanager.h"
#include "input/inputhandlerfactory.h"

namespace SunLight  {
    namespace Input  {

        namespace  {

            // Analog stick deadzones: a stick position within this distance of center reads as
            // exactly zero, so a worn/uncalibrated stick does not dispatch a phantom direction.
            // Trigger deadzones are not consulted yet (axis-only triggers have no face-button
            // equivalent to translate to today), kept here only as the same named values
            // HandleUserInput's own inline literals used.
            const float  __LEFT_STICK_DEADZONE_X  = 0.1f;
            const float  __LEFT_STICK_DEADZONE_Y  = 0.1f;
            const float  __RIGHT_STICK_DEADZONE_X = 0.1f;
            const float  __RIGHT_STICK_DEADZONE_Y = 0.1f;
        }

        ControllerManager :: ControllerManager( void )  {

            m_pInputHandler         = SunLight :: Input :: InputHandlerFactory :: CreateInputHandler();
            m_pNullInputEventHandler = nullptr;
            m_DispatchMode           = SunLight :: Input :: INPUT_DISPATCH_MODE_ACTIVE;
            m_pConnectionHandler     = nullptr;
        }

        /**
         * @brief Set the gamepad id that will be polled by this manager.
         * @param nGamePadId The gamepad device index to add;
         */
        void ControllerManager :: AddGamePad( int nGamePadId )  {

            m_GamePadList.push_back( nGamePadId );
        }

        /**
         * @brief Set the user key event handler callback routine.
         * @param evt The event code to call the related routine for;
         * @param handler The callback routine that will be called for the key event;
         */
        void ControllerManager :: SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt,
                                                           SunLight :: Input :: INPUT_EVENT_HANDLER handler )  {

            if( evt != SunLight :: Input :: KeyboardKey :: KEY_NULL )  {
                std :: unique_ptr<__stInputEventData>  pEvtData = std :: make_unique<__stInputEventData>();

                pEvtData -> nEvent   = evt;
                pEvtData -> pHandler = handler;
                m_KeyInputEventHandlerList.push_back( std :: move( pEvtData ) );
            }
            else  {
                m_pNullInputEventHandler = handler;
            }
        }

        /**
         * @brief Set the user gamepad button event handler callback routine.
         * @param evt The event code to call the related routine for;
         * @param handler The callback routine that will be called for the button event;
         */
        void ControllerManager :: SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt,
                                                               SunLight :: Input :: INPUT_EVENT_HANDLER handler )  {

            std :: unique_ptr<__stInputEventData>  pEvtData = std :: make_unique<__stInputEventData>();

            pEvtData -> nEvent   = evt;
            pEvtData -> pHandler = handler;
            m_GPadInputEventHandlerList.push_back( std :: move( pEvtData ) );
        }

        SunLight :: Input :: IInputHandler& ControllerManager :: GetInputHandler( void )  {

            return *m_pInputHandler;
        }

        void ControllerManager :: SetInputDispatchMode( SunLight :: Input :: InputDispatchMode mode )  {

            m_DispatchMode = mode;
        }

        SunLight :: Input :: InputDispatchMode ControllerManager :: GetInputDispatchMode( void ) const  {

            return m_DispatchMode;
        }

        /**
         * @brief List every gamepad slot currently reported available, with its name.
         * @return One entry per connected gamepad, in device id order;
         */
        std :: vector<SunLight :: Input :: stGamepadInfo> ControllerManager :: GetConnectedGamepads( void ) const  {

            std :: vector<SunLight :: Input :: stGamepadInfo>  connectedList;

            for( int nGamePadId = 0; nGamePadId < MAX_GAMEPAD_COUNT; nGamePadId++ )  {
                if( m_pInputHandler -> IsGamepadAvailable( nGamePadId ) )
                    connectedList.push_back( { nGamePadId, m_pInputHandler -> GetGamepadName( nGamePadId ) } );
            }

            return connectedList;
        }

        void ControllerManager :: SetGamepadConnectionHandler( SunLight :: Input :: GAMEPAD_CONNECTION_HANDLER handler )  {

            m_pConnectionHandler = handler;
        }

        /**
         * @brief Dispatch to every registered key handler whose event matches evt.
         * @param evt The key event to dispatch;
         */
        void ControllerManager :: HandleKeyEvent( SunLight :: Input :: KeyboardKey evt )  {

            for( InputEventHandlerList :: iterator itItem = m_KeyInputEventHandlerList.begin(); itItem != m_KeyInputEventHandlerList.end(); itItem++ )  {
                if( ( *itItem ) -> nEvent == evt )
                    ( *itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_KEYBOARD, KEYBOARD_ID );
            }
        }

        /**
         * @brief Polls every gamepad slot (0..MAX_GAMEPAD_COUNT-1, independent of AddGamePad)
         * and fires m_pConnectionHandler once for each one whose availability changed since the
         * last call - a slot that was never added still gets reported, since a newly plugged-in
         * pad must be discoverable before anything registers it for dispatch.
         */
        void ControllerManager :: DetectGamepadConnectionChanges( void )  {

            for( int nGamePadId = 0; nGamePadId < MAX_GAMEPAD_COUNT; nGamePadId++ )  {
                bool    bNowAvailable = m_pInputHandler -> IsGamepadAvailable( nGamePadId );

                if( bNowAvailable != m_GamepadConnected[ nGamePadId ] )  {
                    m_GamepadConnected[ nGamePadId ] = bNowAvailable;

                    if( m_pConnectionHandler != nullptr )
                        m_pConnectionHandler( nGamePadId, bNowAvailable );
                }
            }
        }

        /**
         * @brief Poll every added gamepad and dispatch matching gamepad handlers (analog sticks
         * translated to their DPad-equivalent direction, beyond the deadzone), then dispatch
         * matching keyboard handlers per the current InputDispatchMode, then the no-input
         * fallback if nothing else fired this frame; also detects and reports gamepad
         * connect/disconnect transitions. Must be called once per frame.
         */
        void ControllerManager :: Update( void )  {

            bool    bEventHandled = false;

            DetectGamepadConnectionChanges();

            for( int nGamePadId : m_GamePadList )  {
                SunLight :: Input :: GamepadButton button[]    = { SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_UNKNOWN,
                                                                   SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_UNKNOWN,
                                                                   SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_UNKNOWN,
                                                                   SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_UNKNOWN };
                float    fPosX = m_pInputHandler -> GetGamepadAxisMovement( nGamePadId,
                                                                            SunLight :: Input :: GamepadAxis :: GAMEPAD_AXIS_LEFT_X );
                float    fPosY = m_pInputHandler -> GetGamepadAxisMovement( nGamePadId,
                                                                            SunLight :: Input :: GamepadAxis :: GAMEPAD_AXIS_LEFT_Y );
                float    fRightPosX = m_pInputHandler -> GetGamepadAxisMovement( nGamePadId,
                                                                                 SunLight :: Input :: GamepadAxis :: GAMEPAD_AXIS_RIGHT_X );
                float    fRightPosY = m_pInputHandler -> GetGamepadAxisMovement( nGamePadId,
                                                                                 SunLight :: Input :: GamepadAxis :: GAMEPAD_AXIS_RIGHT_Y );

                // Calculate deadzones
                if( ( fPosX > -__LEFT_STICK_DEADZONE_X ) && ( fPosX < __LEFT_STICK_DEADZONE_X ) )
                    fPosX = 0.0f;
                if( ( fPosY > -__RIGHT_STICK_DEADZONE_Y ) && ( fPosY < __RIGHT_STICK_DEADZONE_Y ) )
                    fPosY = 0.0f;
                if( ( fRightPosX > -__RIGHT_STICK_DEADZONE_X ) && ( fRightPosX < __RIGHT_STICK_DEADZONE_X ) )
                    fRightPosX = 0.0f;
                if( ( fRightPosY > -__RIGHT_STICK_DEADZONE_Y ) && ( fRightPosY < __RIGHT_STICK_DEADZONE_Y ) )
                    fRightPosY = 0.0f;

                // Handle Analog GamePad control stick (left)
                if( fPosX > 0.0 )  {
                    button[0] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
                    bEventHandled = true;
                }
                else  {
                    if( fPosX < 0.0 )  {
                        button[0] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_LEFT;
                        bEventHandled = true;
                    }
                }

                if( fPosY > 0.0 )  {
                    button[1] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_DOWN;
                    bEventHandled = true;
                }
                else  {
                    if( fPosY < 0.0 )  {
                        button[1] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_UP;
                        bEventHandled = true;
                    }
                }

                // Handle Analog GamePad control stick (right)
                if( fRightPosX > 0.0 )  {
                    button[2] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
                    bEventHandled = true;
                }
                else  {
                    if( fRightPosX < 0.0 )  {
                        button[2] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
                        bEventHandled = true;
                    }
                }

                if( fRightPosY > 0.0 )  {
                    button[3] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
                    bEventHandled = true;
                }
                else  {
                    if( fRightPosY < 0.0 )  {
                        button[3] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_UP;
                        bEventHandled = true;
                    }
                }

                // Handle DPad and analog stick controls (left + right)
                for( InputEventHandlerList :: iterator itItem = m_GPadInputEventHandlerList.begin(); itItem != m_GPadInputEventHandlerList.end(); itItem++ )  {
                    SunLight :: Input :: GamepadButton   evt = ( SunLight :: Input :: GamepadButton ) ( * itItem ) -> nEvent;

                    /*
                     * DPad and Analog MUST be processed in the same handler list iteration.
                     * The order of events can affect main renderer processing behavior
                     * (depending on final developer game logic);
                     */
                    if( m_pInputHandler -> IsGamepadButtonDown( nGamePadId, evt ) )  {
                        ( * itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_GAMEPAD, nGamePadId );
                        bEventHandled = true;
                    }
                    else  {
                        if( ( evt == button[0] ) || ( evt == button[1] ) || ( evt == button[2] ) || ( evt == button[3] ) )  {
                            ( * itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_GAMEPAD, nGamePadId );
                            bEventHandled = true;
                        }
                    }
                }
            }

            // Handle keyboard if there's no response from game pad
            if( !bEventHandled )  {
                switch( m_DispatchMode )  {
                    case SunLight :: Input :: INPUT_DISPATCH_MODE_REACTIVE :
                        HandleKeyEvent( m_pInputHandler -> GetKeyPressed() );
                        bEventHandled = true;
                    break;

                    case SunLight :: Input :: INPUT_DISPATCH_MODE_ACTIVE :  {
                        for( InputEventHandlerList :: iterator itItem = m_KeyInputEventHandlerList.begin(); itItem != m_KeyInputEventHandlerList.end(); itItem++ )  {
                            if( m_pInputHandler -> IsKeyDown( ( SunLight :: Input :: KeyboardKey ) ( * itItem ) -> nEvent ) )  {
                                ( * itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_KEYBOARD, KEYBOARD_ID );
                                bEventHandled = true;
                            }
                        }
                    }
                }
            }

            if( !bEventHandled && ( m_pNullInputEventHandler != nullptr ) )  {
                m_pNullInputEventHandler( SunLight :: Input :: CONTROLLER_NULL, -1 );
            }
        }
    }
}
