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
#include "collision/collisionmanager.h"
#include "mock_tilemap.h"
#include <vector>
#include <utility>

using namespace SunLight :: Collision;
using namespace SunLight :: TileMap;

namespace  {

    /**
     * @brief Records every OnCollision() call it receives, so tests can
     * assert on what CollisionManager :: Update() actually fired.
     */
    class TestCollisionListener : public ICollisionListener  {

        public:

        std :: vector<std :: pair<Collider*, Collider*>>  colliderHits;
        std :: vector<Collider*>                          tileHits;

        void OnCollision( Collider *pFirst, Collider *pSecond )  {
            colliderHits.push_back( { pFirst, pSecond } );
        }

        void OnCollision( Collider *pFirst, stTile *pSecond )  {
            tileHits.push_back( pFirst );
        }
    };
}

TEST_SUITE( "collision/CollisionManager" )  {

    TEST_CASE( "AddCollider accepts a valid layer id and rejects an out-of-range one" )  {

        MockTileMap        tileMap;
        CollisionManager   manager( &tileMap );
        Collider           collider;

        CHECK( manager.AddCollider( 0, &collider ) == true );
        CHECK( manager.AddCollider( MAX_COLLIDER_LAYERS, &collider ) == false );
    }

    TEST_CASE( "AddColliderToColliderRule pairs two layers exactly once" )  {

        MockTileMap        tileMap;
        CollisionManager   manager( &tileMap );

        CHECK( manager.AddColliderToColliderRule( 0, 1 ) == true );
        CHECK( manager.AddColliderToColliderRule( 0, 1 ) == false );
    }

    TEST_CASE( "Update fires OnCollision only for overlapping colliders on a paired layer" )  {

        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB, colliderC;
        stDimension2D           dimA { { 0, 0 },     { 50, 50 } };
        stDimension2D           dimB { { 25, 25 },   { 50, 50 } };  // overlaps A
        stDimension2D           dimC { { 500, 500 }, { 10, 10 } };  // does not overlap A

        colliderA.SetDimension2D( dimA );
        colliderB.SetDimension2D( dimB );
        colliderC.SetDimension2D( dimC );

        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        manager.AddCollider( 1, &colliderC );
        manager.AddColliderToColliderRule( 0, 1 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        REQUIRE( listener.colliderHits.size() == 1 );
        CHECK( listener.colliderHits[0].first  == &colliderA );
        CHECK( listener.colliderHits[0].second == &colliderB );
    }

    TEST_CASE( "Update ignores colliders on layers with no rule between them" )  {

        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB;
        stDimension2D           dim { { 0, 0 }, { 50, 50 } };

        colliderA.SetDimension2D( dim );
        colliderB.SetDimension2D( dim );

        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        // No AddColliderToColliderRule() call.
        manager.AddCollisionListener( &listener );

        manager.Update();

        CHECK( listener.colliderHits.empty() );
    }

    TEST_CASE( "RemoveCollider excludes a collider from future Update checks" )  {

        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB;
        stDimension2D           dim { { 0, 0 }, { 50, 50 } };

        colliderA.SetDimension2D( dim );
        colliderB.SetDimension2D( dim );

        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        manager.AddColliderToColliderRule( 0, 1 );
        manager.AddCollisionListener( &listener );

        manager.RemoveCollider( 0, &colliderA );
        manager.Update();

        CHECK( listener.colliderHits.empty() );
    }

    TEST_CASE( "AddColliderToTileRule requires the tile layer to exist on the parent map" )  {

        MockTileMap        tileMap;
        CollisionManager   manager( &tileMap );

        CHECK( manager.AddColliderToTileRule( 0, 7 ) == false );

        tileMap.layersById[7] = stLayer { true, 255, { 0, 0 }, nullptr };

        CHECK( manager.AddColliderToTileRule( 0, 7 ) == true );
    }

    TEST_CASE( "Update's collider-to-tile path fires nothing when TileMapToTileMatrix fails" )  {

        MockTileMap             tileMap;
        TestCollisionListener   listener;
        Collider                collider;

        tileMap.layersById[7] = stLayer { true, 255, { 0, 0 }, nullptr };
        tileMap.bTileMapToTileMatrixResult = false;

        CollisionManager  manager( &tileMap );

        collider.SetDimension2D( stDimension2D { { 0, 0 }, { 32, 32 } } );
        manager.AddCollider( 0, &collider );
        manager.AddColliderToTileRule( 0, 7 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        CHECK( listener.tileHits.empty() );
        CHECK( tileMap.nGetTileCalls == 0 );
    }

    TEST_CASE( "Update's collider-to-tile path fires nothing when GetTile fails" )  {

        MockTileMap             tileMap;
        TestCollisionListener   listener;
        Collider                collider;

        tileMap.layersById[7] = stLayer { true, 255, { 0, 0 }, nullptr };
        tileMap.bGetTileResult = false;

        CollisionManager  manager( &tileMap );

        collider.SetDimension2D( stDimension2D { { 0, 0 }, { 32, 32 } } );
        manager.AddCollider( 0, &collider );
        manager.AddColliderToTileRule( 0, 7 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        CHECK( listener.tileHits.empty() );
        CHECK( tileMap.nGetTileCalls == 1 );
    }

    TEST_CASE( "Update's collider-to-tile path reaches Hit() safely when the tile has no collision shape" )  {

        MockTileMap             tileMap;
        TestCollisionListener   listener;
        Collider                collider;
        tmx_tile                fakeTile {};  // zero-initialized - collision == nullptr

        tileMap.layersById[7] = stLayer { true, 255, { 0, 0 }, nullptr };
        tileMap.bGetTileResult = true;
        tileMap.getTileResult.pTile = &fakeTile;
        tileMap.getTileResult.dimension = stDimension2D { { 0, 0 }, { 32, 32 } };

        CollisionManager  manager( &tileMap );

        collider.SetDimension2D( stDimension2D { { 0, 0 }, { 32, 32 } } );
        manager.AddCollider( 0, &collider );
        manager.AddColliderToTileRule( 0, 7 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        CHECK( tileMap.nGetTileCalls == 1 );
        CHECK( listener.tileHits.empty() );
    }
}
