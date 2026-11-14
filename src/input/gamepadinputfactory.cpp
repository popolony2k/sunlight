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

#include "input/gamepadinputfactory.h"

#ifndef DEFAULT_GAMEPAD_ENGINE
    #error "Unexpected value of DEFAULT_GAMEPAD_ENGINE"
#endif

#if DEFAULT_GAMEPAD_ENGINE == 1    /* USES SDL2, raylib/GLFW AS THE FALLBACK */
    #include "backends/sdl/sdlgamepadinput.h"
    #include "backends/raylib/raylibgamepadinput.h"
#elif DEFAULT_GAMEPAD_ENGINE == 2  /* USES RAYLIB/GLFW */
    #include "backends/raylib/raylibgamepadinput.h"
#else
    #error "Unknown value of DEFAULT_GAMEPAD_ENGINE"
#endif

namespace SunLight  {
    namespace Input  {

        static GamepadInputFactory :: CreatorFunction  s_pCreator = nullptr;

        /**
         * @brief A new IGamepadInput: the build's backend (see the class's doc comment), unless
         * overridden by SetCreator().
         */
        std :: unique_ptr<IGamepadInput> GamepadInputFactory :: CreateGamepadInput( void )  {

            if( s_pCreator )
                return s_pCreator();

#if DEFAULT_GAMEPAD_ENGINE == 1
            // Whether SDL starts is known only later, on its own thread (starting it runs its
            // drivers' pad set-up, see SDLGamepadDevices): the SDL input gets raylib/GLFW's pads as
            // its fallback and switches to them if SDL fails.
            return std :: make_unique<SunLight :: Input :: SDL :: SDLGamepadInput>( SunLight :: Input :: SDL :: SDLGamepadDevices :: Acquire(),
                                                                                  std :: make_unique<SunLight :: Input :: RayLib :: RayLibGamepadInput>() );
#else
            return std :: make_unique<SunLight :: Input :: RayLib :: RayLibGamepadInput>();
#endif
        }

        /**
         * @brief Override the backend used by CreateGamepadInput().
         * @param creator The replacement creator function, or an empty std::function;
         */
        void GamepadInputFactory :: SetCreator( CreatorFunction creator )  {

            s_pCreator = creator;
        }
    }
}
