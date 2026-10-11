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

#include "backends/sdl/sdlgamepadhaptics.h"
#include "input/gamepadidentitymatcher.h"
#include "general/clock.h"
#include <SDL.h>
#include <algorithm>

namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            SDLGamepadHaptics :: SDLGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler )
                : m_InputHandler( inputHandler )  {

                ::SDL_InitSubSystem( SDL_INIT_GAMECONTROLLER );
            }

            SDLGamepadHaptics :: ~SDLGamepadHaptics( void )  {

                ::SDL_QuitSubSystem( SDL_INIT_GAMECONTROLLER );
            }

            /**
             * @brief Correlates nGamePadId (raylib/GLFW's own device index) against SDL's own,
             * independently-enumerated device list (see gamepadidentitymatcher.h: exactly one
             * gamepad on each side is always treated as the same physical device - the only
             * cross-library signal strong enough to trust unconditionally, confirmed live
             * against a real Xbox One S controller that raylib/GLFW names "Xbox Wireless
             * Controller" and SDL names "Xbox One S Controller" - two disagreeing mapping
             * databases describing the identical, single connected unit. Name matching is the
             * fallback once there is more than one candidate on either side).
             */
            int SDLGamepadHaptics :: FindMatchingSDLIndex( int nGamePadId ) const  {

                if( !m_InputHandler.IsGamepadAvailable( nGamePadId ) )
                    return -1;

                std :: vector<int>          raylibIndices;
                std :: vector<std :: string> raylibNames;

                for( int nId = 0; nId < MAX_GAMEPAD_COUNT; nId++ )  {
                    if( m_InputHandler.IsGamepadAvailable( nId ) )  {
                        raylibIndices.push_back( nId );
                        raylibNames.push_back( m_InputHandler.GetGamepadName( nId ) );
                    }
                }

                std :: vector<int>          sdlIndices;
                std :: vector<std :: string> sdlNames;
                int                          nNumJoysticks = ::SDL_NumJoysticks();

                for( int nIndex = 0; nIndex < nNumJoysticks; nIndex++ )  {
                    if( ::SDL_IsGameController( nIndex ) )  {
                        const char  *szName = ::SDL_GameControllerNameForIndex( nIndex );

                        sdlIndices.push_back( nIndex );
                        sdlNames.push_back( ( szName != nullptr ) ? szName : "" );
                    }
                }

                std :: vector<int>  matches = SunLight :: Input :: MatchGamepadsByName( raylibNames, sdlNames );

                for( size_t nPos = 0; nPos < raylibIndices.size(); nPos++ )  {
                    if( raylibIndices[ nPos ] == nGamePadId )
                        return ( matches[ nPos ] >= 0 ) ? sdlIndices[ matches[ nPos ] ] : -1;
                }

                return -1;
            }

            bool SDLGamepadHaptics :: IsSupported( int nGamePadId ) const  {

                int  nSdlIndex = FindMatchingSDLIndex( nGamePadId );

                if( nSdlIndex < 0 )
                    return false;

                SDL_GameController  *pController = ::SDL_GameControllerOpen( nSdlIndex );

                if( pController == nullptr )
                    return false;

                bool  bSupported = ::SDL_GameControllerHasRumble( pController ) == SDL_TRUE;

                ::SDL_GameControllerClose( pController );

                return bSupported;
            }

            void SDLGamepadHaptics :: SetVibration( int nGamePadId, float fLeftMotor, float fRightMotor, float fDuration )  {

                int  nSdlIndex = FindMatchingSDLIndex( nGamePadId );

                if( nSdlIndex < 0 )
                    return;

                SDL_GameController  *pController = ::SDL_GameControllerOpen( nSdlIndex );

                if( pController == nullptr )
                    return;

                float  fClampedLeft     = std :: clamp( fLeftMotor, 0.0f, 1.0f );
                float  fClampedRight    = std :: clamp( fRightMotor, 0.0f, 1.0f );
                float  fClampedDuration = std :: max( fDuration, 0.0f );

                ::SDL_GameControllerRumble( pController,
                                           ( Uint16 ) ( fClampedLeft * 65535.0f ),
                                           ( Uint16 ) ( fClampedRight * 65535.0f ),
                                           ( Uint32 ) ( fClampedDuration * 1000.0f ) );

                ::SDL_GameControllerClose( pController );

                // SDL's own duration-based auto-stop needs SDL_JoystickUpdate(), which nothing
                // else in this engine ever calls (see this class's own doc comment) - track the
                // deadline ourselves and enforce it from Update() instead. A duration of 0
                // means "stay on until Stop() is called explicitly" - no deadline to track.
                if( fClampedDuration > 0.0f )
                    m_VibrationDeadlines[ nGamePadId ] = SunLight :: General :: Clock :: NowMilliseconds() + ( int64_t ) ( fClampedDuration * 1000.0f );
                else
                    m_VibrationDeadlines.erase( nGamePadId );
            }

            void SDLGamepadHaptics :: Stop( int nGamePadId )  {

                m_VibrationDeadlines.erase( nGamePadId );

                int  nSdlIndex = FindMatchingSDLIndex( nGamePadId );

                if( nSdlIndex < 0 )
                    return;

                SDL_GameController  *pController = ::SDL_GameControllerOpen( nSdlIndex );

                if( pController == nullptr )
                    return;

                // Zero strengths stop an in-progress rumble immediately - verified against
                // SDL's own source (SDL_joystick.c calls exactly this, with these exact
                // arguments, to silence a controller's motors on close).
                ::SDL_GameControllerRumble( pController, 0, 0, 0 );

                ::SDL_GameControllerClose( pController );
            }

            /**
             * @brief Calls the real Stop() for every pad whose own tracked deadline has passed -
             * sunlight's own replacement for SDL's internal, never-ticked rumble expiration (see
             * this class's own doc comment).
             */
            void SDLGamepadHaptics :: Update( void )  {

                int64_t  nNow = SunLight :: General :: Clock :: NowMilliseconds();
                std :: vector<int>  expiredIds;

                for( const std :: pair<const int, int64_t>& entry : m_VibrationDeadlines )  {
                    if( nNow >= entry.second )
                        expiredIds.push_back( entry.first );
                }

                for( int nGamePadId : expiredIds )
                    Stop( nGamePadId );
            }
        }
    }
}
