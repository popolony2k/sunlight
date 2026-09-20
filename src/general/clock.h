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

#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <cstdint>


namespace SunLight  {
    namespace General  {

        /**
         * @brief Source of "now", in milliseconds, for sunlight's own
         * time-based logic - script waits (ScriptProcessor WAIT_CMD),
         * sprite frame animation (TextureMap) and tile animation
         * (TileMapRenderer). The values are only meaningful as
         * differences/comparisons against each other, never as calendar
         * time (the default source is a monotonic steady_clock, whose
         * origin is arbitrary).
         */
        class IClock  {

            public:

            virtual ~IClock( void )  {}

            /**
             * @brief Must be implemented to return the current time, in
             * milliseconds, on this clock. Monotonic (never decreases).
             */
            virtual int64_t NowMilliseconds( void ) = 0;
        };

        /**
         * @brief Process-global access point every time-based piece of
         * sunlight reads "now" through, instead of calling the system
         * clock directly - so a virtual clock (headless/test runs that
         * must not depend on real elapsed time) can be substituted for all
         * of them at once. Process-global rather than per-object on
         * purpose: e.g. TextureMap lives deep inside Sprite, created by
         * callers independently of any renderer, so a clock could not be
         * threaded through their constructors without touching every
         * creation path (and there is one renderer per process anyway).
         *
         * By default it reads a real monotonic clock, so nothing changes
         * unless a clock is installed with @see SetClock. Install one
         * BEFORE creating anything that timestamps against it, and don't
         * swap it while such objects exist: they store absolute times
         * taken from whichever clock was active, which are not comparable
         * across two different clocks.
         *
         * Single-threaded use, like everything that reads it. Note the
         * real-thread Timer (concurrent/Timer) deliberately does not use
         * this: it sleeps on a background thread in real time.
         */
        class Clock  {

            public:

            /**
             * @brief Current time in milliseconds - from the installed
             * clock, or the real monotonic clock if none is installed.
             */
            static int64_t NowMilliseconds( void );

            /**
             * @brief Install a replacement clock, or pass nullptr to
             * restore the real one. Not owned - the caller keeps it alive
             * for as long as it is installed.
             *
             * @param pClock The clock to install, or nullptr;
             */
            static void SetClock( IClock *pClock );
        };
    }
}

#endif  /* __CLOCK_H__ */
