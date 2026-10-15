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

#include "renderer/map/mapdata.h"
#include "renderer/map/externalresources.h"
#include "renderer/map/maporientation.h"
#include "renderer/projection/orthogonalprojection.h"
#include "renderer/projection/isometricprojection.h"
#include "renderer/projection/staggeredprojection.h"
#include "renderer/projection/hexagonalprojection.h"
#include "filesystem/filesystemfactory.h"
#include "engines/enginefactory.h"
#include "general/clock.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>


namespace SunLight  {
    namespace Renderer  {

        namespace  {

            /*
             * External tilesets (.tsx) and object templates (.tx).
             *
             * libtmx opens these ITSELF (xmlReaderForFile - a raw OS open,
             * relative to the working directory), so they bypass
             * SunLight::FileSystem: they could not come from a mounted archive
             * (a .zip pack with no loose copy) and could never pass through the
             * read filter (an encrypted pack). Load therefore reads them
             * through the FileSystem UP FRONT and hands them to libtmx as buffers
             * via its Resource Manager, which libtmx consults - keyed by the raw
             * `source`/`template` attribute text - before it would open anything.
             * A reference the FileSystem cannot provide is simply left for
             * libtmx's own lookup, so a project that keeps them loose beside the
             * working directory keeps working exactly as before.
             */

            /**
             * libtmx's texture loader hook: loads a map's (or a tileset's) image through IEngine.
             * @param szFileName Texture file name;
             */
            void* TextureLoaderCallback( const char* szFileName )  {

                int  nWidth, nHeight;

                // While an external tileset/template is being preloaded, this is the image path as written
                // in that file - relative to the file's own directory. (Maps' own images arrive already
                // resolved by libtmx against the map's path.)
                std :: string  strImageBase = ExternalResources :: ImageBase();

                if( !strImageBase.empty() )  {
                    std :: string  strResolved = ExternalResources :: JoinVirtualPath( strImageBase, szFileName );

                    if( !strResolved.empty() )
                        return SunLight :: Engines :: EngineFactory :: GetEngine().LoadTexture( strResolved.c_str(), nWidth, nHeight );
                }

                return SunLight :: Engines :: EngineFactory :: GetEngine().LoadTexture( szFileName, nWidth, nHeight );
            }

            /**
             * libtmx's texture release hook: releases an image TextureLoaderCallback loaded.
             * @param pTexture The texture to release;
             */
            void TextureFreeCallback( void* pTexture )  {

                SunLight :: Engines :: EngineFactory :: GetEngine().UnloadTexture( pTexture );
            }

            /**
             * @brief Walks pLayer's linked list looking for a named object inside any L_OBJGR
             * layer, recursing into L_GROUP the same way the renderer's layer walk does, so a map
             * that nests an object group inside a `<group>` layer is still searched.
             * @param pLayer Head of the tmx_layer linked list to search;
             * @param szObjectName The object's name, as set in the map editor;
             * @param object Reference to receive the object's data if found;
             * @return true if an object with this name was found;
             */
            bool FindObjectInLayers( tmx_layer* pLayer, const char* szObjectName, SunLight :: TileMap :: stObject& object )  {

                for( ; pLayer != nullptr; pLayer = pLayer -> next )  {

                    if( pLayer -> type == L_GROUP )  {
                        if( FindObjectInLayers( pLayer -> content.group_head, szObjectName, object ) )
                            return true;

                        continue;
                    }

                    if( pLayer -> type != L_OBJGR )
                        continue;

                    for( tmx_object* pObject = pLayer -> content.objgr -> head; pObject != nullptr; pObject = pObject -> next )  {

                        if( ( pObject -> name != nullptr ) && ( ::strcmp( pObject -> name, szObjectName ) == 0 ) )  {

                            object.strName            = pObject -> name;
                            object.dimension.pos.x    = ( int ) pObject -> x;
                            object.dimension.pos.y    = ( int ) pObject -> y;
                            object.dimension.size.nWidth  = ( int ) pObject -> width;
                            object.dimension.size.nHeight = ( int ) pObject -> height;

                            return true;
                        }
                    }
                }

                return false;
            }
        }

        /**
         * @brief Load a map through SunLight::FileSystem.
         *
         * Reads the .tmx XML itself rather than through libtmx's own path-based ::tmx_load(),
         * which fopen()s directly - one whole-file ReadFile() up front (the same "load it all up
         * front" pattern used for textures and sound), then tmx_rcmgr_load_buffer_vpath() (NOT
         * the plain tmx_load_buffer()) to parse it.
         *
         * A real, segfault-causing bug found live going straight to tmx_load_buffer() first: its
         * public signature takes no vpath at all, unlike tmx_load()'s path argument - internally,
         * every relative reference inside the map (a `<tileset source="...">` or an
         * `<imagelayer>`'s `<image source="...">`) gets resolved against this vpath via
         * mk_absolute_path() (tmx_utils.c) - passing none means every such reference resolves to
         * its bare relative path, with no base directory at all, which then reached the texture
         * hook's IEngine::LoadTexture call already missing the context needed to find it.
         * tmx_rcmgr_load_buffer_vpath() is libtmx's own sanctioned way to load from memory while
         * still supplying that vpath explicitly - confirmed safe to call with rc_mgr=NULL (no
         * tileset/template caching) by reading its own source: tmx_load() itself passes
         * rc_mgr=NULL internally the exact same way.
         *
         * @param szTmxMapFile The map file, as a virtual path;
         * @return The loaded map; nullptr if the file cannot be read or parsed, or its
         * orientation has no projection - the reason is written to standard error;
         */
        std :: unique_ptr<MapData> MapData :: Load( const char* szTmxMapFile )  {

            std :: vector<unsigned char>  data;

            if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( szTmxMapFile, data ) )  {
                fprintf( stderr, "Cannot load map: [%s] could not be read\n", szTmxMapFile );
                return nullptr;
            }

            // libtmx loads every image a map names through these process-wide hooks.
            tmx_img_load_func = TextureLoaderCallback;
            tmx_img_free_func = TextureFreeCallback;

            /*
             * External tilesets/templates the map names (see the "External tilesets" note
             * above): read through the FileSystem and handed over in a Resource Manager. A map
             * that names none keeps the plain, manager-less path.
             */
            std :: vector<ExternalResources :: ExternalReference>  externalReferences;

            ExternalResources :: ScanExternalReferences( std :: string( data.begin(), data.end() ), externalReferences );

            tmx_resource_manager  *pRcMgr = nullptr;

            if( !externalReferences.empty() )  {
                std :: map<std :: string, std :: string>  loadedKeys;
                std :: set<std :: string>                 visited;
                std :: string                             strMapDir = ExternalResources :: DirectoryOfVirtualPath( szTmxMapFile );

                // libtmx's allocator hooks default to NULL and are only set up by its own tmx_load*/
                // tmx_rcmgr_load* entry points - NOT by tmx_make_resource_manager(), which allocates
                // straight away: the first map loaded in a process would crash here. Set the same
                // defaults libtmx would (its set_alloc_functions()).
                if( !tmx_alloc_func )
                    tmx_alloc_func = realloc;

                if( !tmx_free_func )
                    tmx_free_func = free;

                pRcMgr = ::tmx_make_resource_manager();

                for( const ExternalResources :: ExternalReference& reference : externalReferences )
                    ExternalResources :: PreloadExternalResource( pRcMgr, reference, strMapDir, loadedKeys, visited );
            }

            tmx_map  *pTmxMap = ::tmx_rcmgr_load_buffer_vpath( pRcMgr, reinterpret_cast<const char *>( data.data() ), ( int ) data.size(), szTmxMapFile );

            if( !pTmxMap )  {
                ::tmx_perror( "Cannot load map" );

                if( pRcMgr )
                    ::tmx_free_resource_manager( pRcMgr );

                return nullptr;
            }

            /*
             * The tile grid math is behind IMapProjection (E0 of the master plan), one
             * implementation per orientation libtmx can report. Any other orientation is refused,
             * freed the same way the destructor frees a map: the map first, then the manager it
             * points into.
             */
            std :: unique_ptr<Projection :: IMapProjection>  pProjection;

            switch( pTmxMap -> orient )  {

                case O_ORT:
                    pProjection = std :: make_unique<Projection :: OrthogonalProjection>();
                    break;

                case O_ISO:
                    pProjection = std :: make_unique<Projection :: IsometricProjection>();
                    break;

                case O_STA:
                    pProjection = std :: make_unique<Projection :: StaggeredProjection>();
                    break;

                case O_HEX:
                    pProjection = std :: make_unique<Projection :: HexagonalProjection>();
                    break;

                default:
                    // Every orientation string libtmx recognizes (orthogonal/isometric/staggered/hexagonal)
                    // has a projection, and O_NONE is unreachable here - checked against libtmx's own
                    // source (tmx_xml.c): a missing "orientation" attribute and an unrecognized one are
                    // BOTH rejected at parse time (two separate tmx_err calls), so pTmxMap is already
                    // nullptr and Load already returned above, for any map that would otherwise reach
                    // this switch with orient == O_NONE. The same finding B3 made for L_NONE/OT_NONE -
                    // kept only so the switch covers every enumerator.
                    fprintf( stderr, "Cannot load map: [%s] uses %s orientation; only orthogonal, isometric, staggered and hexagonal maps are supported.\n",
                             szTmxMapFile, MapOrientation :: OrientationName( pTmxMap -> orient ) );

                    ::tmx_map_free( pTmxMap );

                    if( pRcMgr )
                        ::tmx_free_resource_manager( pRcMgr );

                    return nullptr;
            }

            return std :: unique_ptr<MapData>( new MapData( pTmxMap, pRcMgr, std :: move( pProjection ) ) );
        }

        /**
         * @brief Takes ownership of a parsed map, its Resource Manager (nullptr when it names no
         * external tileset or template) and its projection, and works out what the renderer
         * reads of them every frame: the map's size in map pixels and whether every tileset
         * matches its grid.
         */
        MapData :: MapData( tmx_map* pTmxMap, tmx_resource_manager* pTmxRcMgr, std :: unique_ptr<Projection :: IMapProjection> pProjection )
                : m_pTmxMap( pTmxMap ),
                  m_pTmxRcMgr( pTmxRcMgr ),
                  m_pProjection( std :: move( pProjection ) )  {

            SunLight :: Base :: stSize2D  mapPixelSize = m_pProjection -> MapPixelSize( m_pTmxMap );

            m_nPixelWidth  = ( uint16_t ) mapPixelSize.nWidth;
            m_nPixelHeight = ( uint16_t ) mapPixelSize.nHeight;

            // A tile layer draws each cell at its own resolved tile's tileset size (pTs->tile_width/height),
            // not the map's declared one; VisibleTileRange's pixel-to-cell math assumes they are the same.
            // That is true for every tileset here, or culling would silently skip tiles far from the origin
            // (a scale mismatch, not an edge case a padding amount could fix) - checked once, so a tile layer
            // only culls when it is exact, and falls back to visiting every cell otherwise.
            m_bUniformTileGrid = true;

            for( tmx_tileset_list* pNode = m_pTmxMap -> ts_head; pNode != nullptr; pNode = pNode -> next )  {
                if( ( pNode -> tileset -> tile_width != m_pTmxMap -> tile_width ) ||
                    ( pNode -> tileset -> tile_height != m_pTmxMap -> tile_height ) )  {
                    m_bUniformTileGrid = false;
                    break;
                }
            }
        }

        /**
         * @brief Free the map - libtmx releases its images through the texture hook - then the
         * Resource Manager it points into (the map held pointers into its external tilesets and
         * templates, so only now). The animation state goes with the members, after the map.
         */
        MapData :: ~MapData( void )  {

            ::tmx_map_free( m_pTmxMap );

            if( m_pTmxRcMgr )
                ::tmx_free_resource_manager( m_pTmxRcMgr );
        }

        tmx_map* MapData :: Tmx( void ) const  {

            return m_pTmxMap;
        }

        Projection :: IMapProjection& MapData :: MapProjection( void ) const  {

            return *m_pProjection;
        }

        uint16_t MapData :: PixelWidth( void ) const  {

            return m_nPixelWidth;
        }

        uint16_t MapData :: PixelHeight( void ) const  {

            return m_nPixelHeight;
        }

        bool MapData :: IsUniformTileGrid( void ) const  {

            return m_bUniformTileGrid;
        }

        /**
         * @brief The scroll step a "-1" (the map's tile size) resolves to, by the projection.
         */
        SunLight :: Base :: stSize2D MapData :: DefaultScrollStep( void ) const  {

            return m_pProjection -> DefaultScrollStep( m_pTmxMap );
        }

        /**
         * @brief A layer by its id, nested in a group or not; nullptr when there is none.
         */
        tmx_layer* MapData :: FindLayer( int nLayerId ) const  {

            return ::tmx_find_layer_by_id( m_pTmxMap, nLayerId );
        }

        /**
         * @brief A layer by its name, nested in a group or not; nullptr when there is none.
         */
        tmx_layer* MapData :: FindLayer( const char* szLayerName ) const  {

            return ::tmx_find_layer_by_name( m_pTmxMap, szLayerName );
        }

        /**
         * @brief The tile at a row and column of a tile layer: its gid (the flip bits removed)
         * and its tile. A position off the map has no gid - reading gids[] there would run past
         * the array (the collision lookup reaches this with a sprite's position, A6) - and a gid
         * past libtmx's tiles[] table (only tilecount entries long) has no tile; both answer an
         * empty tile, the same as an unset gid. tile.dimension is not touched: it depends on the
         * view, which the renderer knows.
         * @param pos The row and column;
         * @param pLayer The tile layer;
         * @param tile Receives the gid and the tile;
         * @return true if there is a tile there;
         */
        bool MapData :: TileAt( const SunLight :: TileMap :: stMatrixPosition& pos, tmx_layer* pLayer, SunLight :: TileMap :: stTile& tile ) const  {

            if( ( pos.nTileRow < 0 ) || ( pos.nTileRow >= ( int ) m_pTmxMap -> height ) ||
                ( pos.nTileCol < 0 ) || ( pos.nTileCol >= ( int ) m_pTmxMap -> width ) )  {
                tile.nGID  = 0;
                tile.pTile = NULL;

                return false;
            }

            tile.nGID = pLayer -> content.gids[( pos.nTileRow *
                                                 m_pTmxMap -> width ) +
                                                 pos.nTileCol] &
                                                TMX_FLIP_BITS_REMOVAL;
            tile.pTile = ( ( m_pTmxMap -> tiles && ( tile.nGID < m_pTmxMap -> tilecount ) ) ?
                           m_pTmxMap -> tiles[tile.nGID] : NULL );

            if( !tile.pTile )  {
                tile.nGID  = 0;
                tile.pTile = NULL;

                return false;
            }

            return true;
        }

        /**
         * @brief Look up a level-design-authored object (a Tiled `<object>` placed inside any
         * `<objectgroup>` layer, groups included) by name - libtmx has no tmx_find_object_by_name
         * of its own (only tmx_find_object_by_id, no use here: objects are authored and named by
         * hand in the map editor, not looked up by their internal id).
         * @param szObjectName The object's name, as set in the map editor;
         * @param object Receives the object's name, position and size;
         * @return true if an object with this name was found;
         */
        bool MapData :: FindObjectByName( const char* szObjectName, SunLight :: TileMap :: stObject& object ) const  {

            return FindObjectInLayers( m_pTmxMap -> ly_head, szObjectName, object );
        }

        /**
         * @brief Which tmx_tileset_list contains a tileset (its firstgid is there).
         */
        tmx_tileset_list* MapData :: TilesetListOf( tmx_tileset* pTileset ) const  {

            for( tmx_tileset_list* pItem = m_pTmxMap -> ts_head; pItem != NULL; pItem = pItem -> next )  {
                if( pItem -> tileset == pTileset )  {
                    return pItem;
                }
            }

            return nullptr;
        }

        /**
         * @brief The frame of an animated tile that is on screen now, advancing its animation by
         * the clock (shared by tile layers and tile objects, so an animated tile moves the same
         * way wherever it is drawn). A frame whose gid has no tile in the tile table has nothing
         * to show: the base tile stays on screen for it.
         * @param pBase The tile as its gid names it (its animation_len is not zero);
         * @param nGID The gid that named pBase;
         * @return The tile to draw now; pBase when there is no other to show;
         */
        tmx_tile* MapData :: AnimatedFrameOf( tmx_tile* pBase, unsigned int nGID )  {

            tmx_tile  *pTile = pBase;

            int64_t                    nMillis = SunLight :: General :: Clock :: NowMilliseconds();
            __stTileAnimInfo           *pAnimInfo = ( __stTileAnimInfo * ) pTile -> user_data.pointer;
            tmx_tileset_list           *pTilesetList;

            if( !pAnimInfo )  {
                std :: unique_ptr<__stTileAnimInfo>  pOwnedAnimInfo = std :: make_unique<__stTileAnimInfo>();

                memset( pOwnedAnimInfo.get(), 0, sizeof( __stTileAnimInfo ) );
                pOwnedAnimInfo -> nMillis   = nMillis;
                pOwnedAnimInfo -> pNextTile = pTile;
                pAnimInfo = pOwnedAnimInfo.get();
                pTile -> user_data.pointer = pAnimInfo;
                m_AnimInfoList.push_back( std :: move( pOwnedAnimInfo ) );
            }

            if( pAnimInfo -> nMillis <= nMillis )  {
                unsigned int     nNextFrmGID;
                _tmx_frame       *pTmxAnimFrm;

                // The frame due now, then the counter steps on to the next one, wrapping after the last:
                // each frame is shown once per cycle, for its own duration (0 1 2 0 1 2 ...). It used
                // to wrap only when the frame after the last was due - setting the counter back to 0 AND
                // showing frame 0 without stepping it on - so frame 0 came again on the next step and was
                // held for two of its durations every cycle (0 1 2 0 0 1 2 0 0 ...).
                pTmxAnimFrm = &pTile -> animation[pAnimInfo -> nCounter];
                pAnimInfo -> nCounter = ( uint16_t ) ( ( pAnimInfo -> nCounter + 1 ) % pTile -> animation_len );

                pTilesetList = TilesetListOf( pTile -> tileset );
                pTile = nullptr;

                if( pTilesetList )  {
                    nNextFrmGID = ( pTilesetList -> firstgid + pTmxAnimFrm -> tile_id );
                    // Same tilecount bound as TileAt: a frame gid past
                    // the tiles[] table has no tile to show.
                    pAnimInfo -> pNextTile = ( nNextFrmGID < m_pTmxMap -> tilecount ?
                                               m_pTmxMap -> tiles[nNextFrmGID] : NULL );
                    pAnimInfo -> nMillis   = ( pTmxAnimFrm -> duration + nMillis );
                    pTile = pAnimInfo -> pNextTile;
                }

                if( !pTile )
                    pTile = m_pTmxMap -> tiles[nGID];
            }
            else  {
                pTile = ( ( __stTileAnimInfo * ) pTile -> user_data.pointer ) -> pNextTile;

                // A frame with no tile (see above) keeps the base tile on screen.
                if( !pTile )
                    pTile = pBase;
            }

            return pTile;
        }
    }
}
