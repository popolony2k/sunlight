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
#include <algorithm>
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
                                                                m_CollisionManager( this ),
                                                                m_nCurrentMap( 0 ),
                                                                m_nNextMapId( 0 ),
                                                                m_nMaxLayerId( 0 )  {
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
         * @brief The world's CURRENT map (I19d): the first one, except while the renderer works on
         * another (SetCurrentMap - a draw pass, a collision test). With one map, simply its map.
         * @return The map, or nullptr when the world has none;
         */
        MapData* World :: Map( void ) const  {

            return m_Maps.empty() ? nullptr : m_Maps[m_nCurrentMap].pMap.get();
        }

        /**
         * @brief Make a newly loaded map the world's ONLY map, at (0, 0), freeing every map it had
         * (exactly once: MapData frees everything in its destructor). Its layer ids are the world's.
         * @param pMap The map, already loaded (MapData::Load); nullptr leaves the world without one;
         */
        void World :: SetMap( std :: unique_ptr<MapData> pMap )  {

            FreeMap();

            if( pMap )
                AddMapData( std :: move( pMap ), 0, 0 );
        }

        /**
         * @brief Free every map of the world, and only those: its sprites and collision rules are
         * left as they are - the renderer decides what happens to them (TileMapRenderer::UnloadMap,
         * UnloadWorldMap, RemoveWorld). The layer table goes with the maps.
         * @return true when there was a map to free;
         */
        bool World :: FreeMap( void )  {

            if( m_Maps.empty() )
                return false;

            m_Maps.clear();
            m_Layers.clear();
            m_nCurrentMap = 0;
            m_nMaxLayerId = 0;

            return true;
        }

        /**
         * @brief Walk a map's layers, groups included, depth first in document order, giving each
         * one its world layer id (I19d): the first map's layers keep their own ids; a later map's
         * layer takes the world id of the first layer of the same NAME, or a new one above the
         * highest so far.
         * @param placed The map being added;
         * @param pLayer The first layer of the list to walk;
         * @param nTopWorldId The world id of the top-level layer above this list (-1 at the top);
         * @param bFirstMap Whether this is the world's first map;
         */
        void World :: AddLayers( stPlacedMap& placed, tmx_layer* pLayer, int nTopWorldId, bool bFirstMap )  {

            for( ; pLayer; pLayer = pLayer -> next )  {
                int            nLocal   = ( int ) pLayer -> id;
                std :: string  strName  = pLayer -> name ? pLayer -> name : "";
                int            nWorldId = -1;

                if( !bFirstMap )  {
                    for( const stWorldLayer& entry : m_Layers )  {
                        if( entry.strName == strName )  {
                            nWorldId = entry.nId;
                            break;
                        }
                    }
                }

                if( nWorldId < 0 )  {
                    nWorldId = bFirstMap ? nLocal : ( m_nMaxLayerId + 1 );
                    m_Layers.push_back( stWorldLayer { nWorldId, strName, ( nTopWorldId < 0 ) ? nWorldId : nTopWorldId } );
                }

                m_nMaxLayerId = std :: max( m_nMaxLayerId, nWorldId );
                placed.localToWorld[nLocal] = nWorldId;

                if( placed.worldToLocal.find( nWorldId ) == placed.worldToLocal.end() )
                    placed.worldToLocal[nWorldId] = nLocal;

                if( nTopWorldId < 0 )
                    placed.topLevel.insert( nLocal );

                if( pLayer -> type == L_GROUP )
                    AddLayers( placed, pLayer -> content.group_head, ( nTopWorldId < 0 ) ? nWorldId : nTopWorldId, bFirstMap );
            }
        }

        /**
         * @brief Add a loaded map to the world at an offset (I19d); a world without maps takes it as
         * its first, whose layer ids become the world's.
         * @param pMap The map, already loaded;
         * @param nX Its left edge, in the world's pixels;
         * @param nY Its top edge;
         * @return Its id in the world (never reused); -1 for an empty pointer;
         */
        int World :: AddMapData( std :: unique_ptr<MapData> pMap, int nX, int nY )  {

            if( !pMap )
                return -1;

            bool         bFirst = m_Maps.empty();
            stPlacedMap  placed;

            placed.nId = m_nNextMapId++;
            placed.nX  = nX;
            placed.nY  = nY;
            AddLayers( placed, pMap -> Tmx() -> ly_head, -1, bFirst );
            placed.pMap = std :: move( pMap );
            m_Maps.push_back( std :: move( placed ) );

            return m_Maps.back().nId;
        }

        /**
         * @brief Free one map of the world (I19d). The layer table stays while maps remain (ids never
         * change under a game's sprites and rules); the last map takes it with it.
         * @param nMapId The map's id;
         * @return false when there is no such map;
         */
        bool World :: RemoveMapData( int nMapId )  {

            int  nIndex = IndexOfMap( nMapId );

            if( nIndex < 0 )
                return false;

            if( m_Maps.size() == 1 )
                return FreeMap();

            m_Maps.erase( m_Maps.begin() + nIndex );
            m_nCurrentMap = 0;

            return true;
        }

        /**
         * @brief Where a map is in m_Maps.
         * @param nMapId The map's id;
         * @return Its index, or -1;
         */
        int World :: IndexOfMap( int nMapId ) const  {

            for( size_t nIndex = 0; nIndex < m_Maps.size(); nIndex++ )  {
                if( m_Maps[nIndex].nId == nMapId )
                    return ( int ) nIndex;
            }

            return -1;
        }

        /**
         * @brief How many maps the world holds (I19d).
         * @return The count;
         */
        size_t World :: MapCount( void ) const  {

            return m_Maps.size();
        }

        /**
         * @brief One of the world's maps, by position in the order they were added.
         * @param nIndex Below MapCount();
         * @return It;
         */
        const World :: stPlacedMap& World :: PlacedMap( size_t nIndex ) const  {

            return m_Maps[nIndex];
        }

        /**
         * @brief Which map Map() answers with now.
         * @return Its index;
         */
        size_t World :: CurrentMapIndex( void ) const  {

            return m_nCurrentMap;
        }

        /**
         * @brief Make Map() answer with another of the world's maps (the renderer, around a draw or a
         * collision test - it puts the first one back). An index out of range is ignored.
         * @param nIndex The map's index;
         */
        void World :: SetCurrentMap( size_t nIndex )  {

            if( nIndex < m_Maps.size() )
                m_nCurrentMap = nIndex;
        }

        /**
         * @brief The world's layers, one per name, in drawing order (I19d).
         * @return The table;
         */
        const std :: vector<World :: stWorldLayer>& World :: Layers( void ) const  {

            return m_Layers;
        }

        /**
         * @brief A world layer in the first map that has it.
         * @param nWorldLayerId The world layer id;
         * @return The layer, or nullptr;
         */
        tmx_layer* World :: FindLayer( int nWorldLayerId ) const  {

            for( size_t nIndex = 0; nIndex < m_Maps.size(); nIndex++ )  {
                tmx_layer  *pLayer = LayerInMap( nIndex, nWorldLayerId );

                if( pLayer )
                    return pLayer;
            }

            return nullptr;
        }

        /**
         * @brief A layer by name, in the first map that has it.
         * @param szLayerName The name;
         * @return The layer, or nullptr;
         */
        tmx_layer* World :: FindLayer( const char* szLayerName ) const  {

            for( const stPlacedMap& placed : m_Maps )  {
                tmx_layer  *pLayer = placed.pMap -> FindLayer( szLayerName );

                if( pLayer )
                    return pLayer;
            }

            return nullptr;
        }

        /**
         * @brief One map's layer for a world layer id: its first layer of that name.
         * @param nIndex The map's index;
         * @param nWorldLayerId The world layer id;
         * @return The layer, or nullptr when that map has none;
         */
        tmx_layer* World :: LayerInMap( size_t nIndex, int nWorldLayerId ) const  {

            if( nIndex >= m_Maps.size() )
                return nullptr;

            const stPlacedMap               &placed = m_Maps[nIndex];
            std :: map<int, int> :: const_iterator  itLocal = placed.worldToLocal.find( nWorldLayerId );

            return ( itLocal == placed.worldToLocal.end() ) ? nullptr : placed.pMap -> FindLayer( itLocal -> second );
        }

        /**
         * @brief A map's own layer id as the world's.
         * @param nIndex The map's index;
         * @param nLocalLayerId Its layer id in that map;
         * @return The world layer id; the id itself when unknown (a world without that map);
         */
        int World :: WorldLayerIdOf( size_t nIndex, int nLocalLayerId ) const  {

            if( nIndex >= m_Maps.size() )
                return nLocalLayerId;

            std :: map<int, int> :: const_iterator  itWorld = m_Maps[nIndex].localToWorld.find( nLocalLayerId );

            return ( itWorld == m_Maps[nIndex].localToWorld.end() ) ? nLocalLayerId : itWorld -> second;
        }

        /**
         * @brief The world layer id of a layer of any of the world's maps (found by pointer).
         * @param pLayer The layer;
         * @return Its world layer id, or -1 when no map of the world holds it;
         */
        int World :: WorldLayerIdOf( tmx_layer* pLayer ) const  {

            if( !pLayer )
                return -1;

            for( size_t nIndex = 0; nIndex < m_Maps.size(); nIndex++ )  {
                if( m_Maps[nIndex].pMap -> FindLayer( ( int ) pLayer -> id ) == pLayer )
                    return WorldLayerIdOf( nIndex, ( int ) pLayer -> id );
            }

            return -1;
        }

        /**
         * @brief The first map, in the order they were added, whose pixel box holds a world position.
         * @param fX The position's x, in the world's pixels;
         * @param fY Its y;
         * @return The map's index, or -1;
         */
        int World :: MapIndexAt( float fX, float fY ) const  {

            for( size_t nIndex = 0; nIndex < m_Maps.size(); nIndex++ )  {
                const stPlacedMap  &placed = m_Maps[nIndex];

                if( ( fX >= placed.nX ) && ( fX < placed.nX + placed.pMap -> PixelWidth() ) &&
                    ( fY >= placed.nY ) && ( fY < placed.nY + placed.pMap -> PixelHeight() ) )
                    return ( int ) nIndex;
            }

            return -1;
        }

        /**
         * @brief The rectangle every map's pixel box fits in, in the world's pixels (I19d); empty at
         * (0, 0) with no map. With one map at (0, 0): exactly its pixel size, as before.
         * @return The rectangle;
         */
        SunLight :: Base :: stDimension2D World :: Bounds( void ) const  {

            if( m_Maps.empty() )
                return SunLight :: Base :: stDimension2D {};

            int  nLeft   = m_Maps[0].nX;
            int  nTop    = m_Maps[0].nY;
            int  nRight  = m_Maps[0].nX + m_Maps[0].pMap -> PixelWidth();
            int  nBottom = m_Maps[0].nY + m_Maps[0].pMap -> PixelHeight();

            for( const stPlacedMap& placed : m_Maps )  {
                nLeft   = std :: min( nLeft, placed.nX );
                nTop    = std :: min( nTop, placed.nY );
                nRight  = std :: max( nRight, placed.nX + ( int ) placed.pMap -> PixelWidth() );
                nBottom = std :: max( nBottom, placed.nY + ( int ) placed.pMap -> PixelHeight() );
            }

            return SunLight :: Base :: stDimension2D { { nLeft, nTop }, { nRight - nLeft, nBottom - nTop } };
        }

        /**
         * @brief The map and cell under a world position (I19d): each map, in the order they were
         * added, takes the position less its offset through its own projection's hit-test
         * (MapToTileMatrix, I17) - so an isometric map's empty corners fall through to the next map -
         * and must land inside its grid (A6).
         * @param coord The position, in the world's pixels;
         * @param nMapId Receives the map's id;
         * @param pos Receives the cell;
         * @return false when no map has a cell there;
         */
        bool World :: PickCell( const SunLight :: Base :: stCoordinate2D& coord, int& nMapId,
                                SunLight :: TileMap :: stMatrixPosition& pos ) const  {

            for( const stPlacedMap& placed : m_Maps )  {
                SunLight :: Base :: stCoordinate2D       local { coord.x - placed.nX, coord.y - placed.nY };
                SunLight :: TileMap :: stMatrixPosition  cell { -1, -1 };
                tmx_map                                  *pTmx = placed.pMap -> Tmx();

                if( placed.pMap -> MapProjection().MapToTileMatrix( local, pTmx, placed.pMap -> PixelWidth(),
                                                                    placed.pMap -> PixelHeight(), cell ) &&
                    ( cell.nTileCol >= 0 ) && ( cell.nTileRow >= 0 ) &&
                    ( cell.nTileCol < ( int ) pTmx -> width ) && ( cell.nTileRow < ( int ) pTmx -> height ) )  {
                    nMapId = placed.nId;
                    pos    = cell;

                    return true;
                }
            }

            return false;
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
         * @brief Load a map and place it in the world at an offset (TileMapRenderer::AddWorldMap).
         * @param szTmxMapFile The map, a FileSystem path;
         * @param nX Its left edge, in the world's pixels;
         * @param nY Its top edge;
         * @return Its id; -1 when not started, it cannot be loaded, or once removed;
         */
        int World :: AddMap( const char* szTmxMapFile, int nX, int nY )  {

            return m_pRenderer ? m_pRenderer -> AddWorldMap( *this, szTmxMapFile, nX, nY ) : -1;
        }

        /**
         * @brief Load a Tiled .world file into the world, all or nothing
         * (TileMapRenderer::LoadWorldFile).
         * @param szWorldFile The .world file, a FileSystem path;
         * @return false when not started, on any failure, or once removed;
         */
        bool World :: LoadWorld( const char* szWorldFile )  {

            return m_pRenderer ? m_pRenderer -> LoadWorldFile( *this, szWorldFile ) : false;
        }

        /**
         * @brief Free one of the world's maps (RemoveMapData; its sprites stay registered).
         * @param nMapId The map's id;
         * @return false with no such map, or once removed;
         */
        bool World :: RemoveMap( int nMapId )  {

            return m_pRenderer ? RemoveMapData( nMapId ) : false;
        }

        /**
         * @brief How many maps the world holds.
         * @return The count (whatever the world's state);
         */
        int World :: GetMapCount( void )  {

            return ( int ) m_Maps.size();
        }

        /**
         * @brief A map's offset in the world.
         * @param nMapId The map's id;
         * @param nX Receives its left edge;
         * @param nY Receives its top edge;
         * @return false with no such map, or once removed;
         */
        bool World :: GetMapOffset( int nMapId, int& nX, int& nY )  {

            int  nIndex = IndexOfMap( nMapId );

            if( !m_pRenderer || ( nIndex < 0 ) )
                return false;

            nX = m_Maps[nIndex].nX;
            nY = m_Maps[nIndex].nY;

            return true;
        }

        /**
         * @brief The rectangle every map of the world fits in (Bounds).
         * @param bounds Receives it;
         * @return false with no map, or once removed;
         */
        bool World :: GetBounds( SunLight :: Base :: stDimension2D& bounds )  {

            if( !m_pRenderer || m_Maps.empty() )
                return false;

            bounds = Bounds();

            return true;
        }

        /**
         * @brief The world layer id of a name: the first entry of the layer table with it.
         * @param szLayerName The name;
         * @return The id; -1 with no such name, no name, or once removed;
         */
        int World :: GetLayerId( const char* szLayerName )  {

            if( !m_pRenderer || !szLayerName )
                return -1;

            for( const stWorldLayer& entry : m_Layers )  {
                if( entry.strName == szLayerName )
                    return entry.nId;
            }

            return -1;
        }

        /**
         * @brief The map and cell under a world position (PickCell).
         * @param coord The position, in the world's pixels;
         * @param nMapId Receives the map's id;
         * @param pos Receives the cell;
         * @return false when no map has a cell there, or once removed;
         */
        bool World :: TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                           int& nMapId,
                                           SunLight :: TileMap :: stMatrixPosition& pos )  {

            return m_pRenderer ? PickCell( coord, nMapId, pos ) : false;
        }

        /**
         * @brief A tile of one of the world's maps, its rectangle in the world's pixels
         * (TileMapRenderer::WorldTile).
         * @param nMapId The map's id;
         * @param pos The cell;
         * @param nLayerId The world layer id;
         * @param tile Receives the tile;
         * @return false with no such map or layer, no tile, or once removed;
         */
        bool World :: GetTile( int nMapId,
                               const SunLight :: TileMap :: stMatrixPosition& pos,
                               int nLayerId,
                               SunLight :: TileMap :: stTile& tile )  {

            int  nIndex = IndexOfMap( nMapId );

            if( !m_pRenderer || ( nIndex < 0 ) )
                return false;

            return m_pRenderer -> WorldTile( *this, ( size_t ) nIndex, pos, nLayerId, tile );
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
