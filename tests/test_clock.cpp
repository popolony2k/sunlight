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

/*
 * The process-global clock source (SunLight::General::Clock) and the three
 * things in sunlight that read "now" through it: ScriptProcessor's WAIT_CMD,
 * TextureMap's sprite frame timing. (Tile animation in TileMapRenderer reads
 * the same source, but needs a real animated .tmx to drive, so it has no
 * direct test here.)
 */

#include <doctest/doctest.h>
#include "general/clock.h"
#include "scripting/scriptprocessor.h"
#include "sprite/texturemap.h"
#include "canvas/texturecanvas.h"
#include "mock_clock.h"
#include <vector>
#include <utility>

using SunLight :: General :: Clock;

TEST_SUITE( "general/Clock" )  {

    TEST_CASE( "The default clock is real and never goes backwards" )  {

        int64_t  nFirst  = Clock :: NowMilliseconds();
        int64_t  nSecond = Clock :: NowMilliseconds();

        CHECK( nSecond >= nFirst );
    }

    TEST_CASE( "An installed clock is what NowMilliseconds reads, and nullptr restores the real one" )  {

        MockClock  mock;

        mock.nNow = 123456789012345LL;   // far from any real steady_clock value

        Clock :: SetClock( &mock );
        CHECK( Clock :: NowMilliseconds() == 123456789012345LL );

        mock.Advance( 5 );
        CHECK( Clock :: NowMilliseconds() == 123456789012350LL );

        Clock :: SetClock( nullptr );
        CHECK( Clock :: NowMilliseconds() != 123456789012350LL );
    }
}

namespace  {

    class CommandRecorder : public SunLight :: Scripting :: IScriptListener  {

        public:

        std :: vector<std :: pair<SunLight :: Scripting :: Commands, uint16_t>>  calls;

        void OnCommand( SunLight :: Scripting :: Commands cmd, uint16_t nEventId )  {
            calls.push_back( { cmd, nEventId } );
        }

        void OnError( std :: string )  {}
    };

    // Run() up to nTimes times, stopping early once the queue is drained.
    void RunSome( SunLight :: Scripting :: ScriptProcessor &sp, int nTimes )  {

        for( int nCount = 0; ( nCount < nTimes ) && sp.Run(); nCount++ )  {}
    }
}

TEST_SUITE( "scripting/ScriptProcessor on the injectable clock" )  {

    TEST_CASE( "WAIT_CMD holds the queue until exactly the requested milliseconds have passed on the clock" )  {

        using namespace SunLight :: Scripting;

        MockClockFixture  clockFixture;
        CommandRecorder   listener;
        ScriptProcessor   sp;

        clockFixture.clock.nNow = 5000;

        sp.AddOneParmCmd( WAIT_CMD, 1000 );
        sp.AddOneParmCmd( PLAY_SONG_CMD, 42 );
        sp.AddScriptListener( &listener );
        sp.Compile();

        // No time passes: however often it runs, the command behind the
        // wait never dispatches.
        RunSome( sp, 10 );
        CHECK( listener.calls.empty() );

        clockFixture.clock.Advance( 999 );
        RunSome( sp, 10 );
        CHECK( listener.calls.empty() );

        // The 1000th millisecond releases the wait.
        clockFixture.clock.Advance( 1 );
        RunSome( sp, 10 );
        REQUIRE( listener.calls.size() == 1 );
        CHECK( listener.calls[0].first == PLAY_SONG_CMD );
        CHECK( listener.calls[0].second == 42 );
    }

    TEST_CASE( "A long WAIT_CMD costs no real time - the clock alone decides" )  {

        using namespace SunLight :: Scripting;

        MockClockFixture  clockFixture;
        CommandRecorder   listener;
        ScriptProcessor   sp;

        sp.AddOneParmCmd( WAIT_CMD, 60000 );   // a full minute
        sp.AddOneParmCmd( PLAY_SONG_CMD, 1 );
        sp.AddScriptListener( &listener );
        sp.Compile();

        RunSome( sp, 3 );
        CHECK( listener.calls.empty() );

        clockFixture.clock.Advance( 60000 );
        RunSome( sp, 3 );
        CHECK( listener.calls.size() == 1 );
    }
}

TEST_SUITE( "sprite/TextureMap on the injectable clock" )  {

    TEST_CASE( "Next advances to the following frame only once that frame's delay has elapsed, then wraps" )  {

        using namespace SunLight :: Sprite;
        using namespace SunLight :: Canvas;

        MockClockFixture  clockFixture;
        TextureMap        map;
        TextureCanvas     frameA;
        TextureCanvas     frameB;

        clockFixture.clock.nNow = 1000;

        map.AddTexture( &frameA, 100 );
        map.AddTexture( &frameB, 100 );

        CHECK( map.GetTextureData().pTexture == &frameA );

        // Frame A was stamped at t=1000, due at 1100.
        clockFixture.clock.nNow = 1099;
        CHECK( map.Next( true ) == false );
        CHECK( map.GetTextureData().pTexture == &frameA );

        clockFixture.clock.nNow = 1100;
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == &frameB );

        // Frame B is due 100 ms after the moment it was switched to.
        clockFixture.clock.nNow = 1199;
        CHECK( map.Next( true ) == false );
        CHECK( map.GetTextureData().pTexture == &frameB );

        clockFixture.clock.nNow = 1200;
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == &frameA );   // wrapped around
    }
}
