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

#ifndef __IGAMEPADHAPTICS_H__
#define __IGAMEPADHAPTICS_H__


namespace SunLight {
    namespace Input  {

        /**
         * @brief Gamepad vibration (rumble) interface (F7a of the master plan). Deliberately
         * separate from IInputHandler - haptics is an output, not an input, and today has only
         * the one implementation below (NullGamepadHaptics, via GamepadHapticsFactory): nothing
         * is wired to a real device yet (F7b, left open until a vibrating platform is chosen).
         *
         * Update() must be called once per frame by whoever owns the instance. Confirmed live
         * against a real controller (F7b): SDL2's own duration-based auto-stop only runs inside
         * SDL_JoystickUpdate(), which nothing else in this engine ever calls (every other input
         * query goes through raylib/GLFW, never SDL's own event pump) - without a periodic tick
         * of some kind, a timed SetVibration() call never actually stops on its own. Update()
         * is sunlight's own replacement for that missing tick, timed against
         * SunLight::General::Clock rather than SDL's internal one.
         */
        class IGamepadHaptics  {

            public:

            virtual ~IGamepadHaptics( void ) = default;

            /**
             * @brief Whether a gamepad can vibrate at all. A game checks this to skip a
             * vibration effect entirely rather than calling SetVibration on a pad that can't
             * act on it.
             * @param nGamePadId The gamepad device id to check;
             * @return true if the pad supports vibration;
             */
            virtual bool IsSupported( int nGamePadId ) const = 0;

            /**
             * @brief Vibrate a gamepad's two motors for a duration, then stop by itself.
             * A no-op on a pad IsSupported reports false for.
             * @param nGamePadId The gamepad device id to vibrate;
             * @param fLeftMotor Left motor strength, 0 (off) to 1 (full);
             * @param fRightMotor Right motor strength, 0 (off) to 1 (full);
             * @param fDuration How long to vibrate, in seconds;
             */
            virtual void SetVibration( int nGamePadId, float fLeftMotor, float fRightMotor, float fDuration ) = 0;

            /**
             * @brief Stop a gamepad's vibration immediately, before its own duration elapses.
             * A no-op if it isn't currently vibrating.
             * @param nGamePadId The gamepad device id to stop;
             */
            virtual void Stop( int nGamePadId ) = 0;

            /**
             * @brief Drives time-based bookkeeping (a SetVibration call's own duration auto-
             * stopping - see this interface's own doc comment for why that needs a tick at
             * all). Must be called once per frame; a no-op with nothing currently vibrating.
             */
            virtual void Update( void ) = 0;
        };
    }
}

#endif  /* __IGAMEPADHAPTICS_H__ */
