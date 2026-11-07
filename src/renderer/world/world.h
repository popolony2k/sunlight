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

// Not __WORLD_H__: ten samples have a world.h of their own with that guard, and whichever was
// included first hid the other (found building the samples, I19b).
#ifndef __RENDERER_WORLD_H__
#define __RENDERER_WORLD_H__

#include <memory>
#include "collision/collisionmanager.h"
#include "renderer/map/mapdata.h"
#include "renderer/sprite/spriteregistry.h"
#include "tilemap/itilemap.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief A world (I19 of the master plan): what a game's things live in - its map, the
         * sprites registered on its layers and the collision rules between them. A view shows a
         * world; several worlds will be alive at once, and a Tiled .world file will be one world of
         * several maps. Today (I19b) a renderer has exactly one, its DEFAULT world, which holds
         * what the renderer itself used to hold (the map, its SpriteRegistry, its
         * CollisionManager), so nothing about any game changes.
         *
         * A world never owns a sprite (the game does, as SpriteRegistry says) and knows nothing about
         * views, cameras or the screen: TileMapRenderer draws it and keeps the checks around it.
         */
        class World  {

            // Declared in this order so the map is destroyed FIRST, then the collision rules, then
            // the sprite lists - the order the renderer's own members used to be destroyed in.
            SpriteRegistry                             m_Sprites;
            SunLight :: Collision :: CollisionManager  m_CollisionManager;
            std :: unique_ptr<MapData>                 m_pMap;

            public:

            explicit World( SunLight :: TileMap :: ITileMap* pCollisionParent );
            ~World( void );
            World( const World& ) = delete;
            World& operator=( const World& ) = delete;

            MapData* Map( void ) const;
            void SetMap( std :: unique_ptr<MapData> pMap );
            bool UnloadMap( void );

            SpriteRegistry& Sprites( void );
            SunLight :: Collision :: CollisionManager& Collisions( void );
        };
    }
}

#endif /* __RENDERER_WORLD_H__ */
