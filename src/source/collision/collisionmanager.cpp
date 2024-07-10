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

#include "collisionmanager.h"
#include <algorithm>


namespace SunLight {
    namespace Collision  {

        /**
         * Throw the OnCollision event through all registered listeners.
         * @param pFirst The first collider involved in the collision;
         * @param pSecond The second collider involved in the collision;
         */
        void CollisionManager :: FireOnCollision( SunLight :: Collision :: Collider *pFirst, 
                                                  SunLight :: Collision :: Collider *pSecond )  {

            for( ICollisionListener *pListener : m_Listeners )  {
                pListener -> OnCollision( pFirst, pSecond );
            }
        }

        /**
         * Throw the OnCollision event through all registered listeners.
         * @param pFirst The collider involved in the collision;
         * @param pSecond The layer tile object involved in the collision;
         */
        void CollisionManager :: FireOnCollision( SunLight :: Collision :: Collider *pFirst, 
                                                  SunLight :: TileMap :: stTile* pSecond )  {

            for( ICollisionListener *pListener : m_Listeners )  {
                pListener -> OnCollision( pFirst, pSecond );
            }
        }

        /**
         * Constructor. Initialize all class data.
         * @param pParent Pointer to the TileMap that this collision
         * manager is attached;
         */
        CollisionManager :: CollisionManager( SunLight :: TileMap :: ITileMap *pParent )  {

            m_pParent = pParent;

            for( int nCount = 0; nCount < m_ColliderLayerList.size(); nCount++)  {
                m_ColliderLayerList[nCount] = new ColliderList();
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
         * Add a collider to manager;
         * @param nColliderLayerId collider layer id to add the collider;
         * @param pCollider Pointer to collider to add;
         */
        bool CollisionManager :: AddCollider( int nColliderLayerId,
                                              SunLight :: Collision :: Collider* pCollider )  {

            if( nColliderLayerId < m_ColliderLayerList.size() )  {
                m_ColliderLayerList[nColliderLayerId] -> push_back( pCollider );

                return true;
            }

            return false;
        }

        /**
         * Add a collider from manager;
         * @param nColliderLayerId collider layer id to remove the collider;
         * @param pCollider Pointer to collider to remove;
         */
        bool CollisionManager :: RemoveCollider( int nColliderLayerId,
                                                 SunLight :: Collision :: Collider *pCollider )  {

            if( nColliderLayerId < m_ColliderLayerList.size() )  {
                ColliderList   *pColliderList = m_ColliderLayerList[nColliderLayerId];
                ColliderList :: iterator itItem = std :: find( pColliderList -> begin(),
                                                            pColliderList -> end(),
                                                            pCollider );

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

            if( nColliderLayerId < m_ColliderLayerList.size() )  {
                if( nColliderLayerId < 0 )  {
                    for( ColliderList *pColliderList : m_ColliderLayerList )  {
                        pColliderList -> clear();
                    }
                }
                else  {
                    m_ColliderLayerList[nColliderLayerId] -> clear();
                }

                return true;
            }

            return false;
        }

        /**
         * Clear the collider manager object (lists status, ....
         */
        void CollisionManager :: Clear( void )  {

            for( int nCount = 0; nCount < m_ColliderLayerList.size(); nCount++)  {
                delete m_ColliderLayerList[nCount];
            }

            for( ColliderPair *pPair : m_ColliderToColliderRuleList )  {
                delete pPair;
            }

            for( ColliderTileLayerPair *pPair : m_ColliderToTileLayerRuleList )  {
                delete pPair;
            }
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

            ColliderLayerList& colliderLayerListRef = m_ColliderLayerList;
            ColliderToColliderRuleList :: iterator itItem = std :: find_if( m_ColliderToColliderRuleList.begin(), 
                                                                            m_ColliderToColliderRuleList.end(), 
                                                                            [nFirstColliderLayerId, 
                                                                             nSecondColliderLayerId, 
                                                                             colliderLayerListRef]( const ColliderPair* pPair ) {
                                
                                ColliderList *pFirst  = colliderLayerListRef[nFirstColliderLayerId];
                                ColliderList *pSecond = colliderLayerListRef[nSecondColliderLayerId];

                                return ( ( pPair -> first == pFirst ) && ( pPair -> second == pSecond ) );
                            } );

            if( ( itItem == m_ColliderToColliderRuleList.end() ) &&
                ( nFirstColliderLayerId < m_ColliderLayerList.size() ) &&
                ( nFirstColliderLayerId < m_ColliderLayerList.size() ) )  {

                ColliderPair  *pPair = new ColliderPair();

                pPair -> first  = m_ColliderLayerList[nFirstColliderLayerId];
                pPair -> second = m_ColliderLayerList[nSecondColliderLayerId];

                m_ColliderToColliderRuleList.push_back( pPair );

                return true;
            }

            return false;
        }

        /**
         * Add collider to tile checking rule based on it's layer id.
         * This method pair two layer that will be checked in collision update
         * checking.
         * @param nColliderId The collider layer id that will be added to
         * checking rule;
         * @param nLayerId The tile layer id that will be added to checking rule;
         */
        bool CollisionManager :: AddColliderToTileRule( int nColliderLayerId,
                                                        int nTileLayerId )  {

            SunLight :: TileMap :: stLayer layer;

            if( ( nColliderLayerId < m_ColliderLayerList.size() ) &&
                m_pParent -> GetLayer( nTileLayerId, layer ) )  {

                ColliderTileLayerPair  *pPair = new ColliderTileLayerPair();

                pPair -> first  = m_ColliderLayerList[nColliderLayerId];
                pPair -> second = nTileLayerId;

                m_ColliderToTileLayerRuleList.push_back( pPair );

                return true;
            }

            return false;
        }

        /**
         * Add an ICollisionListener event object to manager;
         * @param pListener Pointer to the listener object to add;
         */
        void CollisionManager :: AddCollisionListener( SunLight :: Collision :: ICollisionListener *pListener )  {

            m_Listeners.push_back( pListener );
        }

        /**
         * Check if there are collisions between objects managed by
         * this collision manager.
         * Must be called every time is needed to check for all objects
         * collision.
         */
        void CollisionManager :: Update( void )  {

            /*
            * Check collisions between colliders only.
            */
            for( ColliderPair *pPair : m_ColliderToColliderRuleList )  {
                for( Collider *pFirst : *pPair -> first )  {
                    for( Collider *pSecond : *pPair -> second )  {
                        if( pFirst -> Hit( pSecond -> GetDimension2D() ) )  {
                            FireOnCollision( pFirst, pSecond );
                        }
                    }
                }
            }

            /*
            * Check collisions between colliders against static
            * layer objects defined as collision on layer map.
            */
            for( ColliderTileLayerPair *pPair : m_ColliderToTileLayerRuleList )  {
                for( Collider *pFirst : *pPair -> first )  {
                    SunLight :: TileMap :: stTile      tile;
                    SunLight :: TileMap :: stLayer     layer;

                    if( m_pParent -> GetLayer( pPair -> second, layer ) )  {
                        SunLight :: TileMap :: stDimension2D&    spritePos = pFirst -> GetDimension2D();
                        SunLight :: TileMap :: stMatrixPosition  tilePos   = { 0, 0 };

                        if( m_pParent -> TileMapToTileMatrix( spritePos.pos, tilePos ) ) {
                            if( m_pParent -> GetTile( tilePos, layer, tile ) &&
                                pFirst -> Hit( tile ) )  {
                                FireOnCollision(pFirst, &tile );
                            }
                        }
                    }
                }
            }
        }
    }
}
