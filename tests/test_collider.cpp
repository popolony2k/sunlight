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
}
