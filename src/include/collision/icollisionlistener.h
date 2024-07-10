/*
 * icollisionlistener.h
 *
 *  Created on: Jul 20, 2021
 *      Author: popolony2k
 */

#ifndef __ICOLLISIONLISTENER_H__
#define __ICOLLISIONLISTENER_H__

#include "collider.h"


namespace SunLight {
    namespace Collision  {

        /**
         * @brief Collision listener interface. 
         * 
         */
        class ICollisionListener  {

            public:

            virtual ~ICollisionListener( void ) {};

            /**
             * Event thrown when two colliders hit one to another.
             * @param pFirst The first collider involved in the collision;
             * @param pSecond The second collider involved in the collision;
             */
            virtual void OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: Collision :: Collider *pSecond ) = 0;

            /**
             * Event thrown when two colliders hit one to another.
             * @param pFirst The collider involved in the collision;
             * @param pSecond The layer tile object involved in the collision;
             */
            virtual void OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: TileMap :: stTile *pSecond ) = 0;
        };
    }
}

#endif /* __ICOLLISIONLISTENER_H__ */
