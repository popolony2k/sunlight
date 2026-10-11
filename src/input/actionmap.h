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

#ifndef __ACTIONMAP_H__
#define __ACTIONMAP_H__

#include "input/iactionmap.h"
#include <map>


namespace SunLight {
    namespace Input  {

        /**
         * @brief IActionMap's only implementation (F3 of the master plan): a plain
         * action-name-to-binding map, directly constructible like ControllerManager and
         * PlayerSlotManager - no backend variation to swap here, so no factory either.
         */
        class ActionMap : public IActionMap  {

            std :: map<std :: string, SunLight :: Input :: stActionBinding>  m_Bindings;

            // The action already bound to this exact physical input (type/code, and direction
            // too for an axis binding), or an empty string if none holds it.
            std :: string FindActionBoundTo( SunLight :: Input :: ActionBindingType type, int nCode, int nDirection ) const;

            public:

            ActionMap( void ) = default;
            ~ActionMap( void ) override = default;

            bool BindKey( const std :: string& strAction, SunLight :: Input :: KeyboardKey key ) override;
            bool BindGamepadButton( const std :: string& strAction, SunLight :: Input :: GamepadButton button ) override;
            bool BindGamepadAxis( const std :: string& strAction, SunLight :: Input :: GamepadAxis axis, int nDirection ) override;
            void Unbind( const std :: string& strAction ) override;
            SunLight :: Input :: stActionBinding GetBinding( const std :: string& strAction ) const override;
            bool IsActionActive( const std :: string& strAction, SunLight :: Input :: IInputHandler& input,
                                 int nGamePadId, float fAxisThreshold ) const override;
            bool Save( const std :: string& strPath ) const override;
            bool Load( const std :: string& strPath ) override;
        };
    }
}

#endif  /* __ACTIONMAP_H__ */
