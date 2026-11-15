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

#include "input/playerslotmanager.h"

namespace SunLight  {
    namespace Input  {

        /**
         * @brief Claim a gamepad for a slot, refusing one already held by a different slot.
         */
        bool PlayerSlotManager :: ClaimSlot( int nSlot, int nGamePadId )  {

            int    nHoldingSlot = GetSlotForGamepad( nGamePadId );

            if( ( nHoldingSlot != NO_SLOT_ASSOCIATION ) && ( nHoldingSlot != nSlot ) )
                return false;

            m_SlotToGamepad[ nSlot ] = nGamePadId;

            return true;
        }

        /**
         * @brief Release whatever gamepad a slot holds, freeing it for another slot to claim.
         */
        void PlayerSlotManager :: ReleaseSlot( int nSlot )  {

            m_SlotToGamepad.erase( nSlot );
        }

        /**
         * @brief The gamepad a slot currently holds, or NO_SLOT_ASSOCIATION.
         */
        int PlayerSlotManager :: GetGamepadForSlot( int nSlot ) const  {

            std :: map<int, int> :: const_iterator  itItem = m_SlotToGamepad.find( nSlot );

            return ( itItem != m_SlotToGamepad.end() ) ? itItem -> second : NO_SLOT_ASSOCIATION;
        }

        /**
         * @brief The slot currently holding a gamepad, or NO_SLOT_ASSOCIATION.
         */
        int PlayerSlotManager :: GetSlotForGamepad( int nGamePadId ) const  {

            for( const std :: pair<const int, int>& entry : m_SlotToGamepad )  {
                if( entry.second == nGamePadId )
                    return entry.first;
            }

            return NO_SLOT_ASSOCIATION;
        }
    }
}
