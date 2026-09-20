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

#include "window/closehandlerlist.h"
#include <algorithm>


namespace SunLight {
    namespace Window  {

        /**
         * @brief Register a handler.
         * @param handler The handler to call on every Fire();
         * @return An id to pass to Remove() (always > 0);
         */
        int CloseHandlerList :: Add( const Handler &handler )  {

            int  nId = m_nNextId++;

            m_Entries.push_back( Entry { nId, handler } );

            return nId;
        }

        /**
         * @brief Unregister a handler. Unknown/already-removed ids are ignored.
         * @param nId The id Add() returned;
         */
        void CloseHandlerList :: Remove( int nId )  {

            m_Entries.erase( std :: remove_if( m_Entries.begin(), m_Entries.end(),
                                               [nId]( const Entry &entry ) { return entry.nId == nId; } ),
                             m_Entries.end() );
        }

        /**
         * @brief Call every registered handler, in registration order.
         */
        void CloseHandlerList :: Fire( void )  {

            std :: vector<Entry>  snapshot = m_Entries;

            for( const Entry &entry : snapshot )  {
                // Skip a handler an earlier handler in this same Fire()
                // just removed.
                bool  bStillRegistered = std :: any_of( m_Entries.begin(), m_Entries.end(),
                                                        [&entry]( const Entry &live ) { return live.nId == entry.nId; } );

                if( bStillRegistered )
                    entry.handler();
            }
        }
    }
}
