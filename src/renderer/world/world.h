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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
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

            public:

            // I19d: one map of the world, at its offset (the top-left of its MapPixelSize box, in the
            // world's pixels - as Tiled's .world places a map, whatever its orientation), with the
            // translation between its own layer ids and the world's.
            struct stPlacedMap  {
                std :: unique_ptr<MapData>  pMap;
                int                         nId;
                int                         nX;
                int                         nY;
                std :: map<int, int>        localToWorld;      // its layer id -> the world's
                std :: map<int, int>        worldToLocal;      // the world's -> its first layer of that name
                std :: set<int>             topLevel;          // its own ids of its top-level layers
            };

            // I19d: a world-level layer - one per layer NAME across the world's maps (the first map's
            // ids kept, a name first seen in a later map numbered above the highest), in that order.
            // nTopId: the world id of its top-level ancestor in the map it was first seen in (its own
            // id for a top-level layer) - its sprites are drawn after that layer, across every map.
            struct stWorldLayer  {
                int            nId;
                std :: string  strName;
                int            nTopId;
            };

            private:

            // Declared in this order so the maps are destroyed FIRST, then the collision rules, then
            // the sprite lists - the order the renderer's own members used to be destroyed in.
            SpriteRegistry                             m_Sprites;
            SunLight :: Collision :: CollisionManager  m_CollisionManager;
            std :: vector<stPlacedMap>                 m_Maps;              // in the order they were added
            std :: vector<stWorldLayer>                m_Layers;
            size_t                                     m_nCurrentMap;       // the map Map() answers with (I19d)
            int                                        m_nNextMapId;
            int                                        m_nMaxLayerId;

            void AddLayers( stPlacedMap& placed, tmx_layer* pLayer, int nTopWorldId, bool bFirstMap );
            int IndexOfMap( int nMapId ) const;

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

            // The renderer's own access (I19b; several maps, I19d).
            MapData* Map( void ) const;
            void SetMap( std :: unique_ptr<MapData> pMap );
            bool FreeMap( void );
            int AddMapData( std :: unique_ptr<MapData> pMap, int nX, int nY );
            bool RemoveMapData( int nMapId );
            SpriteRegistry& Sprites( void );
            SunLight :: Collision :: CollisionManager& Collisions( void );

            size_t MapCount( void ) const;
            const stPlacedMap& PlacedMap( size_t nIndex ) const;
            size_t CurrentMapIndex( void ) const;
            void SetCurrentMap( size_t nIndex );
            const std :: vector<stWorldLayer>& Layers( void ) const;
            tmx_layer* FindLayer( int nWorldLayerId ) const;
            tmx_layer* FindLayer( const char* szLayerName ) const;
            tmx_layer* LayerInMap( size_t nIndex, int nWorldLayerId ) const;
            int WorldLayerIdOf( size_t nIndex, int nLocalLayerId ) const;
            int WorldLayerIdOf( tmx_layer* pLayer ) const;
            int MapIndexAt( float fX, float fY ) const;
            SunLight :: Base :: stDimension2D Bounds( void ) const;
            bool PickCell( const SunLight :: Base :: stCoordinate2D& coord, int& nMapId,
                           SunLight :: TileMap :: stMatrixPosition& pos ) const;

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
            int AddMap( const char* szTmxMapFile, int nX, int nY ) override;
            bool RemoveMap( int nMapId ) override;
            int GetMapCount( void ) override;
            bool GetMapOffset( int nMapId, int& nX, int& nY ) override;
            bool GetBounds( SunLight :: Base :: stDimension2D& bounds ) override;
            int GetLayerId( const char* szLayerName ) override;
            bool TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                      int& nMapId,
                                      SunLight :: TileMap :: stMatrixPosition& pos ) override;
            bool GetTile( int nMapId,
                          const SunLight :: TileMap :: stMatrixPosition& pos,
                          int nLayerId,
                          SunLight :: TileMap :: stTile& tile ) override;
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
