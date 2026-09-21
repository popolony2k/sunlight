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

#ifndef __NULLBACKEND_H__
#define __NULLBACKEND_H__

#include <memory>
#include "backends/null/nullengine.h"
#include "backends/null/nullwindow.h"
#include "general/virtualclock.h"

namespace SunLight  {
    namespace Backends  {
        namespace Null  {

            /**
             * @brief The null backend as ONE set - engine, window, input
             * handler and virtual clock - installed and removed together, so
             * a mismatched combination (e.g. a null window with the real
             * engine) can't happen.
             *
             * Installing means pointing the process-global access points
             * (EngineFactory, WindowFactory, InputHandlerFactory, Clock) at
             * this set; removing restores the build's own defaults. There is
             * one renderer's worth of these per process, so the set is a
             * shared singleton: @see Acquire hands out the live one (or
             * creates and installs it), and it uninstalls itself when the
             * last holder lets go. The renderer holds one for as long as it
             * lives.
             *
             * Anything created through the engine (textures, render
             * targets) must be released before the last holder goes - the
             * same rule as for the real backend, whose handles die with its
             * window.
             */
            class NullBackend  {

                public:

                /**
                 * @brief Get the process's null backend, creating and
                 * installing it if none is live.
                 * @param bRealTimePacing Whether the loop is held to the
                 * target FPS in real time. Only honoured when this call
                 * creates the set; an already-live one keeps its own;
                 */
                static std :: shared_ptr<NullBackend> Acquire( bool bRealTimePacing );

                /**
                 * @brief Whether a null backend is currently installed.
                 */
                static bool IsActive( void );

                ~NullBackend( void );

                SunLight :: General :: VirtualClock& GetClock( void );

                private:

                explicit NullBackend( bool bRealTimePacing );

                SunLight :: General :: VirtualClock         m_Clock;
                SunLight :: Engines :: Null :: NullEngine   m_Engine;
                SunLight :: Window :: Null :: NullWindow    m_Window;
            };
        }
    }
}
#endif  /* __NULLBACKEND_H__ */
