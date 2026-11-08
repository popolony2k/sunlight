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

#include "renderer/world/world.h"
#include <utility>

namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief An empty world: no map, no sprites, no collision rules.
         * @param pCollisionParent What the world's CollisionManager asks for layers and the tiles a
         * collider touches (ITileMap::GetLayer, GetCollidingTiles) - the renderer, for now (I19b);
         */
        World :: World( SunLight :: TileMap :: ITileMap* pCollisionParent ) : m_CollisionManager( pCollisionParent )  {
        }

        /**
         * @brief Frees the map, if one is still loaded (its textures, Resource Manager and tile
         * animations, through MapData's own destructor). The sprites are the game's: the world only
         * drops its lists of them.
         */
        World :: ~World( void )  {
        }

        /**
         * @brief The world's map.
         * @return The map, or nullptr when none is loaded;
         */
        MapData* World :: Map( void ) const  {

            return m_pMap.get();
        }

        /**
         * @brief Make a newly loaded map the world's, freeing the one it had (exactly once: MapData
         * frees everything in its destructor).
         * @param pMap The map, already loaded (MapData::Load); nullptr leaves the world without one;
         */
        void World :: SetMap( std :: unique_ptr<MapData> pMap )  {

            m_pMap = std :: move( pMap );
        }

        /**
         * @brief Free the world's map, if it has one. Its sprites and collision rules are left as
         * they are: the renderer decides what happens to them (TileMapRenderer::UnloadMap).
         * @return true when there was a map to free;
         */
        bool World :: UnloadMap( void )  {

            if( !m_pMap )
                return false;

            m_pMap.reset();

            return true;
        }

        /**
         * @brief The sprites registered on the world's layers (see SpriteRegistry).
         * @return The registry;
         */
        SpriteRegistry& World :: Sprites( void )  {

            return m_Sprites;
        }

        /**
         * @brief The world's collision rules and colliders.
         * @return Its CollisionManager;
         */
        SunLight :: Collision :: CollisionManager& World :: Collisions( void )  {

            return m_CollisionManager;
        }
    }
}
