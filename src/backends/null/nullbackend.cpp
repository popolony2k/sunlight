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

#include "backends/null/nullbackend.h"
#include "backends/null/nullinputhandler.h"
#include "engines/enginefactory.h"
#include "window/windowfactory.h"
#include "input/inputhandlerfactory.h"
#include "general/clock.h"

namespace SunLight  {
    namespace Backends  {
        namespace Null  {

            static std :: weak_ptr<NullBackend>  s_pLive;

            /**
             * @brief Install the whole set as the process-global backend.
             */
            NullBackend :: NullBackend( bool bRealTimePacing ) :
                                        m_Window( m_Clock, bRealTimePacing )  {

                SunLight :: Engines :: EngineFactory :: SetEngine( &m_Engine );
                SunLight :: Window :: WindowFactory :: SetWindow( &m_Window );
                SunLight :: General :: Clock :: SetClock( &m_Clock );
                SunLight :: Input :: InputHandlerFactory :: SetCreator( []( void ) -> std :: unique_ptr<SunLight :: Input :: IInputHandler> {
                    return std :: make_unique<SunLight :: Input :: Null :: NullInputHandler>();
                } );
            }

            /**
             * @brief Restore the build's own defaults.
             */
            NullBackend :: ~NullBackend( void )  {

                SunLight :: Engines :: EngineFactory :: SetEngine( nullptr );
                SunLight :: Window :: WindowFactory :: SetWindow( nullptr );
                SunLight :: General :: Clock :: SetClock( nullptr );
                SunLight :: Input :: InputHandlerFactory :: SetCreator( nullptr );
            }

            std :: shared_ptr<NullBackend> NullBackend :: Acquire( bool bRealTimePacing )  {

                std :: shared_ptr<NullBackend>  pLive = s_pLive.lock();

                if( !pLive )  {
                    // Private constructor, so std::make_shared can't be used.
                    pLive = std :: shared_ptr<NullBackend>( new NullBackend( bRealTimePacing ) );
                    s_pLive = pLive;
                }

                return pLive;
            }

            bool NullBackend :: IsActive( void )  {

                return !s_pLive.expired();
            }

            SunLight :: General :: VirtualClock& NullBackend :: GetClock( void )  {

                return m_Clock;
            }
        }
    }
}
