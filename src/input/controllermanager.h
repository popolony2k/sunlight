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

#ifndef __CONTROLLERMANAGER_H__
#define __CONTROLLERMANAGER_H__

#include "input/icontrollermanager.h"
#include <deque>
#include <memory>


namespace SunLight {
    namespace Input  {

        /**
         * @brief IControllerManager's only implementation (F0a of the master plan): owns its own
         * IInputHandler (via InputHandlerFactory - the same real-or-mock swap point
         * TileMapRenderer used directly before), the registered key/gamepad handler lists and the
         * gamepad list, and dispatches both once per frame from Update(). No factory of its own:
         * unlike IInputHandler/IEngine/IWindow, there is no backend variation to swap here - the
         * dispatch logic is the same C++ regardless of platform, the same reasoning
         * CollisionManager's own lack of a factory already follows.
         */
        class ControllerManager : public IControllerManager  {

            /**
             * @brief One registered handler and the key/button code it fires for.
             */
            struct __stInputEventData  {
                int                                       nEvent;
                SunLight :: Input :: INPUT_EVENT_HANDLER  pHandler;
            };

            typedef std :: deque<std :: unique_ptr<__stInputEventData>> InputEventHandlerList;
            typedef std :: deque<int>                                  GamePadList;

            std :: unique_ptr<SunLight :: Input :: IInputHandler>  m_pInputHandler;
            GamePadList                                            m_GamePadList;
            InputEventHandlerList                                  m_KeyInputEventHandlerList;
            InputEventHandlerList                                  m_GPadInputEventHandlerList;
            SunLight :: Input :: INPUT_EVENT_HANDLER               m_pNullInputEventHandler;
            SunLight :: Input :: InputDispatchMode                 m_DispatchMode;

            // Dispatches to every registered key handler whose event matches evt - the
            // REACTIVE dispatch strategy's own per-key-press lookup.
            void HandleKeyEvent( SunLight :: Input :: KeyboardKey evt );

            public:

            ControllerManager( void );
            ~ControllerManager( void ) override = default;

            void AddGamePad( int nGamePadId ) override;
            void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt,
                                         SunLight :: Input :: INPUT_EVENT_HANDLER handler ) override;
            void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt,
                                             SunLight :: Input :: INPUT_EVENT_HANDLER handler ) override;
            SunLight :: Input :: IInputHandler& GetInputHandler( void ) override;
            void SetInputDispatchMode( SunLight :: Input :: InputDispatchMode mode ) override;
            SunLight :: Input :: InputDispatchMode GetInputDispatchMode( void ) const override;
            void Update( void ) override;
        };
    }
}

#endif  /* __CONTROLLERMANAGER_H__ */
