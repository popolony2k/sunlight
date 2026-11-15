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
        std :: vector<stTile>                             tiles;            // the tile of each tile hit

        void OnCollision( Collider* pFirst, Collider* pSecond )  {
            colliderHits.push_back( { pFirst, pSecond } );
        }

        void OnCollision( Collider* pFirst, stTile* pSecond )  {
            tileHits.push_back( pFirst );
            tiles.push_back( *pSecond );
        }
    };

    /**
     * @brief Reacts to its first collider-to-collider hit by removing a
     * different, not-yet-checked collider from the manager - the same
     * "despawn what I just hit" shape a real game's on-hit handler takes,
     * used to exercise CollisionManager::Update()'s same-frame-removal
     * handling (see the "does not fire a stale hit" test case below).
     */
    class RemovingCollisionListener : public ICollisionListener  {

        CollisionManager  &m_Manager;
        int               m_nLayerIdToRemoveFrom;
        Collider          *m_pColliderToRemove;
        bool              m_bHasFired = false;

        public:

        std :: vector<std :: pair<Collider*, Collider*>>  colliderHits;

        RemovingCollisionListener( CollisionManager& manager, int nLayerIdToRemoveFrom, Collider* pColliderToRemove )
            : m_Manager( manager ), m_nLayerIdToRemoveFrom( nLayerIdToRemoveFrom ), m_pColliderToRemove( pColliderToRemove )  {
        }

        void OnCollision( Collider* pFirst, Collider* pSecond )  {

            colliderHits.push_back( { pFirst, pSecond } );

            if( !m_bHasFired )  {
                m_bHasFired = true;
                m_Manager.RemoveCollider( m_nLayerIdToRemoveFrom, m_pColliderToRemove );
            }
        }

        void OnCollision( Collider* pFirst, stTile* pSecond )  {
        }
    };

    /**
     * @brief Reacts to its first collider-to-collider hit by outright
     * `delete`-ing pSecond - no RemoveCollider() call at all, simulating a
     * host application that frees a despawned entity directly rather than
     * unregistering-then-freeing (or pooling it). Exercises
     * ColliderRegistry: the deleted Collider's handle stays registered in
     * the manager's layer list (nothing ever called RemoveCollider), so a
     * later Update() must resolve it to nullptr and skip it instead of
     * touching freed memory.
     */
    class DeletingCollisionListener : public ICollisionListener  {

        bool  m_bHasFired = false;

        public:

        std :: vector<std :: pair<Collider*, Collider*>>  colliderHits;

        void OnCollision( Collider* pFirst, Collider* pSecond )  {

            colliderHits.push_back( { pFirst, pSecond } );

            if( !m_bHasFired )  {
                m_bHasFired = true;
                delete pSecond;
            }
        }

        void OnCollision( Collider* pFirst, stTile* pSecond )  {
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

    TEST_CASE( "AddColliderToColliderRule refuses a layer id out of range on either side, and adds no rule" )  {

        // Found by Scarab: the second id was never checked, so ( 1, 300 ) returned true and kept a pointer read
        // from past the layer array in the rule, which the next Update() dereferenced.
        const int               nOutOfRange = 300;
        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB;
        SunLight :: Base :: stDimension2D  dim { { 0, 0 }, { 50, 50 } };

        CHECK( manager.AddColliderToColliderRule( 1, MAX_COLLIDER_LAYERS ) == false );
        CHECK( manager.AddColliderToColliderRule( MAX_COLLIDER_LAYERS, 1 ) == false );
        CHECK( manager.AddColliderToColliderRule( 1, nOutOfRange ) == false );
        CHECK( manager.AddColliderToColliderRule( nOutOfRange, 1 ) == false );

        // No rule was added: two overlapping colliders on layers 1 and 2 are never checked.
        colliderA.SetDimension2D( dim );
        colliderB.SetDimension2D( dim );
        manager.AddCollider( 1, &colliderA );
        manager.AddCollider( 2, &colliderB );
        manager.AddCollisionListener( &listener );
        manager.Update();

        CHECK( listener.colliderHits.empty() );
        CHECK( manager.AddColliderToColliderRule( 1, MAX_COLLIDER_LAYERS - 1 ) == true );    // the last valid id
    }

    TEST_CASE( "AddColliderToColliderRule refuses a negative layer id on either side while a rule already exists" )  {

        // With a rule in the list, the duplicate search used to index the layer array with the id before any
        // check - a read before the array for a negative id (seen by AddressSanitizer).
        MockTileMap        tileMap;
        CollisionManager   manager( &tileMap );

        REQUIRE( manager.AddColliderToColliderRule( 0, 1 ) == true );

        CHECK( manager.AddColliderToColliderRule( -1, 1 ) == false );
        CHECK( manager.AddColliderToColliderRule( 1, -1 ) == false );
    }

    TEST_CASE( "Every layer method refuses a negative layer id" )  {

        MockTileMap        tileMap;
        CollisionManager   manager( &tileMap );
        Collider           collider;

        CHECK( manager.AddCollider( -1, &collider ) == false );
        CHECK( manager.RemoveCollider( -1, &collider ) == false );
        CHECK( manager.AddColliderToTileRule( -1, 0 ) == false );
    }

    TEST_CASE( "RemoveAll() empties every layer, RemoveAll( id ) one layer, and any other id is refused" )  {

        // RemoveAll's default, -1 ("every layer"), compared as a huge unsigned value, so it returned false and
        // cleared nothing.
        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB, colliderC;
        SunLight :: Base :: stDimension2D  dim { { 0, 0 }, { 50, 50 } };

        colliderA.SetDimension2D( dim );
        colliderB.SetDimension2D( dim );
        colliderC.SetDimension2D( dim );
        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        manager.AddCollider( 2, &colliderC );
        manager.AddColliderToColliderRule( 0, 1 );
        manager.AddColliderToColliderRule( 0, 2 );
        manager.AddCollisionListener( &listener );

        CHECK( manager.RemoveAll( 2 ) == true );                    // one layer: only A-B still hits
        manager.Update();
        CHECK( listener.colliderHits.size() == 1 );

        CHECK( manager.RemoveAll() == true );                       // every layer: nothing hits
        listener.colliderHits.clear();
        manager.Update();
        CHECK( listener.colliderHits.empty() );

        CHECK( manager.RemoveAll( -2 ) == false );
        CHECK( manager.RemoveAll( MAX_COLLIDER_LAYERS ) == false );
    }

    TEST_CASE( "After Clear() every layer is empty but usable, as on a new manager" )  {

        // Clear() used to destroy every layer's list, so AddCollider, RemoveCollider and Update dereferenced null.
        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB;
        SunLight :: Base :: stDimension2D  dim { { 0, 0 }, { 50, 50 } };

        colliderA.SetDimension2D( dim );
        colliderB.SetDimension2D( dim );
        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        manager.AddColliderToColliderRule( 0, 1 );
        manager.AddCollisionListener( &listener );

        manager.Clear();
        manager.Update();
        CHECK( listener.colliderHits.empty() );                    // the colliders and the rule are gone

        CHECK( manager.AddCollider( 0, &colliderA ) == true );
        CHECK( manager.AddCollider( 1, &colliderB ) == true );
        CHECK( manager.RemoveCollider( 1, &colliderB ) == true );
        CHECK( manager.AddCollider( 1, &colliderB ) == true );
        CHECK( manager.AddColliderToColliderRule( 0, 1 ) == true );
        manager.Update();
        CHECK( listener.colliderHits.size() == 1 );
    }

    TEST_CASE( "Update fires OnCollision only for overlapping colliders on a paired layer" )  {

        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB, colliderC;
        SunLight :: Base :: stDimension2D           dimA { { 0, 0 },     { 50, 50 } };
        SunLight :: Base :: stDimension2D           dimB { { 25, 25 },   { 50, 50 } };  // overlaps A
        SunLight :: Base :: stDimension2D           dimC { { 500, 500 }, { 10, 10 } };  // does not overlap A

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

    TEST_CASE( "Update honors the SECOND collider's own SetInset, not just the first's" )  {

        // Regression coverage for the actual Update() call site, not just
        // Collider::Hit(Collider&) in isolation (see tests/test_collider.cpp)
        // - guards against this line ever reverting back to calling
        // pFirst->Hit(pSecond->GetDimension2D()), which would silently
        // reintroduce the bug (that overload compiles fine too, it just
        // ignores pSecond's own inset entirely). Mirrors the "fires
        // OnCollision only for overlapping colliders" test above, but with
        // an inset on colliderB (the SECOND/"pSecond" side of the rule)
        // shrinking it away from colliderA - their raw, full-size boxes
        // still overlap (0,0,50,50 vs 45,0,50,50), only colliderB's own
        // inset-shrunk effective box (55,10)-(85,40) doesn't reach
        // colliderA's (0,0)-(50,50) anymore.
        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB;
        SunLight :: Base :: stDimension2D           dimA { { 0, 0 },  { 50, 50 } };
        SunLight :: Base :: stDimension2D           dimB { { 45, 0 }, { 50, 50 } };  // overlaps A's raw box

        colliderA.SetDimension2D( dimA );
        colliderB.SetDimension2D( dimB );
        colliderB.SetInset( 0.2f, 0.2f, 0.2f, 0.2f );

        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        manager.AddColliderToColliderRule( 0, 1 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        CHECK( listener.colliderHits.size() == 0 );
    }

    TEST_CASE( "Update ignores colliders on layers with no rule between them" )  {

        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        TestCollisionListener   listener;
        Collider                colliderA, colliderB;
        SunLight :: Base :: stDimension2D           dim { { 0, 0 }, { 50, 50 } };

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
        SunLight :: Base :: stDimension2D           dim { { 0, 0 }, { 50, 50 } };

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

    TEST_CASE( "Update does not fire a stale hit for a collider an OnCollision callback "
               "already removed this same Update() call" )  {

        // Regression coverage for a real SIGSEGV (root-caused and verified
        // against this exact call site by the Caravellius session, a
        // downstream consumer of this engine): Update() used to range-
        // iterate the live per-layer std::deque<Collider*> directly, and
        // FireOnCollision() calls every ICollisionListener::OnCollision
        // synchronously, in the same call stack. A listener reacting to a
        // hit by despawning what it just hit - the single most ordinary
        // thing a game does in a hit handler - called RemoveCollider() on
        // the exact deque Update() was still iterating; std::deque::erase()
        // in the middle invalidates all of its iterators, including the
        // range-for's own captured end(), making continued iteration UB.
        //
        // This test can't reproduce the UB itself deterministically without
        // a sanitizer (that was verified separately, out-of-band, with
        // ASan: heap-use-after-free pre-fix, clean post-fix - see the PR
        // description). What it does verify deterministically, with no
        // sanitizer needed, is the actual guarantee the fix adds:
        // Update() snapshots its working lists before iterating (so a
        // same-frame removal can't invalidate that iteration) and
        // re-validates membership right before firing (so a collider
        // removed by an earlier listener call this same Update() doesn't
        // get a stale/duplicate event fired for it afterwards). Colliders
        // are never deleted here (RemoveCollider only unregisters a
        // pointer, it doesn't destroy the object - see
        // ICollisionListener's own doc comment), so pre-fix code stays
        // memory-safe in this small, 3-element scenario and the two
        // behaviors differ on a plain, portable logical assertion: without
        // the fix, colliderC still gets a hit fired for it; with the fix,
        // it doesn't.
        MockTileMap                     tileMap;
        CollisionManager                manager( &tileMap );
        Collider                        colliderA, colliderB, colliderC, colliderD;
        SunLight :: Base :: stDimension2D                   dim { { 0, 0 }, { 50, 50 } };  // all mutually overlapping

        colliderA.SetDimension2D( dim );
        colliderB.SetDimension2D( dim );
        colliderC.SetDimension2D( dim );
        colliderD.SetDimension2D( dim );

        // Layer 1 insertion order matters: B is checked first (triggering
        // the removal), C is the not-yet-checked victim, D comes after C
        // to prove iteration keeps going correctly past the removal rather
        // than stopping short or skipping unrelated entries.
        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, &colliderB );
        manager.AddCollider( 1, &colliderC );
        manager.AddCollider( 1, &colliderD );
        manager.AddColliderToColliderRule( 0, 1 );

        RemovingCollisionListener  listener( manager, 1, &colliderC );
        manager.AddCollisionListener( &listener );

        manager.Update();

        REQUIRE( listener.colliderHits.size() == 2 );
        CHECK( listener.colliderHits[0].second == &colliderB );
        CHECK( listener.colliderHits[1].second == &colliderD );  // C skipped, D still reached
    }

    TEST_CASE( "Update skips a collider a listener deleted outright, across later Update() calls" )  {

        // Regression coverage for ColliderRegistry - a defense-in-depth
        // follow-up to the same-frame-removal fix above. That fix assumes
        // a listener only ever unregisters a collider (RemoveCollider,
        // which doesn't destroy the object), never destroys it directly.
        // Nothing enforced that assumption; a listener that instead does a
        // raw `delete` on what it just hit - no RemoveCollider() call at
        // all, simulating a host application freeing a despawned entity
        // outright rather than pooling/unregistering-then-freeing it -
        // would leave the manager's layer list holding a dangling pointer,
        // read on a later Update() pass. Verified against a real,
        // deterministic ASan-caught heap-use-after-free pre-#3 (and a
        // clean run post-#3) by the Caravellius session, using this exact
        // scenario shape; see the PR description for that report.
        //
        // Like the test above, the *portable* assertion this makes doesn't
        // need a sanitizer: colliderB is heap-allocated and never freed by
        // this test itself (only by the listener's own `delete`), so if
        // Update() ever dereferences its handle after that delete, that's
        // a real heap-use-after-free independent of whether it happens to
        // crash in this particular run - ColliderRegistry::Resolve() is
        // what has to turn that into a clean, deterministic skip instead.
        MockTileMap             tileMap;
        CollisionManager        manager( &tileMap );
        DeletingCollisionListener  listener;
        Collider                colliderA;
        Collider                *pColliderB = new Collider();
        SunLight :: Base :: stDimension2D            dim { { 0, 0 }, { 50, 50 } };  // overlapping

        colliderA.SetDimension2D( dim );
        pColliderB -> SetDimension2D( dim );

        manager.AddCollider( 0, &colliderA );
        manager.AddCollider( 1, pColliderB );
        manager.AddColliderToColliderRule( 0, 1 );
        manager.AddCollisionListener( &listener );

        manager.Update();  // A vs B collide; listener deletes B, no RemoveCollider() call

        REQUIRE( listener.colliderHits.size() == 1 );
        CHECK( listener.colliderHits[0].second == pColliderB );

        // B's handle is still registered in layer 1's list (nothing ever
        // called RemoveCollider) but the object behind it no longer
        // exists. These calls must not crash, and must not fire any
        // further event for B.
        manager.Update();
        manager.Update();

        CHECK( listener.colliderHits.size() == 1 );
    }

    TEST_CASE( "AddColliderToTileRule requires the tile layer to exist on the parent map" )  {

        MockTileMap        tileMap;
        CollisionManager   manager( &tileMap );

        CHECK( manager.AddColliderToTileRule( 0, 7 ) == false );

        tileMap.layersById[7] = stLayer { true, 1.0f, { 0, 0 }, nullptr };

        CHECK( manager.AddColliderToTileRule( 0, 7 ) == true );
    }

    TEST_CASE( "Update asks the map for the tiles a collider touches, with its inset rectangle and its world-space mode (I7)" )  {

        MockTileMap             tileMap;
        TestCollisionListener   listener;
        Collider                collider;

        tileMap.layersById[7] = stLayer { true, 1.0f, { 0, 0 }, nullptr };

        CollisionManager  manager( &tileMap );

        collider.SetDimension2D( SunLight :: Base :: stDimension2D { { 10, 20 }, { 40, 20 } } );
        collider.SetInset( 0.25f, 0.0f, 0.25f, 0.0f );                // 10 px off each side: x 20, width 20
        manager.AddCollider( 0, &collider );
        manager.AddColliderToTileRule( 0, 7 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        REQUIRE( tileMap.nGetCollidingTilesCalls == 1 );
        CHECK( tileMap.lastCollidingRect.x == 20.0f );
        CHECK( tileMap.lastCollidingRect.y == 20.0f );
        CHECK( tileMap.lastCollidingRect.width == 20.0f );
        CHECK( tileMap.lastCollidingRect.height == 20.0f );
        CHECK( tileMap.bLastCollidingWorldSpace == false );

        collider.SetWorldSpace( true );                             // a collider of its own, world-space
        manager.Update();
        CHECK( tileMap.bLastCollidingWorldSpace == true );
        CHECK( listener.tileHits.empty() );                         // the map reported no tile
    }

    TEST_CASE( "A sprite's collider is tested in its sprite's world-space mode (Canvas passes it on to its collider)" )  {

        MockTileMap                   tileMap;
        SunLight :: Sprite :: Sprite  sprite;

        tileMap.layersById[7] = stLayer { true, 1.0f, { 0, 0 }, nullptr };

        CollisionManager  manager( &tileMap );

        manager.AddCollider( 0, &sprite.GetCollider() );
        manager.AddColliderToTileRule( 0, 7 );

        manager.Update();
        CHECK( tileMap.bLastCollidingWorldSpace == false );

        sprite.SetWorldSpace( true );
        manager.Update();
        CHECK( tileMap.bLastCollidingWorldSpace == true );
    }

    TEST_CASE( "Update fires once for each tile the map reports, with that tile, in its order (I7)" )  {

        MockTileMap             tileMap;
        TestCollisionListener   listener;
        Collider                collider;
        stTile                  left {}, right {};

        left.nGID  = 3;
        left.dimension  = SunLight :: Base :: stDimension2D { { 160, 160 }, { 32, 32 } };
        right.nGID = 4;
        right.dimension = SunLight :: Base :: stDimension2D { { 192, 160 }, { 32, 32 } };

        tileMap.layersById[7] = stLayer { true, 1.0f, { 0, 0 }, nullptr };
        tileMap.collidingTiles = { left, right };

        CollisionManager  manager( &tileMap );

        manager.AddCollider( 0, &collider );
        manager.AddColliderToTileRule( 0, 7 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        REQUIRE( listener.tiles.size() == 2 );
        CHECK( listener.tileHits[0] == &collider );
        CHECK( listener.tiles[0].nGID == 3 );
        CHECK( listener.tiles[0].dimension.pos.x == 160 );
        CHECK( listener.tiles[1].nGID == 4 );
        CHECK( listener.tiles[1].dimension.pos.x == 192 );
    }

    TEST_CASE( "A listener that removes the collider on its first tile gets no second tile callback" )  {

        struct RemovingTileListener : public ICollisionListener  {
            CollisionManager  *pManager = nullptr;
            int               nTileHits = 0;

            void OnCollision( Collider*, Collider* ) override  {}
            void OnCollision( Collider* pFirst, stTile* ) override  {
                nTileHits++;
                pManager -> RemoveCollider( 0, pFirst );
            }
        };

        MockTileMap           tileMap;
        RemovingTileListener  listener;
        Collider              collider;

        tileMap.layersById[7] = stLayer { true, 1.0f, { 0, 0 }, nullptr };
        tileMap.collidingTiles = { stTile {}, stTile {} };

        CollisionManager  manager( &tileMap );

        listener.pManager = &manager;
        manager.AddCollider( 0, &collider );
        manager.AddColliderToTileRule( 0, 7 );
        manager.AddCollisionListener( &listener );

        manager.Update();

        CHECK( listener.nTileHits == 1 );
    }

    TEST_CASE( "Collider::Hit on a tile with several collision shapes tests each one once, and stops" )  {

        // It stepped to the FIRST shape's next every time: with two or more shapes and a miss on the
        // second, it tested the second again forever (a hung frame).
        Collider    collider;
        tmx_object  shapes[3] {};
        tmx_tile    tile {};
        stTile      t {};

        for( int nIdx = 0; nIdx < 3; nIdx++ )  {
            shapes[nIdx].obj_type = OT_SQUARE;
            shapes[nIdx].width    = 4;
            shapes[nIdx].height   = 4;
            shapes[nIdx].next     = ( nIdx < 2 ) ? &shapes[nIdx + 1] : nullptr;
        }
        shapes[0].x = 0;   shapes[0].y = 0;
        shapes[1].x = 28;  shapes[1].y = 0;
        shapes[2].x = 0;   shapes[2].y = 28;

        tile.collision = &shapes[0];
        t.pTile        = &tile;
        t.dimension    = SunLight :: Base :: stDimension2D { { 100, 100 }, { 32, 32 } };

        collider.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 8, 8 } } );    // far from all three
        CHECK( collider.Hit( t ) == false );

        collider.SetDimension2D( SunLight :: Base :: stDimension2D { { 100, 128 }, { 8, 8 } } );   // on the third
        CHECK( collider.Hit( t ) == true );
    }
}
