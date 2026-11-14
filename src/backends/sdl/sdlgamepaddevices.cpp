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

#include "backends/sdl/sdlgamepaddevices.h"
#include "backends/sdl/sdlcontrollerapi.h"


#define __NO_FREE_PAD                  -1
#define __EMPTY_NAME                   ""

namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            static std :: weak_ptr<SDLGamepadDevices>                s_pCurrent;
            static SDLGamepadDevices :: ApiCreatorFunction           s_pApiCreator = nullptr;

            /**
             * @brief Takes pApi, or the real SDL API when it is nullptr. Opens nothing: the first
             * Update() does.
             * @param pApi The API to use (the tests' fake), or nullptr;
             */
            SDLGamepadDevices :: SDLGamepadDevices( std :: unique_ptr<ISDLControllerApi> pApi )
                : m_pApi( pApi ? std :: move( pApi ) : std :: make_unique<SDLControllerApi>() )  {
            }

            /**
             * @brief Closes every controller still open, before m_pApi goes (the real one releases
             * SDL's subsystem then).
             */
            SDLGamepadDevices :: ~SDLGamepadDevices( void )  {

                for( stPad& pad : m_Pads )  {
                    if( pad.hController != nullptr )  {
                        m_pApi -> Close( pad.hController );
                        pad.hController = nullptr;
                    }
                }
            }

            /**
             * @brief The live process-wide set on the real SDL API, or a new one.
             */
            std :: shared_ptr<SDLGamepadDevices> SDLGamepadDevices :: Acquire( void )  {

                std :: shared_ptr<SDLGamepadDevices>  pDevices = s_pCurrent.lock();

                if( !pDevices )  {
                    pDevices   = std :: make_shared<SDLGamepadDevices>( s_pApiCreator ? s_pApiCreator() : nullptr );
                    s_pCurrent = pDevices;
                }

                return pDevices;
            }

            /**
             * @brief The live process-wide set if SDL started for it, nullptr otherwise.
             */
            std :: shared_ptr<SDLGamepadDevices> SDLGamepadDevices :: Current( void )  {

                std :: shared_ptr<SDLGamepadDevices>  pDevices = s_pCurrent.lock();

                return ( pDevices && pDevices -> IsReady() ) ? pDevices : nullptr;
            }

            /**
             * @brief Sets the API Acquire() makes a new set on (tests only).
             * @param creator The replacement creator function, or an empty std::function;
             */
            void SDLGamepadDevices :: SetApiCreator( ApiCreatorFunction creator )  {

                s_pApiCreator = creator;
            }

            /**
             * @brief Whether SDL's game-controller subsystem started.
             */
            bool SDLGamepadDevices :: IsReady( void ) const  {

                return m_pApi -> IsReady();
            }

            /**
             * @brief Whether a gamepad id already holds the device with this SDL instance id.
             * @param nInstanceId The SDL instance id to look for;
             */
            bool SDLGamepadDevices :: HasInstance( int32_t nInstanceId ) const  {

                for( const stPad& pad : m_Pads )  {
                    if( ( pad.hController != nullptr ) && ( pad.nInstanceId == nInstanceId ) )
                        return true;
                }

                return false;
            }

            /**
             * @brief The lowest gamepad id with no controller, or __NO_FREE_PAD.
             */
            int SDLGamepadDevices :: FreePad( void ) const  {

                for( int nGamePadId = 0; nGamePadId < MAX_GAMEPAD_COUNT; nGamePadId++ )  {
                    if( m_Pads[ nGamePadId ].hController == nullptr )
                        return nGamePadId;
                }

                return __NO_FREE_PAD;
            }

            /**
             * @brief The per-frame step (see the class's doc comment). Closing comes before
             * opening, so a pad unplugged and plugged back in within one frame gets its id back
             * when it is the lowest free one. A device SDL lists but cannot open is remembered
             * until it leaves the list, so a failing open is not paid again every frame.
             */
            void SDLGamepadDevices :: Update( void )  {

                if( !m_pApi -> IsReady() )
                    return;

                m_pApi -> Update();

                for( stPad& pad : m_Pads )  {
                    if( ( pad.hController != nullptr ) && !m_pApi -> IsAttached( pad.hController ) )  {
                        m_pApi -> Close( pad.hController );
                        pad = stPad();
                    }
                }

                std :: set<int32_t>  listed;
                int                  nNumJoysticks = m_pApi -> NumJoysticks();

                for( int nIndex = 0; nIndex < nNumJoysticks; nIndex++ )  {
                    if( !m_pApi -> IsGameController( nIndex ) )
                        continue;

                    int32_t  nInstanceId = m_pApi -> InstanceIdForIndex( nIndex );

                    listed.insert( nInstanceId );

                    if( HasInstance( nInstanceId ) || ( m_FailedInstances.count( nInstanceId ) > 0 ) )
                        continue;

                    int  nGamePadId = FreePad();

                    if( nGamePadId == __NO_FREE_PAD )
                        continue;

                    ISDLControllerApi :: ControllerHandle  hController = m_pApi -> Open( nIndex );

                    if( hController == nullptr )  {
                        m_FailedInstances.insert( nInstanceId );
                        continue;
                    }

                    const char  *szName = m_pApi -> NameForIndex( nIndex );

                    m_Pads[ nGamePadId ].hController = hController;
                    m_Pads[ nGamePadId ].nInstanceId = m_pApi -> InstanceId( hController );
                    m_Pads[ nGamePadId ].strName     = ( szName != nullptr ) ? szName : __EMPTY_NAME;
                }

                for( std :: set<int32_t> :: iterator itItem = m_FailedInstances.begin(); itItem != m_FailedInstances.end(); )  {
                    if( listed.count( *itItem ) == 0 )
                        itItem = m_FailedInstances.erase( itItem );
                    else
                        ++itItem;
                }
            }

            /**
             * @brief Whether nGamePadId has an open controller.
             * @param nGamePadId The gamepad id;
             */
            bool SDLGamepadDevices :: IsAvailable( int nGamePadId ) const  {

                return Controller( nGamePadId ) != nullptr;
            }

            /**
             * @brief nGamePadId's open controller, nullptr if it has none or is out of range.
             * @param nGamePadId The gamepad id;
             */
            ISDLControllerApi :: ControllerHandle SDLGamepadDevices :: Controller( int nGamePadId ) const  {

                if( ( nGamePadId < 0 ) || ( nGamePadId >= MAX_GAMEPAD_COUNT ) )
                    return nullptr;

                return m_Pads[ nGamePadId ].hController;
            }

            /**
             * @brief nGamePadId's name as SDL gave it when it was opened, "" if it has none - never
             * nullptr, as raylib's own name array never is.
             * @param nGamePadId The gamepad id;
             */
            const char* SDLGamepadDevices :: Name( int nGamePadId ) const  {

                if( !IsAvailable( nGamePadId ) )
                    return __EMPTY_NAME;

                return m_Pads[ nGamePadId ].strName.c_str();
            }

            /**
             * @brief The SDL API the controllers are open on, for the calls made on them.
             */
            ISDLControllerApi& SDLGamepadDevices :: Api( void ) const  {

                return *m_pApi;
            }
        }
    }
}
