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

#ifndef __ITILEMAPLISTENER_H__
#define __ITILEMAPLISTENER_H__

#include "tilemap/itilemap.h"


namespace SunLight {
    namespace TileMap  {

        /**
         * @brief TileMap event listener class interface.
         */

        class ITileMapListener  {

            public:

            virtual ~ITileMapListener( void ) {};

            /**
             * @brief Must be implemented to provide action on frame update.
             * @param tileMap The @link ITileMap object used to interact with tilemap
             * renderer implementation;
             */
            virtual void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap ) = 0;

            /**
             * @brief Must be implemented to perform some world renderer clean
             * process when Renderer.Stop() is called;
             */
            virtual void OnStop( void ) = 0;
        };
    }
}
#endif /* __ITILEMAPLISTENER_H__ */
