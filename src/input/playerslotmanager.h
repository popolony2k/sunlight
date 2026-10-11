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

#ifndef __PLAYERSLOTMANAGER_H__
#define __PLAYERSLOTMANAGER_H__

#include "input/iplayerslotmanager.h"
#include <map>


namespace SunLight {
    namespace Input  {

        /**
         * @brief IPlayerSlotManager's only implementation (F2 of the master plan): a plain
         * slot-to-gamepad map, directly constructible like ControllerManager - no backend
         * variation to swap here, so no factory either.
         */
        class PlayerSlotManager : public IPlayerSlotManager  {

            std :: map<int, int>  m_SlotToGamepad;

            public:

            PlayerSlotManager( void ) = default;
            ~PlayerSlotManager( void ) override = default;

            bool ClaimSlot( int nSlot, int nGamePadId ) override;
            void ReleaseSlot( int nSlot ) override;
            int GetGamepadForSlot( int nSlot ) const override;
            int GetSlotForGamepad( int nGamePadId ) const override;
        };
    }
}

#endif  /* __PLAYERSLOTMANAGER_H__ */
