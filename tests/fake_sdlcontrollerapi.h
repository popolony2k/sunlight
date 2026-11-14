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

#ifndef __FAKE_SDLCONTROLLERAPI_H__
#define __FAKE_SDLCONTROLLERAPI_H__

#include <doctest/doctest.h>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "backends/sdl/isdlcontrollerapi.h"

/*
 * A fake ISDLControllerApi for the SDL backend's tests (SDLGamepadHaptics, SDLGamepadDevices,
 * SDLGamepadInput): no SDL device is ever opened, so these tests cannot reach a real controller,
 * whatever is plugged in. Each device's buttons and axes are set by the test; GetButton/GetAxis
 * read them through the handle the device was opened with.
 */
namespace FakeSDL  {

    // What the fake saw. It outlives the haptics object, so the destructor's closes can be checked.
    struct FakeLog  {
        struct RumbleCall  {
            int32_t   nInstanceId;
            uint16_t  nLow;
            uint16_t  nHigh;
            uint32_t  nDurationMs;
        };

        int                          nOpens  = 0;
        int                          nCloses = 0;
        std :: vector<RumbleCall>    rumbles;
    };

    // One SDL-side device, at its index in FakeControllerApi::devices.
    struct FakeDevice  {
        std :: string                                                    strName;
        int32_t                                                          nInstanceId;
        bool                                                             bAttached;
        bool                                                             bHasRumble;
        bool                                                             bIsGameController = true;
        std :: set<SunLight :: Input :: SDL :: ISDLControllerApi :: Button>  buttons {};          // held
        std :: array<int16_t, 6>                                         axes {};             // SDL's raw values
    };

    class FakeControllerApi : public SunLight :: Input :: SDL :: ISDLControllerApi  {

        FakeLog                             &m_Log;
        std :: map<void*, int32_t>          m_Open;          // each open handle and its device's instance id

        const FakeDevice* Find( int32_t nInstanceId ) const  {
            for( const FakeDevice& device : devices )
                if( device.nInstanceId == nInstanceId )
                    return &device;

            return nullptr;
        }

        public:

        std :: vector<FakeDevice>  devices;
        bool                       bOpenFails = false;
        bool                       bReady     = true;
        int                        nUpdates   = 0;
        std :: function<void( void )>  onOpen;          // optional: runs inside each Open (eg. blocks, as a slow pad's set-up does)
        std :: function<void( void )>  onRumble;        // optional: runs inside each Rumble

        explicit FakeControllerApi( FakeLog& log ) : m_Log( log )  {}

        ~FakeControllerApi( void ) override  {
            CHECK( m_Open.empty() );                     // every controller closed before the API goes

            for( std :: pair<void* const, int32_t>& entry : m_Open )
                delete static_cast<int*>( entry.first );
        }

        bool IsReady( void ) override                          { return bReady; }
        void Update( void ) override                           { nUpdates++; }
        int NumJoysticks( void ) override                      { return ( int ) devices.size(); }
        bool IsGameController( int nIndex ) override           { return devices[ nIndex ].bIsGameController; }
        const char* NameForIndex( int nIndex ) override        { return devices[ nIndex ].strName.c_str(); }
        int32_t InstanceIdForIndex( int nIndex ) override      { return devices[ nIndex ].nInstanceId; }

        ControllerHandle Open( int nIndex ) override  {
            if( onOpen )
                onOpen();

            if( bOpenFails )
                return nullptr;

            void  *hController = new int( 0 );        // a distinct handle per open

            m_Open[ hController ] = devices[ nIndex ].nInstanceId;
            m_Log.nOpens++;

            return hController;
        }

        void Close( ControllerHandle hController ) override  {
            REQUIRE( m_Open.count( hController ) == 1 );
            m_Open.erase( hController );
            delete static_cast<int*>( hController );
            m_Log.nCloses++;
        }

        bool IsAttached( ControllerHandle hController ) override  {
            const FakeDevice  *pDevice = Find( m_Open.at( hController ) );

            return ( pDevice != nullptr ) && pDevice -> bAttached;
        }

        int32_t InstanceId( ControllerHandle hController ) override  {
            return m_Open.at( hController );
        }

        bool HasRumble( ControllerHandle hController ) override  {
            const FakeDevice  *pDevice = Find( m_Open.at( hController ) );

            return ( pDevice != nullptr ) && pDevice -> bHasRumble;
        }

        void Rumble( ControllerHandle hController, uint16_t nLow, uint16_t nHigh, uint32_t nDurationMs ) override  {
            m_Log.rumbles.push_back( FakeLog :: RumbleCall { m_Open.at( hController ), nLow, nHigh, nDurationMs } );

            if( onRumble )
                onRumble();
        }

        bool GetButton( ControllerHandle hController, Button button ) override  {
            const FakeDevice  *pDevice = Find( m_Open.at( hController ) );

            return ( pDevice != nullptr ) && ( pDevice -> buttons.count( button ) > 0 );
        }

        int16_t GetAxis( ControllerHandle hController, Axis axis ) override  {
            const FakeDevice  *pDevice = Find( m_Open.at( hController ) );

            return ( pDevice != nullptr ) ? pDevice -> axes[ ( size_t ) axis ] : 0;
        }

        // The device with this instance id, for a test to change it.
        FakeDevice& Device( int32_t nInstanceId )  {
            for( FakeDevice& device : devices )
                if( device.nInstanceId == nInstanceId )
                    return device;

            REQUIRE( false );
            return devices.front();
        }
    };
}

#endif /* __FAKE_SDLCONTROLLERAPI_H__ */
