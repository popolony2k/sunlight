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

#include "input/gamepadhapticsfactory.h"

#ifndef DEFAULT_HAPTICS_ENGINE
    #error "Unexpected value of DEFAULT_HAPTICS_ENGINE"
#endif

#if DEFAULT_HAPTICS_ENGINE == 1    /* USES SDL2 */
    #include "backends/sdl/sdlgamepadhaptics.h"

    #define __DEFAULT_HAPTICS_ENGINE  SunLight :: Input :: SDL :: SDLGamepadHaptics
#else
    #error "Unknown value of DEFAULT_HAPTICS_ENGINE"
#endif

namespace SunLight  {
    namespace Input  {

        static GamepadHapticsFactory :: CreatorFunction  s_pCreator = nullptr;

        /**
         * @brief Create a new IGamepadHaptics instance - the backend selected at build time,
         * unless overridden by SetCreator() (tests only).
         */
        std :: unique_ptr<IGamepadHaptics> GamepadHapticsFactory :: CreateGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler )  {

            if( s_pCreator )
                return s_pCreator();

            return std :: make_unique<__DEFAULT_HAPTICS_ENGINE>( inputHandler );
        }

        /**
         * @brief Override the backend used by CreateGamepadHaptics().
         */
        void GamepadHapticsFactory :: SetCreator( CreatorFunction creator )  {

            s_pCreator = creator;
        }
    }
}
