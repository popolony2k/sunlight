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

#ifndef __SDLGAMEPADHAPTICS_H__
#define __SDLGAMEPADHAPTICS_H__

#include "input/igamepadhaptics.h"
#include "input/iinputhandler.h"
#include <cstdint>
#include <map>


namespace SunLight {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief Real gamepad rumble via SDL2's GameController API (F7b of the master
             * plan) - raylib/GLFW, which every other gamepad query in this project goes
             * through, has no vibration support at all. Every nGamePadId here is still the
             * same id IInputHandler uses (raylib/GLFW's own device index), re-correlated to
             * SDL's own, separately-enumerated device list on every call via
             * MatchGamepadsByName - exactly one gamepad on each side is always the same
             * device; with more than one on either side it falls back to a best-effort name
             * match, not a guaranteed-unique device identity (see gamepadidentitymatcher.h for
             * why - confirmed live that even a single, correctly-detected real controller can
             * have disagreeing names between the two libraries). Initializes/releases SDL's
             * joystick+game-controller subsystem by reference count
             * (SDL_InitSubSystem/SDL_QuitSubSystem, not SDL_Init/SDL_Quit), so more than one
             * instance can coexist safely.
             *
             * SetVibration's own duration does not auto-stop by itself - confirmed live: SDL2's
             * internal rumble expiration only runs inside SDL_JoystickUpdate(), which nothing
             * else in this engine ever calls. m_VibrationDeadlines tracks each vibrating pad's
             * own stop time (SunLight::General::Clock, not SDL's own clock), and Update() (the
             * owner's own per-frame tick) calls the real Stop() once it passes.
             */
            class SDLGamepadHaptics : public SunLight :: Input :: IGamepadHaptics  {

                SunLight :: Input :: IInputHandler&  m_InputHandler;
                std :: map<int, int64_t>             m_VibrationDeadlines;

                // The SDL device index matching nGamePadId's raylib/GLFW identity, or -1 if
                // nGamePadId is unavailable or no SDL device matched it.
                int FindMatchingSDLIndex( int nGamePadId ) const;

                public:

                explicit SDLGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler );
                ~SDLGamepadHaptics( void ) override;

                bool IsSupported( int nGamePadId ) const override;
                void SetVibration( int nGamePadId, float fLeftMotor, float fRightMotor, float fDuration ) override;
                void Stop( int nGamePadId ) override;
                void Update( void ) override;
            };
        }
    }
}

#endif  /* __SDLGAMEPADHAPTICS_H__ */
