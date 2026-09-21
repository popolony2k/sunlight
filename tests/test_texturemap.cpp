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
 * TextureMap: the ordered list of frames of one sprite sequence, and its cursor. The cursor used to be a
 * std::deque iterator set when the first entry was added and kept across every later push_back - which
 * the standard invalidates - so stepping from it read freed memory once the deque had outgrown its block
 * map (found by AddressSanitizer with a few thousand entries; scarab-df's recycled pool slots append one
 * entry per recycle). It is an index now. Most of what only an address checker can see is asserted here as
 * behaviour - every entry visited, in order - and the suite is meant to be run under ASAN too (CLAUDE.md).
 */

#include <doctest/doctest.h>
#include <cstdint>
#include "sprite/texturemap.h"
#include "canvas/texturecanvas.h"
#include "mock_clock.h"

using namespace SunLight :: Sprite;
using namespace SunLight :: Canvas;

namespace  {

    // A frame identity that is never dereferenced: TextureMap only stores and returns the pointer.
    TextureCanvas* Frame( int nIndex )  {
        return reinterpret_cast<TextureCanvas*>( static_cast<uintptr_t>( nIndex + 1 ) * 16 );
    }
}

TEST_SUITE( "sprite/TextureMap cursor" )  {

    TEST_CASE( "Entries appended AFTER the cursor was set are all reached, in order (no iterator kept across push_back)" )  {

        const int  nEntries = 6000;                             // far past one deque block map
        TextureMap  map;

        for( int nIndex = 0; nIndex < nEntries; nIndex++ )
            map.AddTexture( Frame( nIndex ), -1 );              // the cursor is set by the FIRST call only

        REQUIRE( map.GetTextureCount() == ( size_t ) nEntries );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );

        // Delay -1: every Next steps at once. No First() re-point in between - that is the point.
        for( int nIndex = 1; nIndex < nEntries; nIndex++ )  {
            REQUIRE( map.Next( true ) == true );
            REQUIRE( map.GetTextureData().pTexture == Frame( nIndex ) );
        }

        // Past the last entry it wraps to the first and says the frame is held.
        CHECK( map.Next( true ) == false );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );
    }

    TEST_CASE( "Timed frames appended after the cursor was set are stepped by the clock" )  {

        MockClockFixture  clockFixture;
        TextureMap        map;
        const int         nEntries = 3000;

        clockFixture.clock.nNow = 1000;

        for( int nIndex = 0; nIndex < nEntries; nIndex++ )
            map.AddTexture( Frame( nIndex ), 10 );

        for( int nIndex = 1; nIndex < nEntries; nIndex++ )  {
            clockFixture.clock.Advance( 10 );
            REQUIRE( map.Next( true ) == true );
            REQUIRE( map.GetTextureData().pTexture == Frame( nIndex ) );
        }

        clockFixture.clock.Advance( 10 );
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );     // wrapped
    }

    TEST_CASE( "First() rewinds the cursor; Next(false) walks to the end and says so" )  {

        TextureMap  map;

        for( int nIndex = 0; nIndex < 3; nIndex++ )
            map.AddTexture( Frame( nIndex ), 100 );

        CHECK( map.First() == true );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );
        CHECK( map.Next( false ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 1 ) );
        CHECK( map.Next( false ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 2 ) );
        CHECK( map.Next( false ) == false );                        // past the last
        CHECK( map.First() == true );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );
    }

    TEST_CASE( "An empty map has nothing to step to" )  {

        TextureMap  map;

        CHECK( map.GetTextureCount() == 0 );
        CHECK( map.First() == false );
        CHECK( map.Next( true ) == false );
        CHECK( map.Next( false ) == false );
    }

    TEST_CASE( "SetDelay changes the delay of every entry and restarts the current frame's timing from now" )  {

        MockClockFixture  clockFixture;
        TextureMap        map;

        clockFixture.clock.nNow = 1000;
        map.AddTexture( Frame( 0 ), 200 );                          // due at 1200
        map.AddTexture( Frame( 1 ), 200 );

        clockFixture.clock.nNow = 1050;
        map.SetDelay( 40 );                                         // the current frame is now due at 1090, not 1200

        clockFixture.clock.nNow = 1089;
        CHECK( map.Next( true ) == false );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );

        clockFixture.clock.nNow = 1090;
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 1 ) );

        // The next frame uses the new delay too (40 ms from the moment it was switched to).
        clockFixture.clock.nNow = 1129;
        CHECK( map.Next( true ) == false );
        clockFixture.clock.nNow = 1130;
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );

        // -1 keeps its meaning: every Next steps at once, and the wrap reports a held frame.
        map.SetDelay( -1 );
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 1 ) );
        CHECK( map.Next( true ) == false );
        CHECK( map.GetTextureData().pTexture == Frame( 0 ) );
    }

    TEST_CASE( "SetDelay with the delay every entry already has changes nothing: it cannot keep postponing the next step" )  {

        MockClockFixture  clockFixture;
        TextureMap        map;

        clockFixture.clock.nNow = 1000;
        map.AddTexture( Frame( 0 ), 100 );                          // due at 1100
        map.AddTexture( Frame( 1 ), 100 );

        // Reconfigured over and over with the same delay, shortly before it is due: the schedule stays put.
        clockFixture.clock.nNow = 1090;
        map.SetDelay( 100 );
        map.SetDelay( 100 );

        clockFixture.clock.nNow = 1100;
        CHECK( map.Next( true ) == true );
        CHECK( map.GetTextureData().pTexture == Frame( 1 ) );
    }

    TEST_CASE( "SetDelay never moves the cursor" )  {

        MockClockFixture  clockFixture;
        TextureMap        map;

        clockFixture.clock.nNow = 1000;
        for( int nIndex = 0; nIndex < 4; nIndex++ )
            map.AddTexture( Frame( nIndex ), 10 );

        clockFixture.clock.Advance( 10 );
        map.Next( true );
        clockFixture.clock.Advance( 10 );
        map.Next( true );
        REQUIRE( map.GetTextureData().pTexture == Frame( 2 ) );

        map.SetDelay( 500 );
        CHECK( map.GetTextureData().pTexture == Frame( 2 ) );
        map.SetDelay( -1 );
        CHECK( map.GetTextureData().pTexture == Frame( 2 ) );
        map.SetDelay( 10 );
        CHECK( map.GetTextureData().pTexture == Frame( 2 ) );
    }
}
