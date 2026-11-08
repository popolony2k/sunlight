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
#include "tilemap/iworld.h"
#include "tilemap/itilesource.h"


namespace SunLight  {
    namespace Renderer  {

        // The one forward declaration sunlight keeps here (doc/CODE_STYLE.md), as View's: a World
        // points back at its renderer and the renderer holds its worlds, so tilemaprenderer.h
        // includes this header and this one cannot include it back.
        class TileMapRenderer;

        /**
         * @brief The concrete world (see @see SunLight::TileMap::IWorld, I19 of the master plan): a
         * map, the sprites registered on its layers and the collision rules between them.
         *
         * Like View, a holder of state whose operations RE-USE the renderer's own: each one briefly
         * makes this world the ACTIVE one (the renderer then reads this world's map, sprites and
         * rules), runs the renderer's own method, and swaps back - so a world behaves exactly as the
         * renderer's single-map API does, by construction. It is also its own collision manager's
         * ITileSource, so that manager only ever reads this world's map; it outlives the manager,
         * so that link cannot dangle.
         *
         * A world never owns a sprite (the game does, as SpriteRegistry says) and knows nothing about
         * views, cameras or the screen. Only TileMapRenderer creates worlds.
         */
        class World : public SunLight :: TileMap :: IWorld, public SunLight :: TileMap :: ITileSource  {

            friend class TileMapRenderer;

            TileMapRenderer                            *m_pRenderer;
            int                                        m_nId;

            // Declared in this order so the map is destroyed FIRST, then the collision rules, then
            // the sprite lists - the order the renderer's own members used to be destroyed in.
            SpriteRegistry                             m_Sprites;
            SunLight :: Collision :: CollisionManager  m_CollisionManager;
            std :: unique_ptr<MapData>                 m_pMap;

            template<class Function>
            void Run( Function function );

            // Called by the renderer when this world is removed, and for every world when the
            // renderer is destroyed: cut the link so a handle still held is inert, not dangling.
            void Detach( void );

            public:

            World( TileMapRenderer* pRenderer, int nId );
            virtual ~World( void );
            World( const World& ) = delete;
            World& operator=( const World& ) = delete;

            // The renderer's own access (I19b).
            MapData* Map( void ) const;
            void SetMap( std :: unique_ptr<MapData> pMap );
            bool FreeMap( void );
            SpriteRegistry& Sprites( void );
            SunLight :: Collision :: CollisionManager& Collisions( void );

            // IWorld
            int GetId( void ) override;
            bool IsRemoved( void ) override;
            bool LoadMap( const char* szTmxMapFile ) override;
            bool UnloadMap( void ) override;
            bool GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo ) override;
            bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer ) override;
            bool GetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer ) override;
            bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                          const SunLight :: TileMap :: stLayer& layer,
                          SunLight :: TileMap :: stTile& tile ) override;
            bool TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                      SunLight :: TileMap :: stMatrixPosition& pos ) override;
            bool AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) override;
            bool RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) override;
            SunLight :: Collision :: ICollisionManager& GetCollisionManager( void ) override;

            // ITileSource (GetLayer( int, stLayer& ) above serves both)
            bool GetCollidingTiles( const SunLight :: Base :: stRectangle& rect,
                                    bool bWorldSpace,
                                    const SunLight :: TileMap :: stLayer& layer,
                                    std :: vector<SunLight :: TileMap :: stTile>& tiles ) override;
        };
    }
}

#endif /* __RENDERER_WORLD_H__ */
