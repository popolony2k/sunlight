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

#ifndef __GAMEPADHAPTICSFACTORY_H__
#define __GAMEPADHAPTICSFACTORY_H__

#include "input/igamepadhaptics.h"
#include <functional>
#include <memory>


namespace SunLight {
    namespace Input  {

        /**
         * @brief Gamepad haptics access point (F7a of the master plan). Unlike
         * EngineFactory/WindowFactory/InputHandlerFactory (each with a build-time backend
         * switch on DEFAULT_ENGINE), there is no real backend to switch to yet - every build's
         * default is NullGamepadHaptics, until F7b picks a vibrating platform and gives this
         * factory its own #if. Each call returns a brand new IGamepadHaptics, the same
         * fresh-instance shape SoundFactory::CreateSound() uses, not a shared singleton.
         */
        class GamepadHapticsFactory  {

            public:

            typedef std :: function<std :: unique_ptr<IGamepadHaptics>( void )>  CreatorFunction;

            static std :: unique_ptr<IGamepadHaptics> CreateGamepadHaptics( void );

            /**
             * @brief Override the backend used by CreateGamepadHaptics() - for tests only, to
             * substitute a mock IGamepadHaptics. Pass an empty CreatorFunction to restore the
             * default (NullGamepadHaptics).
             * @param creator The replacement creator function, or an empty std::function to
             * reset back to the default;
             */
            static void SetCreator( CreatorFunction creator );
        };
    }
}

#endif  /* __GAMEPADHAPTICSFACTORY_H__ */
