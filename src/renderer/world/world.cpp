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
#include "renderer/tilemaprenderer.h"
#include <utility>

namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief An empty world of a renderer: no map, no sprites, no collision rules. Its collision
         * manager asks the world itself for layers and tiles (ITileSource).
         * @param pRenderer The renderer it belongs to; nullptr makes a world that is inert from the start;
         * @param nId 0 for the renderer's default world, otherwise the id CreateWorld gave it;
         */
        World :: World( TileMapRenderer* pRenderer, int nId ) : m_pRenderer( pRenderer ),
                                                                m_nId( nId ),
                                                                m_CollisionManager( this )  {
        }

        /**
         * @brief Frees the map, if one is still loaded (its textures, Resource Manager and tile
         * animations, through MapData's own destructor). The sprites are the game's: the world only
         * drops its lists of them.
         */
        World :: ~World( void )  {
        }

        /**
         * Run one of the renderer's own operations with THIS world active: swap the renderer's
         * working world to this one, call it, swap back. Nothing at all once detached.
         * @param function The operation;
         */
        template<class Function>
        void World :: Run( Function function )  {

            if( !m_pRenderer )
                return;

            World  *pPrevious = m_pRenderer -> ActivateWorld( this );

            function();

            m_pRenderer -> ActivateWorld( pPrevious );
        }

        /**
         * @brief Cut the world loose from its renderer: every operation that would act through it
         * does nothing or answers "no" from now on (see IWorld::IsRemoved). Idempotent.
         */
        void World :: Detach( void )  {

            m_pRenderer = nullptr;
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
         * @brief Free the world's map, if it has one, and only that: its sprites and collision rules
         * are left as they are - the renderer decides what happens to them (TileMapRenderer::UnloadMap,
         * UnloadWorldMap, RemoveWorld).
         * @return true when there was a map to free;
         */
        bool World :: FreeMap( void )  {

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

        /**
         * @brief The world's id.
         * @return 0 for the default world, otherwise the one CreateWorld gave it;
         */
        int World :: GetId( void )  {

            return m_nId;
        }

        /**
         * @brief Whether the world was cut loose (Detach): removed, or its renderer destroyed.
         * @return true once inert;
         */
        bool World :: IsRemoved( void )  {

            return ( m_pRenderer == nullptr );
        }

        /**
         * @brief Load a map into this world (TileMapRenderer::LoadWorldMap: no camera moves, the
         * scroll steps of the views showing it are resolved).
         * @param szTmxMapFile The map, a FileSystem path;
         * @return false when not started, the map cannot be loaded, or once removed;
         */
        bool World :: LoadMap( const char* szTmxMapFile )  {

            return m_pRenderer ? m_pRenderer -> LoadWorldMap( *this, szTmxMapFile ) : false;
        }

        /**
         * @brief Free the world's map and detach its sprites (TileMapRenderer::UnloadWorldMap).
         * @return false with no map, or once removed;
         */
        bool World :: UnloadMap( void )  {

            return m_pRenderer ? m_pRenderer -> UnloadWorldMap( *this ) : false;
        }

        /**
         * @brief The renderer's own GetMapInfo, with this world active.
         * @param mapInfo Receives the information;
         * @return false with no map, or once removed;
         */
        bool World :: GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo )  {

            bool  bResult = false;

            Run( [this, &mapInfo, &bResult]() { bResult = m_pRenderer -> GetMapInfo( mapInfo ); } );

            return bResult;
        }

        /**
         * @brief The renderer's own GetLayer by id, with this world active - also what this world's
         * collision manager asks (ITileSource).
         * @param nLayerId The layer's id;
         * @param layer Receives the layer;
         * @return false with no such layer, no map, or once removed;
         */
        bool World :: GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer )  {

            bool  bResult = false;

            Run( [this, nLayerId, &layer, &bResult]() { bResult = m_pRenderer -> GetLayer( nLayerId, layer ); } );

            return bResult;
        }

        /**
         * @brief The renderer's own GetLayer by name, with this world active.
         * @param szLayerName The layer's name;
         * @param layer Receives the layer;
         * @return false with no such layer, no map, or once removed;
         */
        bool World :: GetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer )  {

            bool  bResult = false;

            Run( [this, szLayerName, &layer, &bResult]() { bResult = m_pRenderer -> GetLayer( szLayerName, layer ); } );

            return bResult;
        }

        /**
         * @brief The renderer's own GetTile, with this world active.
         * @param pos The cell;
         * @param layer The tile layer;
         * @param tile Receives the tile, its rectangle in this world's map pixels;
         * @return false with no tile there, no map, or once removed;
         */
        bool World :: GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                               const SunLight :: TileMap :: stLayer& layer,
                               SunLight :: TileMap :: stTile& tile )  {

            bool  bResult = false;

            Run( [this, &pos, &layer, &tile, &bResult]() { bResult = m_pRenderer -> GetTile( pos, layer, tile ); } );

            return bResult;
        }

        /**
         * @brief The renderer's own TileMapToTileMatrix, with this world active.
         * @param coord The position, in this world's map pixels;
         * @param pos Receives the cell;
         * @return false off the map's cells, with no map, or once removed;
         */
        bool World :: TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                           SunLight :: TileMap :: stMatrixPosition& pos )  {

            bool  bResult = false;

            Run( [this, &coord, &pos, &bResult]() { bResult = m_pRenderer -> TileMapToTileMatrix( coord, pos ); } );

            return bResult;
        }

        /**
         * @brief The renderer's own AddSprite, with this world active (it refuses a sprite another
         * world holds).
         * @param nLayerId The layer;
         * @param sprite The sprite;
         * @return false when refused, or once removed;
         */
        bool World :: AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite )  {

            bool  bResult = false;

            Run( [this, nLayerId, &sprite, &bResult]() { bResult = m_pRenderer -> AddSprite( nLayerId, sprite ); } );

            return bResult;
        }

        /**
         * @brief The renderer's own RemoveSprite, with this world active.
         * @param nLayerId The layer;
         * @param sprite The sprite;
         * @return false when it was not registered there, or once removed;
         */
        bool World :: RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite )  {

            bool  bResult = false;

            Run( [this, nLayerId, &sprite, &bResult]() { bResult = m_pRenderer -> RemoveSprite( nLayerId, sprite ); } );

            return bResult;
        }

        /**
         * @brief The world's own collision manager, whatever the world's state (a removed world's
         * rules are never tested again).
         * @return It;
         */
        SunLight :: Collision :: ICollisionManager& World :: GetCollisionManager( void )  {

            return m_CollisionManager;
        }

        /**
         * @brief The tiles a collider touches in this world's map (ITileSource, what this world's
         * collision manager asks): TileMapRenderer::CollidingTilesIn, which activates this world and
         * a view that shows it, for the camera a view-relative collider is drawn with.
         * @param rect The collider's rectangle;
         * @param bWorldSpace Whether rect is already a map position;
         * @param layer The tile layer;
         * @param tiles Receives the tiles touched;
         * @return Whether the layer could be tested; false once removed;
         */
        bool World :: GetCollidingTiles( const SunLight :: Base :: stRectangle& rect,
                                         bool bWorldSpace,
                                         const SunLight :: TileMap :: stLayer& layer,
                                         std :: vector<SunLight :: TileMap :: stTile>& tiles )  {

            if( !m_pRenderer )  {
                tiles.clear();
                return false;
            }

            return m_pRenderer -> CollidingTilesIn( *this, rect, bWorldSpace, layer, tiles );
        }
    }
}
