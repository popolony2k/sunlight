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

#ifndef __VIRTUALCLOCK_H__
#define __VIRTUALCLOCK_H__

#include "general/clock.h"


namespace SunLight  {
    namespace General  {

        /**
         * @brief A clock that only moves when told to - the frame loop of a
         * windowless backend advances it by one frame's worth of time per
         * frame, so everything reading @see Clock (script waits,
         * animations) sees frames x dt instead of real elapsed time and a
         * long timeline can run as fast as frames can be produced.
         *
         * Time is kept as double seconds; NowMilliseconds() rounds to the
         * nearest millisecond, so accumulating N frames of 1/60 s reads
         * back the intuitive value (e.g. 3 frames = 50 ms, not 49).
         * Monotonic: only ever advances.
         */
        class VirtualClock : public IClock  {

            double  m_dSeconds = 0.0;

            public:

            int64_t NowMilliseconds( void ) override;

            /**
             * @brief Move time forward. Negative values are ignored (the
             * clock never goes backwards).
             * @param dSeconds How far to advance, in seconds;
             */
            void Advance( double dSeconds );

            /**
             * @brief Current virtual time, in seconds.
             */
            double GetSeconds( void ) const;
        };
    }
}

#endif  /* __VIRTUALCLOCK_H__ */
