/*
 * timer.h
 *
 *  Created on: May 15, 2024
 *      Author: popolony2k
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