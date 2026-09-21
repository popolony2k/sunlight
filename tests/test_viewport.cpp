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
#include <cstdlib>

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
        // both bounds are inclusive positions.
        vp.SetMaxZoom( 100 );
        vp.SetMinZoom( 10 );
        CHECK( vp.GetZoomFactor( 100 ) == 6.3125f );   // last valid position (101 x 0.0625)
        CHECK( vp.GetZoomFactor( 101 ) == 1.9375f );   // just past it: fallback
        CHECK( vp.GetZoomFactor( 150 ) == 1.9375f );
        CHECK( vp.GetZoomFactor( 10 )  == 0.6875f );   // first valid position
        CHECK( vp.GetZoomFactor( 9 )   == 1.9375f );   // just below it: fallback
    }

    TEST_CASE( "Zoom limits can be widened again as well as narrowed (validated against the absolute scale)" )  {

        Viewport vp;

        vp.SetMinZoom( 50 );
        vp.SetMaxZoom( 60 );

        vp.SetZoom( 20 );                                        // outside [50, 60]
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 50 );   // (clamped there by the narrowing, and stays)
        vp.SetZoom( 100 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 50 );

        // Widen both ways - used to be impossible (a one-way ratchet).
        vp.SetMinZoom( 10 );
        vp.SetMaxZoom( 200 );

        vp.SetZoom( 20 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 20 );
        vp.SetZoom( 200 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 200 );
        vp.SetZoom( 201 );                                       // still outside
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 200 );

        // Back to the full scale.
        vp.SetMinZoom( ZOOM_POS_MIN );
        vp.SetMaxZoom( ZOOM_POS_MAX );

        vp.SetZoom( ZOOM_POS_MIN );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_MIN );
        vp.SetZoom( ZOOM_POS_MAX );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_MAX );
    }

    TEST_CASE( "SetMaxZoom is an inclusive position: SetMaxZoom( ZOOM_POS_MAX ) leaves the whole scale usable" )  {

        Viewport vp;

        vp.SetMaxZoom( ZOOM_POS_MAX );

        vp.SetZoom( ZOOM_POS_MAX );                      // used to be unreachable (254 was the last)
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_MAX );
        CHECK( vp.GetZoomProperties().fZoomFactor == ZOOM_FACTOR_MAX );

        // ZoomIn stops AT the maximum, not one short of it.
        vp.ZoomIn();
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_MAX );

        vp.SetMaxZoom( 20 );
        vp.SetMinZoom( 10 );
        vp.SetZoom( 20 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 20 );   // the maximum itself is allowed
        vp.ZoomIn();
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 20 );

        vp.SetZoom( 10 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );   // so is the minimum
        vp.ZoomOut();
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );

        vp.SetZoom( 21 );                                        // one past the maximum
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );
    }

    TEST_CASE( "Zoom limit requests off the scale, or that would cross the other limit, are rejected and change nothing" )  {

        Viewport vp;

        vp.SetMinZoom( 10 );
        vp.SetMaxZoom( 20 );

        vp.SetMaxZoom( ZOOM_POS_COUNT );        // one past the last valid position
        vp.SetMaxZoom( 5000 );
        vp.SetMinZoom( ZOOM_POS_COUNT );
        vp.SetMaxZoom( 5 );                     // would put max below min (10)
        vp.SetMinZoom( 30 );                    // would put min above max (20)

        // Limits still exactly [10, 20]: the edges accepted, beyond rejected.
        vp.SetZoom( 20 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 20 );
        vp.SetZoom( 10 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );
        vp.SetZoom( 21 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );
        vp.SetZoom( 9 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );

        // Min == max is a legal single-position range.
        vp.SetMaxZoom( 10 );
        vp.SetZoom( 10 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );
        vp.SetZoom( 11 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 10 );
    }

    TEST_CASE( "Narrowing the limits clamps the current and preferred zoom (and the factor) into the new range" )  {

        Viewport vp;

        vp.SetZoom( 100 );
        vp.SetPreferredZoom( 100 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 100 );

        // Max below the current position: both come down to the max.
        vp.SetMaxZoom( 50 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 50 );
        CHECK( vp.GetZoomProperties().nPreferredZoomPos == 50 );
        CHECK( vp.GetZoomProperties().fZoomFactor == 3.1875f );   // 51 x 0.0625

        // Min above them: both come up to the min.
        vp.SetMaxZoom( ZOOM_POS_MAX );
        vp.SetMinZoom( 120 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 120 );
        CHECK( vp.GetZoomProperties().nPreferredZoomPos == 120 );
        CHECK( vp.GetZoomProperties().fZoomFactor == 7.5625f );   // 121 x 0.0625

        // ResetZoom lands on the (clamped) preferred position, inside the range.
        vp.ResetZoom();
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 120 );

        // Widening doesn't move anything.
        vp.SetMinZoom( ZOOM_POS_MIN );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 120 );
    }

    TEST_CASE( "GetEnableUserZoom reads back what SetEnableUserZoom last set" )  {

        Viewport vp;

        CHECK( vp.GetEnableUserZoom() == true );        // default: user zoom on

        vp.SetEnableUserZoom( false );
        CHECK( vp.GetEnableUserZoom() == false );
        CHECK( vp.GetZoomProperties().bEnabledUserZoom == false );

        vp.SetEnableUserZoom( true );
        CHECK( vp.GetEnableUserZoom() == true );
    }

    TEST_CASE( "The public ZOOM_* constants describe the scale the viewport is actually built from" )  {

        Viewport vp;

        // The step and the range.
        CHECK( ZOOM_STEP == 0.0625f );
        CHECK( ZOOM_POS_MIN == 0u );
        CHECK( ZOOM_POS_COUNT == 256u );
        CHECK( ZOOM_POS_MAX == 255u );
        CHECK( ZOOM_POS_MAX == ZOOM_POS_COUNT - 1u );

        // Factor of a position is (p + 1) x step, with exact endpoints.
        CHECK( ZOOM_FACTOR_MIN == 0.0625f );
        CHECK( ZOOM_FACTOR_MAX == 16.0f );
        CHECK( vp.GetZoomFactor( ZOOM_POS_MIN ) == ZOOM_FACTOR_MIN );
        CHECK( vp.GetZoomFactor( ZOOM_POS_MAX ) == ZOOM_FACTOR_MAX );

        // ZOOM_POS_MAX is the LAST valid position and ZOOM_POS_COUNT the
        // exclusive bound just past it.
        vp.SetZoom( ZOOM_POS_MAX );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_MAX );
        CHECK( vp.GetZoomProperties().fZoomFactor == ZOOM_FACTOR_MAX );

        vp.SetZoom( ZOOM_POS_COUNT );                  // rejected: out of range
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_MAX );

        // The default preferred position is ZOOM_POS_DEFAULT, factor 1.0.
        Viewport  fresh;

        CHECK( ZOOM_POS_DEFAULT == 15u );
        CHECK( fresh.GetZoomProperties().nPreferredZoomPos == ZOOM_POS_DEFAULT );
        CHECK( fresh.GetZoomProperties().nCurrentZoomPos == ZOOM_POS_DEFAULT );
        CHECK( fresh.GetZoomProperties().fZoomFactor == 1.0f );
        CHECK( fresh.GetZoomFactor( ZOOM_POS_DEFAULT ) == 1.0f );

        // Every position matches (p + 1) x ZOOM_STEP bit for bit, so a
        // consumer can trust the formula documented on the constants.
        unsigned  nMismatches = 0;

        for( unsigned nPos = ZOOM_POS_MIN; nPos <= ZOOM_POS_MAX; nPos++ )  {
            if( fresh.GetZoomFactor( nPos ) != ( nPos + 1 ) * ZOOM_STEP )
                nMismatches++;
        }

        CHECK( nMismatches == 0 );
    }

    TEST_CASE( "A requested factor that is a multiple of ZOOM_STEP maps exactly to a position (what validating a factor relies on)" )  {

        // ZOOM_STEP is a power of two, so factor / step is exact: a factor is
        // valid iff that quotient is a whole number in [1, ZOOM_POS_COUNT].
        CHECK( 3.8125f / ZOOM_STEP == 61.0f );          // position 60
        CHECK( 1.0f / ZOOM_STEP == 16.0f );             // position 15
        CHECK( ZOOM_FACTOR_MAX / ZOOM_STEP == ( float ) ZOOM_POS_COUNT );
        CHECK( ZOOM_FACTOR_MIN / ZOOM_STEP == 1.0f );

        // Not a multiple of the step: the quotient isn't whole.
        float  fQuotient = 1.03f / ZOOM_STEP;

        CHECK( fQuotient != ( float ) ( unsigned ) fQuotient );
    }

    TEST_CASE( "The viewport is the rectangle [pos, pos + size): a rectangle is clipped at pos + size, not at size" )  {

        // (10, 10, 1240, 900): x in [10, 1250), y in [10, 910).
        Viewport      vp;
        stDimension2D viewportDim { { 10, 10 }, { 1240, 900 } };

        vp.SetDimension2D( viewportDim );

        // Inside: only translated by the viewport's position.
        stDimension2D  inside { { 100, 200 }, { 50, 40 } };
        stDimension2D  dst;

        CHECK( vp.GetClippedRect( inside, dst ) == true );
        CHECK( dst.pos.x == 110 );
        CHECK( dst.pos.y == 210 );
        CHECK( dst.size.nWidth == 50 );
        CHECK( dst.size.nHeight == 40 );

        // View-local x = 1200 lands at screen x = 1210; width 100 -> ends at 1310,
        // 60 past the right edge (1250): 40 pixels stay.
        stDimension2D  crossing { { 1200, 100 }, { 100, 20 } };

        CHECK( vp.GetClippedRect( crossing, dst ) == true );
        CHECK( dst.pos.x == 1210 );
        CHECK( dst.size.nWidth == 40 );

        // A rectangle ending exactly on the edge is untouched...
        stDimension2D  touching { { 1200, 100 }, { 40, 20 } };

        CHECK( vp.GetClippedRect( touching, dst ) == true );
        CHECK( dst.size.nWidth == 40 );

        // ...and one pixel more is trimmed by exactly one.
        stDimension2D  onePast { { 1200, 100 }, { 41, 20 } };

        CHECK( vp.GetClippedRect( onePast, dst ) == true );
        CHECK( dst.size.nWidth == 40 );

        // Same on the vertical axis: bottom edge is y = 910.
        stDimension2D  tall { { 100, 850 }, { 20, 100 } };

        CHECK( vp.GetClippedRect( tall, dst ) == true );
        CHECK( dst.pos.y == 860 );
        CHECK( dst.size.nHeight == 50 );

        // Entirely beyond the right edge (screen x 1260..1280): not visible.
        stDimension2D  beyond { { 1250, 100 }, { 20, 20 } };

        CHECK( vp.GetClippedRect( beyond, dst ) == false );
    }

    TEST_CASE( "A rectangle inside [pos, pos + size) is not clipped just because pos is not the origin" )  {

        // The old far-edge reading of size (with pos (100, 100), size (200, 200) meaning
        // "x < 200") rejected sprites at screen x 250..270 - inside the real rectangle
        // [100, 300). This is the case that motivated the change.
        Viewport      vp;
        stDimension2D viewportDim { { 100, 100 }, { 200, 200 } };
        stDimension2D dst;

        vp.SetDimension2D( viewportDim );

        stDimension2D  s1 { { 150, 10 }, { 20, 20 } };      // screen x 250..270
        CHECK( vp.GetClippedRect( s1, dst ) == true );
        CHECK( dst.size.nWidth == 20 );

        stDimension2D  s2 { { 100, 10 }, { 20, 20 } };      // screen x 200..220
        CHECK( vp.GetClippedRect( s2, dst ) == true );
        CHECK( dst.size.nWidth == 20 );                     // used to be clipped to 0

        stDimension2D  s3 { { 190, 10 }, { 20, 20 } };      // screen x 290..310: 10 past the edge (300)
        CHECK( vp.GetClippedRect( s3, dst ) == true );
        CHECK( dst.size.nWidth == 10 );
    }

    TEST_CASE( "GetClippedRect trims src coordinates before the viewport origin, at a non-zero pos too" )  {

        Viewport      vp;
        stDimension2D viewportDim { { 50, 60 }, { 400, 300 } };
        stDimension2D dst;

        vp.SetDimension2D( viewportDim );

        stDimension2D  src { { -10, -20 }, { 100, 100 } };

        CHECK( vp.GetClippedRect( src, dst ) == true );
        CHECK( dst.pos.x == 50 );                    // clamped to the viewport's origin
        CHECK( dst.pos.y == 60 );
        CHECK( dst.size.nWidth == 90 );              // the trimmed part is gone
        CHECK( dst.size.nHeight == 80 );
    }

    TEST_CASE( "GetClippedRect on [pos, pos + size) equals the legacy far-edge clipping on (pos, pos + size), everywhere" )  {

        // ORACLE: the pre-v0.29.0 function, copied verbatim, with its `size` argument being the
        // far-edge coordinate. For the same visible area the new function must give the very
        // same answer - that is what keeps Caravellius/Scarab (10, 10, 1250, 910 -> 10, 10,
        // 1240, 900) pixel-identical. A deterministic sweep, not random noise.
        struct Legacy  {
            static bool Clip( int px, int py, int farX, int farY, float fZoom, stDimension2D src, stDimension2D &dst )  {

                float    fClippingX;
                float    fClippingY;
                int32_t  nPos;

                if( ( src.pos.x > farX ) || ( src.pos.y > farY ) || ( src.pos.x < 0 ) || ( src.pos.y < 0 ) ) {

                    if( src.pos.x < 0 ) {
                        nPos = std :: abs( src.pos.x );
                        src.size.nWidth -= ( nPos < src.size.nWidth ? nPos : src.size.nWidth );
                        src.pos.x = 0;
                    }

                    if( src.pos.y < 0 ) {
                        nPos = std :: abs( src.pos.y );
                        src.size.nHeight -= ( nPos < src.size.nHeight ? nPos : src.size.nHeight );
                        src.pos.y = 0;
                    }
                }

                dst.pos.x        = ( int ) ( ( src.pos.x * fZoom ) + px );
                dst.pos.y        = ( int ) ( ( src.pos.y * fZoom ) + py );
                dst.size.nWidth  = ( int ) ( src.size.nWidth * fZoom );
                dst.size.nHeight = ( int ) ( src.size.nHeight * fZoom );
                fClippingX       = ( float ) ( dst.pos.x + dst.size.nWidth );
                fClippingY       = ( float ) ( dst.pos.y + dst.size.nHeight );

                if( fClippingX > farX )  {
                    fClippingX-=farX;

                    if( fClippingX > dst.size.nWidth )
                        return false;

                    dst.size.nWidth-=( int ) fClippingX;
                }

                if( fClippingY > farY )  {
                    fClippingY-=farY;

                    if( fClippingY > dst.size.nHeight )
                        return false;

                    dst.size.nHeight-= ( int ) fClippingY;
                }

                return true;
            }
        };

        // (px, py, width, height): Caravellius's, the samples', origin-anchored and odd ones.
        int  aViewports[][4] = { { 10, 10, 1240, 900 }, { 10, 10, 890, 790 }, { 0, 0, 800, 600 },
                                 { 0, 0, 1260, 920 }, { 25, 40, 875, 660 }, { 7, 3, 633, 477 },
                                 { 100, 100, 200, 200 }, { 1000, 20, 200, 200 } };
        long nCompared   = 0;
        long nMismatches = 0;
        long nVisible    = 0;

        for( const auto &v : aViewports )  {
            Viewport       vp;
            stDimension2D  viewportDim { { v[0], v[1] }, { v[2], v[3] } };

            vp.SetDimension2D( viewportDim );

            // Every zoom position that is a multiple of 1/16, sampled.
            for( unsigned nZoomPos = ZOOM_POS_MIN; nZoomPos <= ZOOM_POS_MAX; nZoomPos += 5 )  {
                vp.SetZoom( nZoomPos );

                float  fZoom = vp.GetZoomProperties().fZoomFactor;

                for( int nX = -60; nX < 1500; nX += 37 )  {
                    for( int nY = -60; nY < 1100; nY += 41 )  {
                        for( int nSize : { 1, 16, 64, 300 } )  {
                            stDimension2D  src { { nX, nY }, { nSize, nSize + 7 } };
                            stDimension2D  dstNew {};
                            stDimension2D  dstOld {};

                            bool  bNew = vp.GetClippedRect( src, dstNew );
                            bool  bOld = Legacy :: Clip( v[0], v[1], v[0] + v[2], v[1] + v[3], fZoom, src, dstOld );

                            nCompared++;

                            if( bNew )
                                nVisible++;

                            if( ( bNew != bOld ) ||
                                ( bNew && ( ( dstNew.pos.x != dstOld.pos.x ) || ( dstNew.pos.y != dstOld.pos.y ) ||
                                            ( dstNew.size.nWidth != dstOld.size.nWidth ) ||
                                            ( dstNew.size.nHeight != dstOld.size.nHeight ) ) ) )
                                nMismatches++;
                        }
                    }
                }
            }
        }

        CHECK( nCompared > 100000 );
        CHECK( nVisible > 10000 );          // the sweep really exercises visible rectangles, not just rejections
        CHECK( nMismatches == 0 );
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
