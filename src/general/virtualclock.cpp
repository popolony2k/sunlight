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

#include "general/virtualclock.h"
#include <cmath>


namespace SunLight  {
    namespace General  {

        /**
         * @brief Current virtual time in whole milliseconds (rounded).
         */
        int64_t VirtualClock :: NowMilliseconds( void )  {

            return ( int64_t ) std :: llround( GetSeconds() * 1000.0 );
        }

        /**
         * @brief Change the frame rate, folding elapsed time into the base.
         */
        void VirtualClock :: SetFrameRate( int nFps )  {

            int  nRate = ( nFps > 0 ) ? nFps : 60;

            if( nRate == m_nFps )
                return;

            m_dBaseSeconds     = GetSeconds();
            m_nFramesSinceBase = 0;
            m_nFps             = nRate;
        }

        /**
         * @brief One frame later. Just a count - the division happens once,
         * in GetSeconds().
         */
        void VirtualClock :: AdvanceFrame( void )  {

            m_nFramesSinceBase++;
        }

        /**
         * @brief Advance virtual time by an arbitrary amount (ignores
         * negative values).
         */
        void VirtualClock :: Advance( double dSeconds )  {

            if( dSeconds > 0.0 )
                m_dBaseSeconds += dSeconds;
        }

        /**
         * @brief Current virtual time in seconds: base + frames / fps.
         */
        double VirtualClock :: GetSeconds( void ) const  {

            return m_dBaseSeconds + ( ( double ) m_nFramesSinceBase / ( double ) m_nFps );
        }
    }
}
