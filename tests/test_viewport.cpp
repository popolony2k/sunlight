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

#include <doctest/doctest.h>
#include "base/viewport.h"

using namespace SunLight :: Base;
using namespace SunLight :: TileMap;

TEST_SUITE( "base/Viewport" )  {

    TEST_CASE( "default construction starts at 1.0 zoom factor with user zoom enabled" )  {

        Viewport vp;

        CHECK( vp.GetZoomProperties().fZoomFactor == doctest :: Approx( 1.0f ) );
        CHECK( vp.GetZoomProperties().bEnabledUserZoom == true );
    }

    TEST_CASE( "ZoomIn/ZoomOut move the factor by one scale step and ResetZoom restores it" )  {

        Viewport vp;
        float    fBase = vp.GetZoomProperties().fZoomFactor;

        vp.ZoomIn();
        CHECK( vp.GetZoomProperties().fZoomFactor > fBase );

        vp.ResetZoom();
        CHECK( vp.GetZoomProperties().fZoomFactor == doctest :: Approx( fBase ) );

        vp.ZoomOut();
        CHECK( vp.GetZoomProperties().fZoomFactor < fBase );
    }

    TEST_CASE( "ZoomIn/ZoomOut are no-ops once user zoom is disabled" )  {

        Viewport vp;
        float    fBase = vp.GetZoomProperties().fZoomFactor;

        vp.SetEnableUserZoom( false );
        vp.ZoomIn();
        vp.ZoomOut();

        CHECK( vp.GetZoomProperties().fZoomFactor == doctest :: Approx( fBase ) );
    }

    TEST_CASE( "SetZoom is rejected once it falls outside the configured min/max border" )  {

        Viewport vp;

        vp.SetMinZoom( 10 );
        vp.SetMaxZoom( 20 );

        vp.SetZoom( 15 );
        float fInRange = vp.GetZoomProperties().fZoomFactor;
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 15 );

        vp.SetZoom( 5 );
        CHECK( vp.GetZoomProperties().fZoomFactor == doctest :: Approx( fInRange ) );

        vp.SetZoom( 25 );
        CHECK( vp.GetZoomProperties().fZoomFactor == doctest :: Approx( fInRange ) );
    }

    TEST_CASE( "GetZoomFactor returns the factor of the position asked for, not the current one" )  {

        // Position p maps to (p + 1) x 0.0625 - every value here is an exact
        // multiple of 1/16, so exact equality is the right assertion.
        Viewport vp;

        // Default: current position is 15 (factor 1.0).
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 15 );

        CHECK( vp.GetZoomFactor( 60 )  == 3.8125f );   // not the current position's 1.0
        CHECK( vp.GetZoomFactor( 0 )   == 0.0625f );
        CHECK( vp.GetZoomFactor( 255 ) == 16.0f );
        CHECK( vp.GetZoomFactor( 15 )  == 1.0f );

        // Asking is read-only: the viewport's own zoom is untouched.
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 15 );
        CHECK( vp.GetZoomProperties().fZoomFactor == 1.0f );

        // Moving the current zoom doesn't change what other positions report.
        vp.SetZoom( 40 );
        CHECK( vp.GetZoomFactor( 60 ) == 3.8125f );
        CHECK( vp.GetZoomFactor( 40 ) == 2.5625f );
        CHECK( vp.GetZoomFactor( 15 ) == 1.0f );
    }

    TEST_CASE( "GetZoomFactor falls back to the preferred position's factor outside the allowed range" )  {

        Viewport vp;

        // Default range is [0, 256): 256 and beyond are out of range.
        CHECK( vp.GetZoomFactor( 256 )  == 1.0f );     // preferred position 15 -> 1.0
        CHECK( vp.GetZoomFactor( 5000 ) == 1.0f );

        // The fallback follows the preferred position, not the current one.
        vp.SetPreferredZoom( 30 );
        vp.SetZoom( 60 );
        CHECK( vp.GetZoomFactor( 5000 ) == 1.9375f );  // (30 + 1) x 0.0625

        // Narrowing the border turns positions outside it into fallbacks;
        // the upper bound is exclusive, the lower inclusive.
        vp.SetMaxZoom( 100 );
        vp.SetMinZoom( 10 );
        CHECK( vp.GetZoomFactor( 99 )  == 6.25f );     // last valid position
        CHECK( vp.GetZoomFactor( 100 ) == 1.9375f );   // excluded upper bound
        CHECK( vp.GetZoomFactor( 150 ) == 1.9375f );
        CHECK( vp.GetZoomFactor( 10 )  == 0.6875f );   // first valid position
        CHECK( vp.GetZoomFactor( 9 )   == 1.9375f );   // below the lower bound
    }

    TEST_CASE( "GetClippedRect keeps a rectangle fully inside the viewport unchanged" )  {

        Viewport      vp;
        stDimension2D dst;
        stDimension2D viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D src         { { 10, 10 }, { 100, 100 } };

        vp.SetDimension2D( viewportDim );

        CHECK( vp.GetClippedRect( src, dst ) == true );
        CHECK( dst.pos.x == 10 );
        CHECK( dst.pos.y == 10 );
        CHECK( dst.size.nWidth  == 100 );
        CHECK( dst.size.nHeight == 100 );
    }

    TEST_CASE( "GetClippedRect trims a rectangle crossing the right/bottom edge" )  {

        Viewport      vp;
        stDimension2D dst;
        stDimension2D viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D src         { { 750, 10 }, { 100, 100 } };

        vp.SetDimension2D( viewportDim );

        CHECK( vp.GetClippedRect( src, dst ) == true );
        CHECK( dst.size.nWidth == 50 );
    }

    TEST_CASE( "GetClippedRect rejects a rectangle fully outside the viewport" )  {

        Viewport      vp;
        stDimension2D dst;
        stDimension2D viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D src         { { 900, 10 }, { 100, 100 } };

        vp.SetDimension2D( viewportDim );

        CHECK( vp.GetClippedRect( src, dst ) == false );
    }

    TEST_CASE( "GetClippedRect trims a rectangle starting before the viewport origin" )  {

        Viewport      vp;
        stDimension2D dst;
        stDimension2D viewportDim { { 0, 0 }, { 800, 600 } };
        stDimension2D src         { { -30, 10 }, { 100, 100 } };

        vp.SetDimension2D( viewportDim );

        CHECK( vp.GetClippedRect( src, dst ) == true );
        CHECK( dst.pos.x == 0 );
        CHECK( dst.size.nWidth == 70 );
    }
}
