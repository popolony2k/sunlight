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

#ifndef __IPLAYERSLOTMANAGER_H__
#define __IPLAYERSLOTMANAGER_H__

/**
 * @brief Sentinel meaning "no association": GetGamepadForSlot returns this for a slot that
 * has claimed no gamepad, GetSlotForGamepad for a gamepad no slot has claimed.
 */
#define NO_SLOT_ASSOCIATION   -1


namespace SunLight  {
    namespace Input  {

        /**
         * @brief Player slot manager interface (F2 of the master plan): maps a game's own
         * player slot numbers (its own concept - sunlight imposes neither a count nor a range)
         * to gamepad device ids. Deliberately independent of IControllerManager/IInputHandler:
         * a slot's claim is never cleared by a gamepad becoming unavailable, only by an explicit
         * ReleaseSlot - unplugging and replugging the same device keeps its slot without any
         * action, and reassigning a freed slot to a different pad is the caller's own decision
         * (e.g. in response to IControllerManager's hot-plug event, F1), not something this
         * class does on its own.
         */
        class IPlayerSlotManager  {

            public:

            virtual ~IPlayerSlotManager( void ) = default;

            /**
             * @brief Claim a gamepad for a slot. Refused if the gamepad is already claimed by a
             * different slot. Claiming the same pad again for the same slot is a no-op success.
             * Claiming a different pad for a slot that already holds one replaces it, freeing
             * the one it held.
             * @param nSlot The slot number claiming the gamepad;
             * @param nGamePadId The gamepad device id to claim;
             * @return true if the slot now holds this gamepad, false if it was already claimed
             * by a different slot;
             */
            virtual bool ClaimSlot( int nSlot, int nGamePadId ) = 0;

            /**
             * @brief Release whatever gamepad a slot holds, freeing it for another slot to
             * claim. A no-op if the slot holds nothing.
             * @param nSlot The slot number to release;
             */
            virtual void ReleaseSlot( int nSlot ) = 0;

            /**
             * @brief The gamepad a slot currently holds.
             * @param nSlot The slot number to check;
             * @return The claimed gamepad device id, or NO_SLOT_ASSOCIATION if the slot holds
             * nothing;
             */
            virtual int GetGamepadForSlot( int nSlot ) const = 0;

            /**
             * @brief The slot currently holding a gamepad.
             * @param nGamePadId The gamepad device id to check;
             * @return The slot number holding it, or NO_SLOT_ASSOCIATION if no slot has claimed
             * it;
             */
            virtual int GetSlotForGamepad( int nGamePadId ) const = 0;
        };
    }
}

#endif /* __IPLAYERSLOTMANAGER_H__ */
