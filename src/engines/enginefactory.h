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

#ifndef __ENGINEFACTORY_H__
#define __ENGINEFACTORY_H__

#include "engines/iengine.h"


namespace SunLight {
    namespace Engines  {

        /**
         * @brief Rendering engine access point used to retrieve the
         * @see IEngine implementation selected at build time (DEFAULT_ENGINE).
         * This is the only place in the codebase that needs to know which
         * concrete backend is compiled in.
         */
        class EngineFactory  {

            public:

            static IEngine& GetEngine( void );

            /**
             * @brief Override the engine returned by @see GetEngine() - for
             * tests only, to substitute a mock @see IEngine without a real
             * window/render context. Pass nullptr to restore the default,
             * build-time backend. The override is not owned by the factory -
             * the caller keeps it alive for as long as it's set.
             *
             * @param pOverride The replacement engine, or nullptr to reset
             * back to the default backend;
             */
            static void SetEngine( IEngine *pOverride );
        };
    }
}

#endif  /* __ENGINEFACTORY_H__ */
