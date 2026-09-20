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

#ifndef __WINDOWFACTORY_H__
#define __WINDOWFACTORY_H__

#include "window/iwindow.h"


namespace SunLight {
    namespace Window  {

        /**
         * @brief Window backend access point used to retrieve the
         * @see IWindow implementation selected at build time. Gated by
         * the SAME DEFAULT_ENGINE switch as @see
         * SunLight::Engines::EngineFactory on purpose: a backend's window
         * and engine are one set (see IWindow's own comment), so they must
         * never be selectable independently of each other. This is the
         * only place in the window module that needs to know which
         * concrete backend is compiled in.
         */
        class WindowFactory  {

            public:

            static IWindow& GetWindow( void );

            /**
             * @brief Override the window returned by @see GetWindow() - for
             * tests only, to substitute a mock @see IWindow without a real
             * display. Pass nullptr to restore the default, build-time
             * backend. The override is not owned by the factory - the
             * caller keeps it alive for as long as it's set.
             *
             * @param pOverride The replacement window, or nullptr to reset
             * back to the default backend;
             */
            static void SetWindow( IWindow *pOverride );
        };
    }
}

#endif  /* __WINDOWFACTORY_H__ */
