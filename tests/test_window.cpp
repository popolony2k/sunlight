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
 * Window-module pieces that need no window: the shared close-handler list
 * every IWindow backend delegates to, and the compatibility aliases the
 * fullscreen strategy left behind on IEngine when it moved to IWindow.
 */

#include <doctest/doctest.h>
#include <string>
#include <type_traits>
#include "window/closehandlerlist.h"
#include "engines/iengine.h"

using SunLight :: Window :: CloseHandlerList;

TEST_SUITE( "window/CloseHandlerList" )  {

    TEST_CASE( "Fire calls every handler in registration order" )  {

        CloseHandlerList  list;
        std :: string     strOrder;

        list.Add( [&]( void ) { strOrder += "a"; } );
        list.Add( [&]( void ) { strOrder += "b"; } );
        list.Add( [&]( void ) { strOrder += "c"; } );

        list.Fire();

        CHECK( strOrder == "abc" );
    }

    TEST_CASE( "Fire on an empty list does nothing, and handlers fire again on every Fire" )  {

        CloseHandlerList  list;
        int               nCalls = 0;

        list.Fire();

        list.Add( [&]( void ) { nCalls++; } );
        list.Fire();
        list.Fire();

        CHECK( nCalls == 2 );
    }

    TEST_CASE( "Add returns distinct, positive ids" )  {

        CloseHandlerList  list;
        int               nFirst  = list.Add( []( void ) {} );
        int               nSecond = list.Add( []( void ) {} );

        CHECK( nFirst > 0 );
        CHECK( nSecond > 0 );
        CHECK( nFirst != nSecond );
    }

    TEST_CASE( "Remove unregisters only that handler; unknown and repeated ids are ignored" )  {

        CloseHandlerList  list;
        std :: string     strOrder;

        list.Add( [&]( void ) { strOrder += "a"; } );
        int  nId = list.Add( [&]( void ) { strOrder += "b"; } );
        list.Add( [&]( void ) { strOrder += "c"; } );

        list.Remove( nId );
        list.Remove( nId );
        list.Remove( 9999 );
        list.Fire();

        CHECK( strOrder == "ac" );
    }

    TEST_CASE( "A handler may remove itself while firing" )  {

        CloseHandlerList  list;
        int               nCalls = 0;
        int               nId    = 0;

        nId = list.Add( [&]( void ) { nCalls++; list.Remove( nId ); } );

        list.Fire();
        list.Fire();

        CHECK( nCalls == 1 );
    }

    TEST_CASE( "A handler removed by an earlier handler in the same Fire is skipped" )  {

        CloseHandlerList  list;
        int               nSecondCalls = 0;
        int               nSecondId    = 0;

        list.Add( [&]( void ) { list.Remove( nSecondId ); } );
        nSecondId = list.Add( [&]( void ) { nSecondCalls++; } );

        list.Fire();

        CHECK( nSecondCalls == 0 );
    }

    TEST_CASE( "A handler added while firing does not run in that same Fire, but does in the next" )  {

        CloseHandlerList  list;
        int               nLateCalls = 0;
        bool              bAdded     = false;

        list.Add( [&]( void )  {
            if( !bAdded )  {
                bAdded = true;
                list.Add( [&]( void ) { nLateCalls++; } );
            }
        } );

        list.Fire();
        CHECK( nLateCalls == 0 );

        list.Fire();
        CHECK( nLateCalls == 1 );
    }
}

TEST_SUITE( "window/FullscreenStrategy" )  {

    TEST_CASE( "The deprecated IEngine names are the very same type and values as the SunLight::Window ones" )  {

        static_assert( std :: is_same<SunLight :: Engines :: IEngine :: FullscreenStrategy,
                                      SunLight :: Window :: FullscreenStrategy> :: value,
                       "IEngine::FullscreenStrategy must alias SunLight::Window::FullscreenStrategy" );

        CHECK( SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL == SunLight :: Window :: FULLSCREEN_STRATEGY_REAL );
        CHECK( SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED == SunLight :: Window :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );

        // Distinct strategies, and REAL is the zero/default value.
        CHECK( SunLight :: Window :: FULLSCREEN_STRATEGY_REAL == 0 );
        CHECK( SunLight :: Window :: FULLSCREEN_STRATEGY_REAL != SunLight :: Window :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED );
    }
}
