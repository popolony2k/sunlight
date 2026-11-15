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
#include <algorithm>

namespace SunLight  {
    namespace Input  {

        ControllerManager :: ControllerManager( void )  {

            m_pInputHandler         = SunLight :: Input :: InputHandlerFactory :: CreateInputHandler();
            m_pNullInputEventHandler = nullptr;
            m_DispatchMode           = SunLight :: Input :: INPUT_DISPATCH_MODE_ACTIVE;
            m_pConnectionHandler     = nullptr;
        }

        /**
         * @brief Set the gamepad id that will be polled by this manager - a no-op if that id
         * is already added (F5 of the master plan: a duplicate add must not double-dispatch).
         * @param nGamePadId The gamepad device index to add;
         */
        void ControllerManager :: AddGamePad( int nGamePadId )  {

            if( std :: find( m_GamePadList.begin(), m_GamePadList.end(), nGamePadId ) == m_GamePadList.end() )
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

        void ControllerManager :: SetAxisDeadzone( int nGamePadId, SunLight :: Input :: GamepadAxis axis, float fDeadzone )  {

            m_AxisDeadzones[ { nGamePadId, ( int ) axis } ] = fDeadzone;
        }

        float ControllerManager :: GetAxisDeadzone( int nGamePadId, SunLight :: Input :: GamepadAxis axis ) const  {

            std :: map<std :: pair<int, int>, float> :: const_iterator  itItem = m_AxisDeadzones.find( { nGamePadId, ( int ) axis } );

            return ( itItem != m_AxisDeadzones.end() ) ? itItem -> second : DEFAULT_AXIS_DEADZONE;
        }

        /**
         * @brief Dispatch to every registered key handler whose event matches evt.
         * @param evt The key event to dispatch;
         */
        void ControllerManager :: HandleKeyEvent( SunLight :: Input :: KeyboardKey evt )  {

            // By index, over the handlers there were when this frame's dispatch began - see Update().
            for( size_t nItem = 0; nItem < m_nKeyHandlersThisFrame; nItem++ )  {
                if( m_KeyInputEventHandlerList[nItem] -> nEvent == evt )
                    m_KeyInputEventHandlerList[nItem] -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_KEYBOARD, KEYBOARD_ID );
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

            /*
             * A handler may call SetUserKeyEventHandler/SetUserGamePadEventHandler/AddGamePad
             * (eg. a menu switching it's controls): each appends to a std::deque walked right here,
             * and a deque's push_back invalidates EVERY iterator into it - the range-for/iterator
             * loops this replaced were undefined behaviour then. Its indexes and element references
             * stay valid, so the lists are walked by index, over the counts they had when this
             * frame's dispatch began: what a handler adds is dispatched from the next frame on.
             * Nothing is ever removed from these lists, so no index can go stale.
             */
            size_t  nGamePads             = m_GamePadList.size();
            size_t  nGamePadHandlers      = m_GPadInputEventHandlerList.size();

            m_nKeyHandlersThisFrame = m_KeyInputEventHandlerList.size();

            for( size_t nPad = 0; nPad < nGamePads; nPad++ )  {
                int      nGamePadId = m_GamePadList[nPad];
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

                // Calculate deadzones - per (pad, axis), F4 of the master plan; a value
                // strictly inside the deadzone reads as zero, exactly at its boundary reads as
                // a real value (pinned by test_controllermanager.cpp).
                float    fLeftDeadzoneX  = GetAxisDeadzone( nGamePadId, SunLight :: Input :: GAMEPAD_AXIS_LEFT_X );
                float    fLeftDeadzoneY  = GetAxisDeadzone( nGamePadId, SunLight :: Input :: GAMEPAD_AXIS_LEFT_Y );
                float    fRightDeadzoneX = GetAxisDeadzone( nGamePadId, SunLight :: Input :: GAMEPAD_AXIS_RIGHT_X );
                float    fRightDeadzoneY = GetAxisDeadzone( nGamePadId, SunLight :: Input :: GAMEPAD_AXIS_RIGHT_Y );

                if( ( fPosX > -fLeftDeadzoneX ) && ( fPosX < fLeftDeadzoneX ) )
                    fPosX = 0.0f;
                if( ( fPosY > -fLeftDeadzoneY ) && ( fPosY < fLeftDeadzoneY ) )
                    fPosY = 0.0f;
                if( ( fRightPosX > -fRightDeadzoneX ) && ( fRightPosX < fRightDeadzoneX ) )
                    fRightPosX = 0.0f;
                if( ( fRightPosY > -fRightDeadzoneY ) && ( fRightPosY < fRightDeadzoneY ) )
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
                for( size_t nItem = 0; nItem < nGamePadHandlers; nItem++ )  {
                    __stInputEventData                   *pItem = m_GPadInputEventHandlerList[nItem].get();
                    SunLight :: Input :: GamepadButton   evt    = ( SunLight :: Input :: GamepadButton ) pItem -> nEvent;

                    /*
                     * DPad and Analog MUST be processed in the same handler list iteration.
                     * The order of events can affect main renderer processing behavior
                     * (depending on final developer game logic);
                     */
                    if( m_pInputHandler -> IsGamepadButtonDown( nGamePadId, evt ) )  {
                        pItem -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_GAMEPAD, nGamePadId );
                        bEventHandled = true;
                    }
                    else  {
                        if( ( evt == button[0] ) || ( evt == button[1] ) || ( evt == button[2] ) || ( evt == button[3] ) )  {
                            pItem -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_GAMEPAD, nGamePadId );
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
                        for( size_t nItem = 0; nItem < m_nKeyHandlersThisFrame; nItem++ )  {
                            __stInputEventData  *pItem = m_KeyInputEventHandlerList[nItem].get();

                            if( m_pInputHandler -> IsKeyDown( ( SunLight :: Input :: KeyboardKey ) pItem -> nEvent ) )  {
                                pItem -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_KEYBOARD, KEYBOARD_ID );
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
