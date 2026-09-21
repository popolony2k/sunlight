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
         * windowless backend advances it by one frame per frame, so
         * everything reading @see Clock (script waits, animations) sees
         * frames x dt instead of real elapsed time and a long timeline can
         * run as fast as frames can be produced.
         *
         * Frames are COUNTED, never summed: time is
         *   base + framesSinceBase / fps
         * - one division, however many frames have run. Adding 1/60 to a
         * running double instead drifts (300 frames summed to
         * 4.999999999999988, 5040 to 83.99999999999652 - missing the
         * whole-second boundary a script waiting for "elapsed >= 84" sees,
         * costing it an extra frame), while a single division of an exact
         * quotient is exact in IEEE arithmetic: 300 / 60 is exactly 5.0 and
         * 5040 / 60 exactly 84.0. When the frame rate changes, the time so
         * far is folded into the base and counting restarts at the new rate.
         *
         * NowMilliseconds() rounds to the nearest millisecond (3 frames of
         * 1/60 s read 50 ms, not 49). Monotonic: only ever advances.
         */
        class VirtualClock : public IClock  {

            double   m_dBaseSeconds       = 0.0;
            int64_t  m_nFramesSinceBase   = 0;
            int      m_nFps               = 60;

            public:

            int64_t NowMilliseconds( void ) override;

            /**
             * @brief Set the frame rate the following AdvanceFrame() calls
             * count at (frames per second). A non-positive rate falls back
             * to 60. Changing the rate folds the time so far into the base,
             * so nothing already elapsed is re-scaled; setting the same
             * rate again is a no-op.
             * @param nFps Frames per second;
             */
            void SetFrameRate( int nFps );

            /**
             * @brief Advance time by exactly one frame at the current rate.
             */
            void AdvanceFrame( void );

            /**
             * @brief Move time forward by an arbitrary amount (folded into
             * the base). Negative values are ignored (the clock never goes
             * backwards). Prefer SetFrameRate + AdvanceFrame for frame
             * loops: repeated Advance() of a fractional step accumulates
             * rounding error.
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
