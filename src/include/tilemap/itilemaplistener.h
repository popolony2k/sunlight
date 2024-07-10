/*
 * itilemaplistener.h
 *
 *  Created on: Jun 30, 2021
 *      Author: popolony2k
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
