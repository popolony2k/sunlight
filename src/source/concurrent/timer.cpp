/*
 * timer.h
 *
 *  Created on: May 15, 2024
 *      Author: popolony2k
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