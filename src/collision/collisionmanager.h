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
