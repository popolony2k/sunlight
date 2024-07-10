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
