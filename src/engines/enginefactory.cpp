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

#include "enginefactory.h"

#ifndef DEFAULT_ENGINE
    #error "Unexpected value of DEFAULT_ENGINE"
#endif

#if DEFAULT_ENGINE == 1    /* USES RAYLIB */
    #include "engines/raylib/raylibengine.h"

    #define __DEFAULT_ENGINE  SunLight :: Engines :: Raylib :: RaylibEngine
#else
    #error "Unknown value of DEFAULT_ENGINE"
#endif


namespace SunLight {
    namespace Engines  {

        static IEngine  *s_pOverrideEngine = nullptr;

        /**
         * @brief Get the single @see IEngine instance for the backend
         * selected at build time, unless overridden by @see SetEngine()
         * (tests only).
         */
        IEngine& EngineFactory :: GetEngine( void )  {

            if( s_pOverrideEngine )
                return *s_pOverrideEngine;

            static __DEFAULT_ENGINE engine;

            return engine;
        }

        /**
         * @brief Override the engine returned by @see GetEngine().
         */
        void EngineFactory :: SetEngine( IEngine *pOverride )  {

            s_pOverrideEngine = pOverride;
        }
    }
}
