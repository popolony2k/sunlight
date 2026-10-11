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

#ifndef __ICONTROLLERMANAGER_H__
#define __ICONTROLLERMANAGER_H__

#include "input/iinputhandler.h"
#include <vector>

/**
 * @brief Default analog stick axis deadzone (F4 of the master plan): a stick position within
 * this distance of center reads as exactly zero when translating sticks to DPad-equivalent
 * handlers, unless SetAxisDeadzone configures a different value for that exact (pad, axis)
 * pair. Today's own value, unchanged by F4 - only now it is the same named constant for every
 * axis, where before each stick axis had its own (identically-valued) copy.
 */
#define DEFAULT_AXIS_DEADZONE   0.1f


namespace SunLight {
    namespace Input  {

        /**
         * @brief How a controller manager's registered handlers get triggered each frame (F0 of
         * the master plan). Renamed from the renderer's own ViewControlMode - the same two
         * strategies, but the old name was camera-flavored for what has only ever been an input
         * dispatch concern, never a camera one.
         */
        enum InputDispatchMode  {
            INPUT_DISPATCH_MODE_ACTIVE,     // continuous IsKeyDown/IsGamepadButtonDown polling: fires every frame a key/button is held
            INPUT_DISPATCH_MODE_REACTIVE    // fires once per key-press edge (GetKeyPressed), gamepad dispatch unaffected
        };

        /**
         * @brief One connected gamepad, as reported by GetConnectedGamepads() (F1 of the master
         * plan): its device id and the backend's own name string for it (never nullptr - see
         * IInputHandler::GetGamepadName).
         */
        struct stGamepadInfo  {
            int          nGamePadId;
            const char  *szName;
        };

        /**
         * @brief Hot-plug event handler callback definition (F1 of the master plan).
         *
         * Parameters accepted:
         * int The gamepad id whose connection state changed;
         * bool true if the gamepad just became available, false if it was just lost;
         */
        typedef std :: function<void( int, bool )> GAMEPAD_CONNECTION_HANDLER;

        /**
         * @brief Controller manager base interface (F0a of the master plan): registers key and
         * gamepad button handlers, polls the gamepads it is told about, and dispatches both once
         * per frame via Update(). Extracted out of TileMapRenderer, which owned this logic
         * directly before - everything here is pure input dispatch, with no dependency on a map,
         * camera or renderer at all.
         */
        class IControllerManager  {

            public:

            virtual ~IControllerManager( void ) = default;

            /**
             * @brief Add a gamepad id this manager polls each Update(). Adding the same id twice
             * is not deduplicated (see F5 of the master plan) - every registered handler whose
             * event matches dispatches once per occurrence in the gamepad list.
             * @param nGamePadId The gamepad device index to poll;
             */
            virtual void AddGamePad( int nGamePadId ) = 0;

            /**
             * @brief Register a handler for a keyboard key. Passing KEY_NULL registers instead a
             * fallback handler, called once per Update() when no other registered key or gamepad
             * handler fired that frame.
             * @param evt The key to handle, or KEY_NULL for the no-input fallback;
             * @param handler The callback to invoke;
             */
            virtual void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt,
                                                 SunLight :: Input :: INPUT_EVENT_HANDLER handler ) = 0;

            /**
             * @brief Register a handler for a gamepad button (including the DPad-equivalent
             * directions GetGamepadAxisMovement's analog sticks translate to - see Update()).
             * @param evt The button to handle;
             * @param handler The callback to invoke;
             */
            virtual void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt,
                                                     SunLight :: Input :: INPUT_EVENT_HANDLER handler ) = 0;

            /**
             * @brief The raw input handler this manager polls through.
             * @return A reference to the IInputHandler in use;
             */
            virtual SunLight :: Input :: IInputHandler& GetInputHandler( void ) = 0;

            /**
             * @brief Set which strategy Update() uses to decide when a registered KEY handler
             * fires (gamepad dispatch is always a per-button down/translated-axis check,
             * unaffected by this mode).
             * @param mode The dispatch strategy to use;
             */
            virtual void SetInputDispatchMode( SunLight :: Input :: InputDispatchMode mode ) = 0;

            /**
             * @brief The dispatch strategy Update() currently uses for keyboard handlers.
             * @return The current InputDispatchMode;
             */
            virtual SunLight :: Input :: InputDispatchMode GetInputDispatchMode( void ) const = 0;

            /**
             * @brief List every gamepad currently reported available, with its name - a fresh
             * poll each call, independent of AddGamePad (a pad can be discovered before anything
             * registers it for button/axis dispatch).
             * @return One entry per connected gamepad, in device id order;
             */
            virtual std :: vector<SunLight :: Input :: stGamepadInfo> GetConnectedGamepads( void ) const = 0;

            /**
             * @brief Register the handler Update() calls when a gamepad's availability changes -
             * once per transition, never for a pad whose state hasn't changed since the last
             * Update(). Replaces any handler set before.
             * @param handler The callback to invoke, or nullptr to stop receiving the event;
             */
            virtual void SetGamepadConnectionHandler( SunLight :: Input :: GAMEPAD_CONNECTION_HANDLER handler ) = 0;

            /**
             * @brief Configure the deadzone for one gamepad's one axis, overriding
             * DEFAULT_AXIS_DEADZONE for that exact (pad, axis) pair only. Only the two
             * analog sticks' four axes are consulted by Update() today - see GetAxisDeadzone.
             * @param nGamePadId The gamepad device id;
             * @param axis The axis to configure;
             * @param fDeadzone The deadzone radius (a stick position strictly within this
             * distance of center reads as zero; exactly at the boundary reads as a real value);
             */
            virtual void SetAxisDeadzone( int nGamePadId, SunLight :: Input :: GamepadAxis axis, float fDeadzone ) = 0;

            /**
             * @brief The deadzone configured for one gamepad's one axis.
             * @param nGamePadId The gamepad device id;
             * @param axis The axis to check;
             * @return The configured deadzone, or DEFAULT_AXIS_DEADZONE if never configured
             * for this exact (pad, axis) pair;
             */
            virtual float GetAxisDeadzone( int nGamePadId, SunLight :: Input :: GamepadAxis axis ) const = 0;

            /**
             * @brief Poll every added gamepad and dispatch matching gamepad handlers, then
             * dispatch matching keyboard handlers per the current InputDispatchMode, then the
             * no-input fallback if nothing else fired; also detects and reports gamepad
             * connect/disconnect transitions (see SetGamepadConnectionHandler). Must be called
             * once per frame.
             */
            virtual void Update( void ) = 0;
        };
    }
}

#endif  /* __ICONTROLLERMANAGER_H__ */
