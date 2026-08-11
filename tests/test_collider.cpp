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
#include "collision/collider.h"

using namespace SunLight :: Collision;
using namespace SunLight :: TileMap;

TEST_SUITE( "collision/Collider" )  {

    TEST_CASE( "Hit detects overlapping rectangles" )  {

        Collider      collider;
        stDimension2D self  { { 0, 0 }, { 50, 50 } };
        stDimension2D other { { 25, 25 }, { 50, 50 } };

        collider.SetDimension2D( self );

        CHECK( collider.Hit( other ) == true );
    }

    TEST_CASE( "Hit rejects disjoint rectangles" )  {

        Collider      collider;
        stDimension2D self  { { 0, 0 }, { 50, 50 } };
        stDimension2D other { { 100, 100 }, { 20, 20 } };

        collider.SetDimension2D( self );

        CHECK( collider.Hit( other ) == false );
    }

    TEST_CASE( "Hit treats exactly touching edges as a collision" )  {

        Collider      collider;
        stDimension2D self  { { 0, 0 }, { 50, 50 } };
        stDimension2D other { { 50, 0 }, { 50, 50 } };

        collider.SetDimension2D( self );

        CHECK( collider.Hit( other ) == true );
    }

    TEST_CASE( "SetInset defaults to zero - unchanged full-size behavior" )  {

        Collider      collider;
        stDimension2D self  { { 0, 0 }, { 50, 50 } };
        stDimension2D other { { 45, 0 }, { 50, 50 } };

        collider.SetDimension2D( self );

        // Same overlap as the plain full-size Hit() tests above - confirms
        // an untouched Collider (no SetInset call) behaves exactly as
        // before this feature existed.
        CHECK( collider.Hit( other ) == true );
    }

    TEST_CASE( "SetInset shrinks the collider's own side of the test" )  {

        Collider      collider;
        // 50x50 self, 20% inset on every edge -> effective rect is
        // (10,10)-(40,40), a 30x30 box centered in the original 50x50.
        stDimension2D self  { { 0, 0 }, { 50, 50 } };
        stDimension2D other { { 45, 0 }, { 50, 50 } };

        collider.SetDimension2D( self );
        collider.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );

        // other still overlaps self's full 50x50 box (as the test above
        // confirms) but no longer reaches the inset-shrunk 30x30 box, which
        // now ends at x=40 while other starts at x=45.
        CHECK( collider.Hit( other ) == false );
    }

    TEST_CASE( "SetInset still detects overlap within the shrunk box" )  {

        Collider      collider;
        stDimension2D self  { { 0, 0 }, { 50, 50 } };
        stDimension2D other { { 35, 0 }, { 50, 50 } };

        collider.SetDimension2D( self );
        collider.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );

        // Effective rect ends at x=40 - other starts at x=35, still inside it.
        CHECK( collider.Hit( other ) == true );
    }

    // Hit(Collider&) - unlike Hit(stDimension2D&) above (which only ever
    // applies "self"'s own inset against the other side's raw, full
    // dimension - by design, since the other side there isn't necessarily
    // a Collider at all), this overload applies BOTH colliders' own
    // SetInset shrink. Regression coverage for a real bug found via
    // Caravellius (2026-08-11): CollisionManager::Update() used to call
    // the stDimension2D overload for it's collider-to-collider rule
    // checks, which silently ignored the SECOND collider's own inset
    // entirely - an enemy's own SetInset never took effect against a
    // player bullet, however aggressively it was shrunk, purely because
    // of which side of the rule pairing it happened to be registered on.

    TEST_CASE( "Hit(Collider&) honors both colliders' own inset" )  {

        Collider      first;
        Collider      second;
        stDimension2D firstDim  { { 0, 0 }, { 50, 50 } };
        stDimension2D secondDim { { 45, 0 }, { 50, 50 } };

        first.SetDimension2D( firstDim );
        second.SetDimension2D( secondDim );

        // Only "second"'s own inset shrinks it's own box - "first" is
        // still it's full, un-inset 50x50. second's effective rect is
        // (55,10)-(90,40) (10,10)-(40,40) offset by it's own (45,0)
        // position - starts at x=55, past first's own right edge (x=50),
        // so they no longer overlap even though their raw, full boxes
        // still would (same setup as the "shrinks the collider's own
        // side" test above, just on the "other" side of the pairing this
        // time - the exact case Hit(stDimension2D&) could never express).
        second.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );

        CHECK( first.Hit( second ) == false );

        // Confirm it's genuinely second's own inset being honored, not
        // some other effect - the same pair without it overlaps.
        Collider second_no_inset;

        second_no_inset.SetDimension2D( secondDim );

        CHECK( first.Hit( second_no_inset ) == true );
    }

    TEST_CASE( "Hit(Collider&) still detects overlap within both shrunk boxes" )  {

        Collider      first;
        Collider      second;
        stDimension2D firstDim  { { 0, 0 }, { 50, 50 } };
        stDimension2D secondDim { { 35, 0 }, { 50, 50 } };

        first.SetDimension2D( firstDim );
        second.SetDimension2D( secondDim );
        first.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );
        second.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );

        // first's effective rect ends at x=40; second's effective rect
        // (35+10=45 to 35+40=75) starts at x=45 - past first's own x=40,
        // so even with both sides properly shrunk this specific pair no
        // longer overlaps (tighter than the single-sided-inset case
        // above, exactly because now both edges moved inward).
        CHECK( first.Hit( second ) == false );

        // A closer pair, still with both sides shrunk, that does overlap.
        stDimension2D closerDim { { 25, 0 }, { 50, 50 } };
        Collider      closer;

        closer.SetDimension2D( closerDim );
        closer.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );

        // closer's effective rect starts at x=25+10=35, still inside
        // first's own effective rect (ends at x=40).
        CHECK( first.Hit( closer ) == true );
    }
}
