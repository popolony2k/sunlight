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

#ifndef __MAPDATA_H__
#define __MAPDATA_H__

#include "tmx.h"
#include "base/primitives.h"
#include "tilemap/tilemapdefs.h"
#include "renderer/projection/imapprojection.h"
#include <cstdint>
#include <deque>
#include <memory>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief One loaded Tiled map (S1c of the master plan): everything that belongs to it and
         * goes with it - the map libtmx parsed, the Resource Manager its external tilesets and
         * templates live in, the projection for its orientation, its size in map pixels, whether
         * every tileset matches its grid, and the running state of its animated tiles. It knows
         * nothing about TileMapRenderer, views or the screen.
         *
         * Load() is the only way to get one, so a MapData always holds a valid map. Destroying it
         * frees everything: the map first (libtmx releases its images through the texture hook),
         * then the Resource Manager the map points into, then the animation state - so replacing
         * a renderer's MapData with a newly loaded one frees the old map exactly once, and a load
         * that fails leaves the old one untouched.
         */
        class MapData  {

            // The running state of one animated tile, kept in the tile's own user_data.
            struct __stTileAnimInfo  {
                int64_t      nMillis;
                uint16_t     nCounter;
                tmx_tile     *pNextTile;
            };

            tmx_map                                                 *m_pTmxMap;
            tmx_resource_manager                                    *m_pTmxRcMgr;
            std :: unique_ptr<Projection :: IMapProjection>         m_pProjection;
            uint16_t                                                m_nPixelWidth;
            uint16_t                                                m_nPixelHeight;
            bool                                                    m_bUniformTileGrid;
            std :: deque<std :: unique_ptr<__stTileAnimInfo>>       m_AnimInfoList;

            MapData( tmx_map *pTmxMap, tmx_resource_manager *pTmxRcMgr, std :: unique_ptr<Projection :: IMapProjection> pProjection );

            tmx_tileset_list* TilesetListOf( tmx_tileset *pTileset ) const;

            public:

            static std :: unique_ptr<MapData> Load( const char *szTmxMapFile );

            ~MapData( void );
            MapData( const MapData& ) = delete;
            MapData& operator=( const MapData& ) = delete;

            tmx_map* Tmx( void ) const;
            Projection :: IMapProjection& MapProjection( void ) const;
            uint16_t PixelWidth( void ) const;
            uint16_t PixelHeight( void ) const;
            bool IsUniformTileGrid( void ) const;
            SunLight :: Base :: stSize2D DefaultScrollStep( void ) const;

            tmx_layer* FindLayer( int nLayerId ) const;
            tmx_layer* FindLayer( const char *szLayerName ) const;
            bool TileAt( const SunLight :: TileMap :: stMatrixPosition &pos, tmx_layer *pLayer, SunLight :: TileMap :: stTile &tile ) const;
            bool FindObjectByName( const char *szObjectName, SunLight :: TileMap :: stObject &object ) const;
            tmx_tile* AnimatedFrameOf( tmx_tile *pBase, unsigned int nGID );
        };
    }
}

#endif  /* __MAPDATA_H__ */
