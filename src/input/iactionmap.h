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

#ifndef __IACTIONMAP_H__
#define __IACTIONMAP_H__

#include "input/iinputhandler.h"
#include <string>


namespace SunLight  {
    namespace Input  {

        /**
         * @brief What physical input kind a named action is bound to (F3 of the master plan).
         */
        enum ActionBindingType  {
            ACTION_BINDING_NONE,              // The action has no binding
            ACTION_BINDING_KEY,               // Bound to a keyboard key
            ACTION_BINDING_GAMEPAD_BUTTON,     // Bound to a gamepad button
            ACTION_BINDING_GAMEPAD_AXIS        // Bound to a gamepad axis past a threshold, in one direction
        };

        /**
         * @brief One action's current binding. nCode is a SunLight::Input::KeyboardKey,
         * GamepadButton or GamepadAxis value, depending on type (meaningless for
         * ACTION_BINDING_NONE). nDirection is meaningful only for ACTION_BINDING_GAMEPAD_AXIS:
         * +1 means the action is active past +threshold, -1 past -threshold.
         */
        struct stActionBinding  {
            ActionBindingType  type      = ACTION_BINDING_NONE;
            int                 nCode     = 0;
            int                 nDirection = 0;
        };

        /**
         * @brief Action map interface (F3 of the master plan): named actions, each bound to at
         * most one physical input (a key, a gamepad button, or a gamepad axis direction),
         * rebindable at any time, with save/load to a plain text file. Sunlight provides this
         * mechanism only - each game names its own actions and decides where its binding files
         * live.
         */
        class IActionMap  {

            public:

            virtual ~IActionMap( void ) = default;

            /**
             * @brief Bind an action to a keyboard key, replacing whatever binding the action
             * had before (a rebind). Refused if that key is already bound to a *different*
             * action.
             * @param strAction The action name;
             * @param key The key to bind it to;
             * @return true if bound, false if refused (already bound elsewhere);
             */
            virtual bool BindKey( const std :: string& strAction, SunLight :: Input :: KeyboardKey key ) = 0;

            /**
             * @brief Bind an action to a gamepad button, replacing whatever binding the action
             * had before (a rebind). Refused if that button is already bound to a *different*
             * action.
             * @param strAction The action name;
             * @param button The button to bind it to;
             * @return true if bound, false if refused (already bound elsewhere);
             */
            virtual bool BindGamepadButton( const std :: string& strAction, SunLight :: Input :: GamepadButton button ) = 0;

            /**
             * @brief Bind an action to a gamepad axis direction, replacing whatever binding the
             * action had before (a rebind). Refused if that axis and direction are already
             * bound to a *different* action - the opposite direction on the same axis is a
             * separate binding and never conflicts (e.g. MoveLeft/MoveRight on the same stick
             * axis).
             * @param strAction The action name;
             * @param axis The axis to bind it to;
             * @param nDirection +1 for the positive direction past the threshold, -1 for the
             * negative direction;
             * @return true if bound, false if refused (already bound elsewhere);
             */
            virtual bool BindGamepadAxis( const std :: string& strAction, SunLight :: Input :: GamepadAxis axis, int nDirection ) = 0;

            /**
             * @brief Remove whatever binding an action has. A no-op if it has none.
             * @param strAction The action name;
             */
            virtual void Unbind( const std :: string& strAction ) = 0;

            /**
             * @brief The binding an action currently has.
             * @param strAction The action name;
             * @return The binding, or one with type ACTION_BINDING_NONE if it has none;
             */
            virtual SunLight :: Input :: stActionBinding GetBinding( const std :: string& strAction ) const = 0;

            /**
             * @brief Whether an action's bound input is currently active. False for an action
             * with no binding.
             * @param strAction The action name;
             * @param input The input handler to poll;
             * @param nGamePadId The gamepad id to poll for a button/axis binding (ignored for a
             * key binding);
             * @param fAxisThreshold How far past center a bound axis must move, in its bound
             * direction, to read as active (ignored for a key or button binding);
             * @return true if the action's binding is currently satisfied;
             */
            virtual bool IsActionActive( const std :: string& strAction, SunLight :: Input :: IInputHandler& input,
                                         int nGamePadId, float fAxisThreshold ) const = 0;

            /**
             * @brief Save every current binding to a plain text file.
             * @param strPath The file path to write;
             * @return true on success, false if the file could not be written;
             */
            virtual bool Save( const std :: string& strPath ) const = 0;

            /**
             * @brief Load bindings from a plain text file, replacing every binding this map
             * currently holds. Left unchanged if the file can't be read or parsed.
             * @param strPath The file path to read;
             * @return true on success, false if the file could not be read or parsed;
             */
            virtual bool Load( const std :: string& strPath ) = 0;
        };
    }
}

#endif /* __IACTIONMAP_H__ */
