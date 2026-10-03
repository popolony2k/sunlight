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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <thread>
#include <chrono>
#include <functional>
#include <cstdio>
#include <atomic>
#include "base/object.h"


namespace SunLight {
    namespace Concurrent  {

        /**
         * @brief Calls a handler every interval, on a background thread, in REAL time.
         *
         * Read this before using it. The handler does NOT run on the thread that drives
         * the renderer or the game:
         *  - It runs on the timer's own thread, after each real-time sleep of the interval.
         *    Nothing is synchronized with the frame loop, and the timeline is the wall
         *    clock, not the injectable virtual clock (SunLight::General::Clock). Under a
         *    headless run that is faster or slower than real time, the ticks land at
         *    different points of the game than they would in a real window.
         *  - A tick is skipped whenever the owner of the handler is busy. The Lua binding
         *    (Scarab's timer_set) takes its lock with try_lock, so a tick that finds the
         *    main thread inside Lua does nothing and is not retried.
         *  - The handler must not touch engine, renderer or script state directly. Any
         *    state it reads or writes must be protected by the same lock the main thread
         *    uses, and it must not block on that lock, or Stop() can deadlock (Stop joins
         *    this thread).
         *  - Stop() joins the thread, so it must not be called from inside the handler.
         *
         * Use it for independent, real-time work that does not change game state (for
         * example a timed log or a watchdog). Anything that must happen at a given point
         * of the game belongs in the frame loop, driven by the game clock.
         */
        class Timer : public SunLight :: Base :: Object {

            private:

            std :: thread        m_Thread;
            std :: atomic<bool>  m_bRunning;
            int                  m_nTimerId;


            public:

            typedef std :: chrono :: milliseconds Interval;
            typedef std :: function<void( int )> SLEEP_TIMER_HANDLER;


            Timer( int nTimerId );
            ~Timer( void );
            
            bool Start( const Interval &nInterval, const SLEEP_TIMER_HANDLER &handler );
            bool Stop( void );
        };
    }
}

#endif  /* __TIMER_H__ */