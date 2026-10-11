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

#include "renderer/projection/orthogonalprojection.h"
#include <algorithm>
#include <cmath>

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            namespace  {

                const int  __NO_GRID_CELLS  = 0;   // the near edge of every map's grid
            }

            SunLight :: Base :: stSize2D OrthogonalProjection :: MapPixelSize( tmx_map *pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) ( pMap -> width * pMap -> tile_width ),
                                                       ( int ) ( pMap -> height * pMap -> tile_height ) };
            }

            SunLight :: Base :: stCoordinate2D OrthogonalProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                                          tmx_map * /* pMap */,
                                                                                          int nTileWidth,
                                                                                          int nTileHeight,
                                                                                          int nLayerOffsetX,
                                                                                          int nLayerOffsetY ) const  {

                return SunLight :: Base :: stCoordinate2D { ( pos.nTileCol * nTileWidth ) + nLayerOffsetX,
                                                             ( pos.nTileRow * nTileHeight ) + nLayerOffsetY };
            }

            SunLight :: Base :: stDimension2D OrthogonalProjection :: TileViewRect( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                                     tmx_map *pMap,
                                                                                     const SunLight :: Base :: stDimension2D &viewport,
                                                                                     const SunLight :: Base :: stVector2D &camera ) const  {

                SunLight :: Base :: stDimension2D  tile;

                tile.pos.x        = ( ( pos.nTileCol * ( int ) pMap -> tile_width ) + viewport.pos.x ) + ( int ) camera.x;
                tile.pos.y        = ( ( pos.nTileRow * ( int ) pMap -> tile_height ) + viewport.pos.y ) + ( int ) camera.y;
                tile.size.nWidth  = ( int ) pMap -> tile_width;
                tile.size.nHeight = ( int ) pMap -> tile_height;

                return tile;
            }

            bool OrthogonalProjection :: ViewToTileMatrix( const SunLight :: Base :: stCoordinate2D &coord,
                                                           tmx_map *pMap,
                                                           const SunLight :: Base :: stDimension2D &viewport,
                                                           const SunLight :: Base :: stVector2D &camera,
                                                           float fZoom,
                                                           int nMapPixelWidth,
                                                           int nMapPixelHeight,
                                                           SunLight :: TileMap :: stMatrixPosition &pos ) const  {

                int  nCoordX = ( int ) ( coord.x / fZoom );
                int  nCoordY = ( int ) ( coord.y / fZoom );

                if( !( ( nCoordX >= __NO_GRID_CELLS ) && ( nCoordX < nMapPixelWidth ) &&
                      ( nCoordY >= __NO_GRID_CELLS ) && ( nCoordY < nMapPixelHeight ) ) )
                    return false;

                pos.nTileCol = ( int ) ( ( ( coord.x + viewport.pos.x ) - camera.x ) / ( int ) pMap -> tile_width );
                pos.nTileRow = ( int ) ( ( ( coord.y + viewport.pos.y ) - camera.y ) / ( int ) pMap -> tile_height );

                return true;
            }

            SunLight :: Base :: stSize2D OrthogonalProjection :: DefaultScrollStep( tmx_map *pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            stTileRange OrthogonalProjection :: VisibleTileRange( tmx_map *pMap,
                                                                   const SunLight :: Base :: stRectangle &mapPixelRect ) const  {

                stTileRange  range;

                range.nColStart = ( int ) std :: floor( mapPixelRect.x / ( float ) pMap -> tile_width );
                range.nColEnd   = ( int ) std :: ceil( ( mapPixelRect.x + mapPixelRect.width ) / ( float ) pMap -> tile_width );
                range.nRowStart = ( int ) std :: floor( mapPixelRect.y / ( float ) pMap -> tile_height );
                range.nRowEnd   = ( int ) std :: ceil( ( mapPixelRect.y + mapPixelRect.height ) / ( float ) pMap -> tile_height );

                range.nColStart = std :: max( range.nColStart, __NO_GRID_CELLS );
                range.nRowStart = std :: max( range.nRowStart, __NO_GRID_CELLS );
                range.nColEnd   = std :: min( range.nColEnd, ( int ) pMap -> width );
                range.nRowEnd   = std :: min( range.nRowEnd, ( int ) pMap -> height );

                return range;
            }
        }
    }
}
