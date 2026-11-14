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
#include <chrono>


#define __NO_FREE_PAD                  -1
#define __EMPTY_NAME                   ""
#define __POLL_INTERVAL_MS             1          // the gamepad thread's pause between passes

namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            static std :: weak_ptr<SDLGamepadDevices>                s_pCurrent;
            static SDLGamepadDevices :: ApiCreatorFunction           s_pApiCreator = nullptr;
            static SDLGamepadDevices :: RunMode                      s_ApiCreatorMode = SDLGamepadDevices :: RUN_MODE_MANUAL;

            /**
             * @brief Keeps the creator and starts: the thread (which makes the API there), or, by
             * hand, the API right here.
             * @param creator Makes the API, or nullptr for the real one;
             * @param mode Who makes the SDL calls;
             */
            SDLGamepadDevices :: SDLGamepadDevices( ApiCreatorFunction creator, RunMode mode )
                : m_ApiCreator( creator ? creator : []( void ) -> std :: unique_ptr<ISDLControllerApi>  { return std :: make_unique<SDLControllerApi>(); } ),
                  m_nState( STATE_STARTING ),
                  m_bStop( false )  {

                if( mode == RUN_MODE_THREAD )
                    m_Thread = std :: thread( &SDLGamepadDevices :: Run, this );
                else
                    Start();
            }

            /**
             * @brief Stops and joins the thread, which shuts SDL down on itself; by hand, shuts it
             * down here.
             */
            SDLGamepadDevices :: ~SDLGamepadDevices( void )  {

                m_bStop = true;

                if( m_Thread.joinable() )
                    m_Thread.join();
                else
                    Shutdown();
            }

            /**
             * @brief Makes the API - the real one starts SDL, on the stepping thread - and records
             * whether SDL started.
             */
            void SDLGamepadDevices :: Start( void )  {

                m_pApi    = m_ApiCreator();
                m_nState  = m_pApi -> IsReady() ? STATE_READY : STATE_FAILED;
            }

            /**
             * @brief Closes every controller still open, then lets the API go (the real one stops
             * SDL's subsystem) - on the stepping thread, which started them.
             */
            void SDLGamepadDevices :: Shutdown( void )  {

                if( !m_pApi )
                    return;

                for( stPad& pad : m_Pads )  {
                    if( pad.hController != nullptr )  {
                        m_pApi -> Close( pad.hController );
                        pad = stPad();
                    }
                }

                m_pApi.reset();
            }

            /**
             * @brief The gamepad thread: starts SDL, steps until stopped (or until SDL failed), and
             * shuts SDL down - every SDL call on this one thread.
             */
            void SDLGamepadDevices :: Run( void )  {

                Start();

                while( !m_bStop && Step() )
                    std :: this_thread :: sleep_for( std :: chrono :: milliseconds( __POLL_INTERVAL_MS ) );

                Shutdown();
            }

            /**
             * @brief The live process-wide set, or a new one on its own thread (on a fake, stepped
             * by hand, while a test's SetApiCreator is in force).
             */
            std :: shared_ptr<SDLGamepadDevices> SDLGamepadDevices :: Acquire( void )  {

                std :: shared_ptr<SDLGamepadDevices>  pDevices = s_pCurrent.lock();

                if( !pDevices )  {
                    pDevices   = s_pApiCreator ? std :: make_shared<SDLGamepadDevices>( s_pApiCreator, s_ApiCreatorMode )
                                               : std :: make_shared<SDLGamepadDevices>( nullptr, RUN_MODE_THREAD );
                    s_pCurrent = pDevices;
                }

                return pDevices;
            }

            /**
             * @brief The live process-wide set unless SDL failed to start for it, nullptr otherwise.
             */
            std :: shared_ptr<SDLGamepadDevices> SDLGamepadDevices :: Current( void )  {

                std :: shared_ptr<SDLGamepadDevices>  pDevices = s_pCurrent.lock();

                return ( pDevices && ( pDevices -> GetState() != STATE_FAILED ) ) ? pDevices : nullptr;
            }

            /**
             * @brief Sets the API, and the run mode, Acquire() makes a new set with (tests only).
             * @param creator The replacement API creator, or an empty std::function;
             * @param mode The run mode for the sets Acquire() makes with it;
             */
            void SDLGamepadDevices :: SetApiCreator( ApiCreatorFunction creator, RunMode mode )  {

                s_pApiCreator    = creator;
                s_ApiCreatorMode = mode;
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
             * @brief One pass (see the class's doc comment). Closing comes before opening, so a pad
             * unplugged and plugged back in within one pass gets its id back when it is the lowest
             * free one. The rumble requests are taken under the lock but carried out without it,
             * and so is every other SDL call: the lock is held only to swap the requests out and to
             * publish the snapshot, so the game's side never waits on SDL.
             */
            bool SDLGamepadDevices :: Step( void )  {

                if( m_nState != STATE_READY )
                    return false;

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

                std :: deque<stRumbleRequest>  requests;

                {
                    std :: lock_guard<std :: mutex>  lock( m_Mutex );

                    requests.swap( m_Requests );
                }

                for( const stRumbleRequest& request : requests )  {
                    ISDLControllerApi :: ControllerHandle  hController = m_Pads[ request.nGamePadId ].hController;

                    if( hController != nullptr )
                        m_pApi -> Rumble( hController, request.nLowFrequency, request.nHighFrequency, request.nDurationMs );
                }

                Snapshot  snapshot;

                for( int nGamePadId = 0; nGamePadId < MAX_GAMEPAD_COUNT; nGamePadId++ )  {
                    const stPad  &pad   = m_Pads[ nGamePadId ];
                    stPadState   &state = snapshot[ nGamePadId ];

                    if( pad.hController == nullptr )
                        continue;

                    state.bAvailable = true;
                    state.bHasRumble = m_pApi -> HasRumble( pad.hController );
                    state.strName    = pad.strName;

                    for( int nButton = 0; nButton < SDL_GAMEPAD_BUTTON_COUNT; nButton++ )
                        state.buttons[ nButton ] = m_pApi -> GetButton( pad.hController, ( ISDLControllerApi :: Button ) nButton );

                    for( int nAxis = 0; nAxis < SDL_GAMEPAD_AXIS_COUNT; nAxis++ )
                        state.axes[ nAxis ] = m_pApi -> GetAxis( pad.hController, ( ISDLControllerApi :: Axis ) nAxis );
                }

                {
                    std :: lock_guard<std :: mutex>  lock( m_Mutex );

                    m_Published.swap( snapshot );
                }

                return true;
            }

            /**
             * @brief Whether SDL is starting, started, or failed to start.
             */
            SDLGamepadDevices :: State SDLGamepadDevices :: GetState( void ) const  {

                return ( State ) m_nState.load();
            }

            /**
             * @brief Copies the latest snapshot.
             * @param snapshot Receives every pad's state;
             */
            void SDLGamepadDevices :: Read( Snapshot& snapshot ) const  {

                std :: lock_guard<std :: mutex>  lock( m_Mutex );

                snapshot = m_Published;
            }

            /**
             * @brief Whether a pad is there and can rumble, from the latest snapshot - false for an
             * id out of range.
             * @param nGamePadId The gamepad id;
             * @param bAvailable Receives whether it is there;
             * @param bHasRumble Receives whether it can rumble;
             */
            void SDLGamepadDevices :: PadInfo( int nGamePadId, bool& bAvailable, bool& bHasRumble ) const  {

                bAvailable = false;
                bHasRumble = false;

                if( ( nGamePadId < 0 ) || ( nGamePadId >= MAX_GAMEPAD_COUNT ) )
                    return;

                std :: lock_guard<std :: mutex>  lock( m_Mutex );

                bAvailable = m_Published[ nGamePadId ].bAvailable;
                bHasRumble = m_Published[ nGamePadId ].bHasRumble;
            }

            /**
             * @brief Queues a rumble for the next pass (SDL_GameControllerRumble's arguments). An id
             * out of range is ignored; a pad gone by then is skipped.
             * @param nGamePadId The gamepad id;
             * @param nLowFrequency The low-frequency motor, 0..65535;
             * @param nHighFrequency The high-frequency motor, 0..65535;
             * @param nDurationMs How long, in milliseconds (0: no end);
             */
            void SDLGamepadDevices :: Rumble( int nGamePadId, uint16_t nLowFrequency, uint16_t nHighFrequency, uint32_t nDurationMs )  {

                if( ( nGamePadId < 0 ) || ( nGamePadId >= MAX_GAMEPAD_COUNT ) )
                    return;

                std :: lock_guard<std :: mutex>  lock( m_Mutex );

                m_Requests.push_back( stRumbleRequest { nGamePadId, nLowFrequency, nHighFrequency, nDurationMs } );
            }
        }
    }
}
