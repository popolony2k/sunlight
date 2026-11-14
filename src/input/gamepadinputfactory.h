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

#ifndef __GAMEPADINPUTFACTORY_H__
#define __GAMEPADINPUTFACTORY_H__

#include "input/igamepadinput.h"
#include <functional>
#include <memory>


namespace SunLight  {
    namespace Input  {

        /**
         * @brief Where an input handler's gamepads come from (I25 of the master plan), chosen at
         * build time by DEFAULT_GAMEPAD_ENGINE, independent of DEFAULT_ENGINE like the haptics
         * switch: 1 = SDL2 (SDLGamepadInput, the default), 2 = raylib/GLFW (RayLibGamepadInput).
         * With 1, a process whose SDL game-controller subsystem fails to start gets raylib/GLFW's
         * pads instead (SDLGamepadInput switches to them once SDL's thread reports the failure), so
         * a game still has gamepads. Each call returns a new instance; the SDL
         * ones share one device list (SDLGamepadDevices).
         */
        class GamepadInputFactory  {

            public:

            typedef std :: function<std :: unique_ptr<IGamepadInput>( void )>  CreatorFunction;

            /**
             * @brief A new IGamepadInput - the build's backend, unless overridden by SetCreator()
             * (tests only).
             */
            static std :: unique_ptr<IGamepadInput> CreateGamepadInput( void );

            /**
             * @brief Override the backend used by CreateGamepadInput() - for tests only. An empty
             * CreatorFunction restores the build's backend.
             * @param creator The replacement creator function, or an empty std::function;
             */
            static void SetCreator( CreatorFunction creator );
        };
    }
}

#endif /* __GAMEPADINPUTFACTORY_H__ */
