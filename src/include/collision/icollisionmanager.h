/*
 * icollisionmanager.h
 *
 *  Created on: Apr 25, 2022
 *      Author: popolony2k
 */

#ifndef __ICOLLISIONMANAGER_H__
#define __ICOLLISIONMANAGER_H__

#include "collision/icollisionlistener.h"


namespace SunLight {
    namespace Collision  {
        /**
         * @brief Collision manager base interface.
         * 
         */
        class ICollisionManager  {

            public:

            /**
             * Add a collider to manager;
             * @param nColliderLayerId collider layer id to add the collider;
             * @param pCollider Pointer to collider to add;
             */
            virtual bool AddCollider( int nColliderLayerId, SunLight :: Collision :: Collider* pCollider ) = 0;

            /**
             * Add a collider from manager;
             * @param nColliderLayerId collider layer id to remove the collider;
             * @param pCollider Pointer to collider to remove;
             */
            virtual bool RemoveCollider( int nColliderLayerId, SunLight :: Collision :: Collider* pCollider ) = 0;

            /**
             * Remove all colliders from layer.
             * @param nColliderLayerId collider layer id to remove the all colliders.
             * If this parameter is -1 (default), remove all collider from all layers;
             */
            virtual bool RemoveAll( int nColliderLayerId = -1 ) = 0;

            /**
             * Clear the collider manager object (lists status, ....
             */
            virtual void Clear( void ) = 0;

            /**
             * Add collider to collider checking rule based on it's layer id.
             * This method pair two layer that will be checked in collision update
             * checking.
             * @param nFirstColliderLayerId First collider layer id that will be added to
             * checking rule;;
             * @param nSecondColliderLayerId Second collider layer id that will be added to
             * checking rule;
             */
            virtual bool AddColliderToColliderRule( int nFirstColliderLayerId,
                                                    int nSecondColliderLayerId ) = 0;

            /**
             * Add collider to tile checking rule based on it's layer id.
             * This method pair two layer that will be checked in collision update
             * checking.
             * @param nColliderId The collider layer id that will be added to
             * checking rule;
             * @param nLayerId The tile layer id that will be added to checking rule;
             */
            virtual bool AddColliderToTileRule( int nColliderLayerId,
                                                int nTileLayerId ) = 0;

            /**
             * Add an ICollisionListener event object to manager;
             * @param pListener Pointer to the listener object to add;
             */
            virtual void AddCollisionListener( SunLight :: Collision :: ICollisionListener *pListener ) = 0;

            /**
             * Check if there are collisions between objects managed by
             * this collision manager.
             * Must be called every time is needed to check for all objects
             * collision.
             */
            virtual void Update( void ) = 0;
        };

    }
}

#endif  /* __ICOLLISIONMANAGER_H__ */
