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

#include "concurrent/timer.h"


namespace SunLight {
    namespace Concurrent  {

        /**
         * @brief Constructor. Initialize all class data;
         * @param nTimerId The TImer Id to assign to event hndler parameter;
         * 
         * @return Timer 
         */
        Timer :: Timer( int nTimerId )  {
            m_bRunning = false;
            m_nTimerId = nTimerId;
        }

        /**
         * @brief Destructor. Finalize all class data.
         * 
         * @return Timer 
         */
        Timer :: ~Timer( void )  {
            Stop();
        }

        /**
         * @brief Start a timer fucntion based on an interval passed by parameter;
         * 
         * @param nInterval The interval to call the routine;
         * @param handler The handler function that will be called when interval is
         * reached;
         * @return true if operation was successfull;
         * @return false if operation has failed;
         */
        bool Timer :: Start( const Interval &nInterval, const SLEEP_TIMER_HANDLER &handler )  {
            if( !m_bRunning )  {
                m_bRunning = true;
                m_Thread   = std :: thread( [this, nInterval, handler] {
                    while( m_bRunning ) {
                        std :: this_thread :: sleep_for( nInterval );
                        handler( m_nTimerId );
                    }
                } );

                return true;
            }

            return false;
        }

        /**
         * @brief Stop a previously started timer;
         * 
         * @return true if operation was successfull;
         * @return false if operation has failed;
         */
        bool Timer :: Stop( void )  {

            if( m_bRunning )  {
                m_bRunning = false;
                m_Thread.join();

                return true;               
            }

            return false;
        }
    }
}