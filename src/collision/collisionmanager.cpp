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

#include "collision/collisionmanager.h"
#include "collision/colliderregistry.h"
#include <algorithm>
#include <vector>

// RemoveAll's id for "every layer" - its documented default (ICollisionManager::RemoveAll).
#define __ALL_COLLIDER_LAYERS   ( -1 )


namespace SunLight  {
    namespace Collision  {

        /**
         * Throw the OnCollision event through all registered listeners.
         * @param pFirst The first collider involved in the collision;
         * @param pSecond The second collider involved in the collision;
         */
        void CollisionManager :: FireOnCollision( SunLight :: Collision :: Collider* pFirst, 
                                                  SunLight :: Collision :: Collider* pSecond )  {

            /*
             * A listener may call AddCollisionListener from OnCollision, which appends to
             * m_Listeners - a std::deque, whose push_back invalidates every iterator into it, so
             * the range-for this replaced was undefined behaviour then. Its indexes stay valid:
             * walked by index over the listeners there were when this event fired, so one added
             * during it starts with the next collision. (Listeners are never removed.)
             */
            size_t  nListeners = m_Listeners.size();

            for( size_t nListener = 0; nListener < nListeners; nListener++ )  {
                m_Listeners[nListener] -> OnCollision( pFirst, pSecond );
            }
        }

        /**
         * Throw the OnCollision event through all registered listeners.
         * @param pFirst The collider involved in the collision;
         * @param pSecond The layer tile object involved in the collision;
         */
        void CollisionManager :: FireOnCollision( SunLight :: Collision :: Collider* pFirst, 
                                                  SunLight :: TileMap :: stTile* pSecond )  {

            // By index, as above.
            size_t  nListeners = m_Listeners.size();

            for( size_t nListener = 0; nListener < nListeners; nListener++ )  {
                m_Listeners[nListener] -> OnCollision( pFirst, pSecond );
            }
        }

        /**
         * Constructor. Initialize all class data.
         * @param pParent Pointer to the TileMap that this collision
         * manager is attached;
         */
        CollisionManager :: CollisionManager( SunLight :: TileMap :: ITileMap* pParent )  {

            m_pParent = pParent;

            for( int nCount = 0; nCount < m_ColliderLayerList.size(); nCount++)  {
                m_ColliderLayerList[nCount] = std :: make_unique<ColliderList>();
            }
        }

        /**
         * Destructor. Finalize all class data.
         */
        CollisionManager :: ~CollisionManager( void )  {

            Clear();
            m_Listeners.clear();
        }

        /**
         * @brief Whether a collider layer id indexes the layer array: 0 <= id < MAX_COLLIDER_LAYERS.
         * Every method taking a layer id checks it first and refuses it otherwise. Comparing the int
         * with the array's size_t size directly turned a negative id into a huge unsigned one: that
         * refused a negative id only by accident, and made RemoveAll's -1 ("every layer") never
         * match.
         * @param nColliderLayerId The id to check;
         * @return Whether it is a valid layer id;
         */
        bool CollisionManager :: IsValidLayerId( int nColliderLayerId )  {

            return ( nColliderLayerId >= 0 ) && ( nColliderLayerId < MAX_COLLIDER_LAYERS );
        }

        /**
         * Add a collider to manager;
         * @param nColliderLayerId collider layer id to add the collider;
         * @param pCollider Pointer to collider to add;
         */
        bool CollisionManager :: AddCollider( int nColliderLayerId,
                                              SunLight :: Collision :: Collider* pCollider )  {

            if( IsValidLayerId( nColliderLayerId ) )  {
                m_ColliderLayerList[nColliderLayerId] -> push_back( pCollider -> GetHandle() );

                return true;
            }

            return false;
        }

        /**
         * Add a collider from manager;
         * Safe to call from within an ICollisionListener::OnCollision()
         * callback fired by this same Update() call: Update() re-validates
         * membership before firing, so a same-frame removal here will not
         * produce a stale/duplicate event for pCollider.
         * @param nColliderLayerId collider layer id to remove the collider;
         * @param pCollider Pointer to collider to remove;
         */
        bool CollisionManager :: RemoveCollider( int nColliderLayerId,
                                                 SunLight :: Collision :: Collider* pCollider )  {

            if( IsValidLayerId( nColliderLayerId ) )  {
                ColliderList   *pColliderList = m_ColliderLayerList[nColliderLayerId].get();
                ColliderList :: iterator itItem = std :: find( pColliderList -> begin(),
                                                            pColliderList -> end(),
                                                            pCollider -> GetHandle() );

                if( itItem != pColliderList -> end() )
                    pColliderList -> erase( itItem );

                return true;
            }

            return false;
        }

        /**
         * Remove all colliders from layer.
         * @param nColliderLayerId collider layer id to remove the all colliders.
         * If this parameter is -1 (default), remove all collider from all layers;
         */
        bool CollisionManager :: RemoveAll( int nColliderLayerId )  {

            if( nColliderLayerId == __ALL_COLLIDER_LAYERS )  {
                for( auto& pColliderList : m_ColliderLayerList )  {
                    pColliderList -> clear();
                }

                return true;
            }

            if( IsValidLayerId( nColliderLayerId ) )  {
                m_ColliderLayerList[nColliderLayerId] -> clear();

                return true;
            }

            return false;
        }

        /**
         * Clear the collider manager object: every layer's colliders, and every rule. Each layer's
         * list is emptied, not destroyed, so the manager stays as usable as a new one: AddCollider,
         * RemoveCollider and Update use every layer's list without checking it, and used to
         * dereference null after a Clear() that destroyed them. The lists themselves are freed with
         * the manager (their unique_ptrs).
         */
        void CollisionManager :: Clear( void )  {

            for( auto& pColliderList : m_ColliderLayerList )  {
                pColliderList -> clear();
            }

            m_ColliderToColliderRuleList.clear();
            m_ColliderToTileLayerRuleList.clear();
        }

        /**
         * Add collider to collider checking rule based on it's layer id.
         * This method pair two layer that will be checked in collision update
         * checking.
         * @param nFirstColliderLayerId First collider layer id that will be added to
         * checking rule;;
         * @param nSecondColliderLayerId Second collider layer id that will be added to
         * checking rule;
         */
        bool CollisionManager :: AddColliderToColliderRule( int nFirstColliderLayerId,
                                                            int nSecondColliderLayerId )  {

            // Both ids are checked before the duplicate search below indexes the layer array with them.
            // Found by Scarab: the search read past the array for an id out of range, and the old check
            // tested the first id twice, so ( 1, 300 ) kept a pointer from past the array in a rule.
            if( !IsValidLayerId( nFirstColliderLayerId ) || !IsValidLayerId( nSecondColliderLayerId ) )
                return false;

            ColliderLayerList& colliderLayerListRef = m_ColliderLayerList;
            ColliderToColliderRuleList :: iterator itItem = std :: find_if( m_ColliderToColliderRuleList.begin(),
                                                                            m_ColliderToColliderRuleList.end(),
                                                                            [nFirstColliderLayerId,
                                                                             nSecondColliderLayerId,
                                                                             &colliderLayerListRef]( const std :: unique_ptr<ColliderPair>& pPair )  {

                                ColliderList* pFirst  = colliderLayerListRef[nFirstColliderLayerId].get();
                                ColliderList* pSecond = colliderLayerListRef[nSecondColliderLayerId].get();

                                return ( ( pPair -> first == pFirst ) && ( pPair -> second == pSecond ) );
                            } );

            if( itItem == m_ColliderToColliderRuleList.end() )  {

                std :: unique_ptr<ColliderPair>  pPair = std :: make_unique<ColliderPair>();

                pPair -> first  = m_ColliderLayerList[nFirstColliderLayerId].get();
                pPair -> second = m_ColliderLayerList[nSecondColliderLayerId].get();

                m_ColliderToColliderRuleList.push_back( std :: move( pPair ) );

                return true;
            }

            return false;
        }

        /**
         * Add collider to tile checking rule based on it's layer id.
         * This method pair two layer that will be checked in collision update
         * checking.
         * @param nColliderLayerId The collider layer id that will be added to
         * checking rule;
         * @param nTileLayerId The tile layer id that will be added to checking rule;
         */
        bool CollisionManager :: AddColliderToTileRule( int nColliderLayerId,
                                                        int nTileLayerId )  {

            SunLight :: TileMap :: stLayer layer;

            if( IsValidLayerId( nColliderLayerId ) &&
                m_pParent -> GetLayer( nTileLayerId, layer ) )  {

                std :: unique_ptr<ColliderTileLayerPair>  pPair = std :: make_unique<ColliderTileLayerPair>();

                pPair -> first  = m_ColliderLayerList[nColliderLayerId].get();
                pPair -> second = nTileLayerId;

                m_ColliderToTileLayerRuleList.push_back( std :: move( pPair ) );

                return true;
            }

            return false;
        }

        /**
         * Add an ICollisionListener event object to manager;
         * @param pListener Pointer to the listener object to add;
         */
        void CollisionManager :: AddCollisionListener( SunLight :: Collision :: ICollisionListener* pListener )  {

            m_Listeners.push_back( pListener );
        }

        /**
         * Check whether handle is still present in pColliderList.
         * Used by Update() to re-validate a snapshotted collider right
         * before firing an event for it, since an earlier listener call
         * within the same Update() may have already removed it (e.g. a
         * listener despawning what it just hit). This turns "don't fire a
         * duplicate/stale hit for an already-removed collider" from a
         * convention every listener must implement itself into a guarantee
         * the manager enforces.
         * @param pColliderList The live layer list to check against;
         * @param handle The collider handle to look for;
         */
        bool CollisionManager :: IsColliderRegistered( ColliderList* pColliderList,
                                                        const ColliderHandle& handle )  {

            return std :: find( pColliderList -> begin(), pColliderList -> end(), handle ) != pColliderList -> end();
        }

        /**
         * Check if there are collisions between objects managed by
         * this collision manager.
         * Must be called every time is needed to check for all objects
         * collision.
         */
        void CollisionManager :: Update( void )  {

            ColliderRegistry&  registry = ColliderRegistry :: Instance();

            /*
            * Check collisions between colliders only.
            *
            * NOTE: pPair->first/second are the live per-layer collider deques
            * that AddCollider/RemoveCollider mutate. FireOnCollision() calls
            * listeners synchronously, and a listener reacting to a hit (e.g.
            * despawning what it just hit) may call RemoveCollider() on the
            * very list being walked here, which invalidates std::deque
            * iterators mid-iteration (UB, observed as a real SIGSEGV). Snapshot
            * each side into a local vector of ColliderHandle (not Collider*)
            * before iterating so a same-frame removal can't invalidate the
            * iteration.
            *
            * Each handle is resolved through ColliderRegistry right before
            * it's dereferenced. Resolve() returns nullptr if the collider it
            * refers to has since been destroyed - e.g. a listener earlier in
            * this same Update() reacted to a hit by deleting the Sprite/
            * Collider outright rather than merely unregistering it - so a
            * stale handle is safely skipped instead of dereferencing freed
            * memory.
            *
            * Re-validating list membership just before firing (below)
            * separately guards against firing a stale/duplicate event for a
            * collider that's still alive but was already removed from this
            * rule by an earlier listener call this Update() - so callers
            * don't each need their own re-entry guard for "did I already
            * handle this despawned entity this frame".
            */
            for( auto& pPair : m_ColliderToColliderRuleList )  {
                std :: vector<ColliderHandle>  firstSnapshot( pPair -> first -> begin(), pPair -> first -> end() );
                std :: vector<ColliderHandle>  secondSnapshot( pPair -> second -> begin(), pPair -> second -> end() );

                for( const ColliderHandle& firstHandle : firstSnapshot )  {
                    Collider  *pFirst = registry.Resolve( firstHandle );

                    if( !pFirst )
                        continue;

                    for( const ColliderHandle& secondHandle : secondSnapshot )  {
                        Collider  *pSecond = registry.Resolve( secondHandle );

                        if( !pSecond )
                            continue;

                        // Collider-to-Collider overload (not the
                        // stDimension2D one) so both sides' own SetInset
                        // shrink apply - passing pSecond->GetDimension2D()
                        // here instead silently ignored pSecond's own
                        // inset entirely, since only pFirst's side was
                        // ever ran through GetEffectiveRect (see Hit(
                        // Collider&)'s own doc comment in collider.cpp).
                        if( pFirst -> Hit( *pSecond ) &&
                            IsColliderRegistered( pPair -> first, firstHandle ) &&
                            IsColliderRegistered( pPair -> second, secondHandle ) )  {
                            FireOnCollision( pFirst, pSecond );
                        }
                    }
                }
            }

            /*
            * Check collisions between colliders against static
            * layer objects defined as collision on layer map.
            * Same snapshot, resolve and re-validation rationale as above
            * applies to pPair->first here.
            */
            for( auto& pPair : m_ColliderToTileLayerRuleList )  {
                std :: vector<ColliderHandle>  firstSnapshot( pPair -> first -> begin(), pPair -> first -> end() );

                for( const ColliderHandle& firstHandle : firstSnapshot )  {
                    Collider  *pFirst = registry.Resolve( firstHandle );

                    if( !pFirst )
                        continue;

                    SunLight :: TileMap :: stLayer                layer;
                    std :: vector<SunLight :: TileMap :: stTile>  tiles;

                    // I7 of the master plan: every tile the collider touches, worked out by the map in map
                    // space (ITileMap::GetCollidingTiles), each reported once and with its map-space
                    // rectangle. It used to test only the tile under the collider's top-left corner, found
                    // through the viewport origin and the camera (see TileMapRenderer::GetCollidingTiles).
                    if( m_pParent -> GetLayer( pPair -> second, layer ) &&
                        m_pParent -> GetCollidingTiles( pFirst -> EffectiveRect(), pFirst -> IsWorldSpace(), layer, tiles ) )  {

                        for( SunLight :: TileMap :: stTile& tile : tiles )  {
                            // A listener may have removed (or destroyed) this collider during an earlier tile's callback.
                            if( !IsColliderRegistered( pPair -> first, firstHandle ) || !registry.Resolve( firstHandle ) )
                                break;

                            FireOnCollision( pFirst, &tile );
                        }
                    }
                }
            }
        }
    }
}
