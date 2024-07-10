/*
 * collisionmanager.h
 *
 *  Created on: Jul 16, 2021
 *      Author: popolony2k
 */

#ifndef __COLLISIONMANAGER_H__
#define __COLLISIONMANAGER_H__

#include <queue>
#include <array>
#include "collision/icollisionmanager.h"
#include "tilemap/itilemap.h"

#define MAX_COLLIDER_LAYERS   255


namespace SunLight {
    namespace Collision  {

        /**
         * @brief CollisionManager implementation. 
         * 
         */
        class CollisionManager : public SunLight :: Base :: Object, public ICollisionManager  {

            typedef std :: deque<Collider*>  ColliderList;
            typedef std :: array<ColliderList*, MAX_COLLIDER_LAYERS> ColliderLayerList;
            typedef std :: deque<ICollisionListener*> CollisionListenerList;
            typedef std :: pair<ColliderList*, ColliderList*> ColliderPair;
            typedef std :: deque<ColliderPair*> ColliderToColliderRuleList;
            typedef std :: pair<ColliderList*, int> ColliderTileLayerPair;
            typedef std :: deque<ColliderTileLayerPair*> ColliderToTileLayerRuleList;

            SunLight :: TileMap :: ITileMap   *m_pParent;
            ColliderLayerList                 m_ColliderLayerList;
            ColliderToColliderRuleList        m_ColliderToColliderRuleList;
            ColliderToTileLayerRuleList       m_ColliderToTileLayerRuleList;
            CollisionListenerList             m_Listeners;


            void FireOnCollision( SunLight :: Collision :: Collider *pFirst, 
                                  SunLight :: Collision :: Collider *pSecond );
            void FireOnCollision( SunLight :: Collision :: Collider *pFirst, 
                                  SunLight :: TileMap :: stTile* pSecond );

            public:

            CollisionManager( SunLight :: TileMap :: ITileMap  *pParent );
            virtual ~CollisionManager( void );

            bool AddCollider( int nColliderLayerId,
                              SunLight :: Collision :: Collider* pCollider );
            bool RemoveCollider( int nColliderLayerId,
                                 SunLight :: Collision :: Collider* pCollider );
            bool RemoveAll( int nColliderLayerId = -1 );
            void Clear( void );

            bool AddColliderToColliderRule( int nFirstColliderLayerId,
                                            int nSecondColliderLayerId );
            bool AddColliderToTileRule( int nColliderLayerId,
                                        int nTileLayerId );
            void AddCollisionListener( SunLight :: Collision :: ICollisionListener *pListener );

            void Update( void );
        };
    }
}

#endif /* __COLLISIONMANAGER_H__ */
