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
#include "base/object.h"


namespace SunLight {
    namespace Concurrent  {

        /**
         * @brief Timer class impleentation for handling interval event
         * handling.
         */
        class Timer : public SunLight :: Base :: Object {

            private:
            
            std :: thread       m_Thread;
            bool                m_bRunning;
            int                 m_nTimerId;


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