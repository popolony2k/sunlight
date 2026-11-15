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
#include "input/iinputhandler.h"
#include <functional>
#include <memory>


namespace SunLight  {
    namespace Input  {

        /**
         * @brief Gamepad haptics access point. F7a shipped this with every build defaulting
         * to NullGamepadHaptics, since no real backend existed; F7b of the master plan gives
         * it the same build-time switch shape EngineFactory/WindowFactory/InputHandlerFactory
         * already use (DEFAULT_HAPTICS_ENGINE, 1 = SDL2's SDLGamepadHaptics, the only value
         * today). Each call returns a brand new IGamepadHaptics, the same fresh-instance shape
         * SoundFactory::CreateSound() uses, not a shared singleton. The real backend needs an
         * IInputHandler to correlate its own device list against (see SDLGamepadHaptics) -
         * ignored by NullGamepadHaptics and by a SetCreator override, both of which exist only
         * for callers that don't need it.
         */
        class GamepadHapticsFactory  {

            public:

            typedef std :: function<std :: unique_ptr<IGamepadHaptics>( void )>  CreatorFunction;

            /**
             * @brief Create a new IGamepadHaptics instance - the backend selected at build
             * time (DEFAULT_HAPTICS_ENGINE), unless overridden by SetCreator() (tests only).
             * @param inputHandler The input handler whose gamepad list the real backend
             * correlates against (see SDLGamepadHaptics) - the caller must keep it alive for
             * at least as long as the returned IGamepadHaptics; unused when a SetCreator
             * override is active;
             */
            static std :: unique_ptr<IGamepadHaptics> CreateGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler );

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

#endif /* __GAMEPADHAPTICSFACTORY_H__ */
