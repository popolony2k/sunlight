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

#include "renderer/projection/isometricprojection.h"
#include <algorithm>
#include <cmath>

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            namespace  {

                const int    __NO_GRID_CELLS       = 0;      // the near edge of every map's grid
                const float  __HALF_TILE_DIVISOR   = 2.0f;   // the diamond lattice steps by half a tile, both axes
                const int    __LATTICE_HEIGHT_SHIFT = 1;     // the diamond's left edge sits at x=0 when the map is
                                                              // offset by (heightCells - 1) tiles, not heightCells -
                                                              // confirmed by comparing two map heights (2 and 4
                                                              // cells); a formula without this "-1" only coincided
                                                              // with the measured export at height 2.

                // (col, row) -> the cell's own top-left corner, in map pixels. The lattice step always uses the
                // MAP's own declared tile size (pMap->tile_width/height), never a resolved tile's own image size -
                // confirmed by a probe map whose tile images (32x32) were smaller than its declared grid step
                // (64x32): every cell still landed on this lattice, regardless of its own tile image's size.
                SunLight :: Base :: stCoordinate2D CellTopLeft( int nCol, int nRow, tmx_map *pMap )  {

                    float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                    float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                    int    nShift = ( ( int ) pMap -> height ) - __LATTICE_HEIGHT_SHIFT;

                    return SunLight :: Base :: stCoordinate2D { ( int ) ( ( nCol - nRow + nShift ) * fHalfW ),
                                                                 ( int ) ( ( nCol + nRow ) * fHalfH ) };
                }
            }

            SunLight :: Base :: stSize2D IsometricProjection :: MapPixelSize( tmx_map *pMap ) const  {

                int  nCells = ( int ) pMap -> width + ( int ) pMap -> height;

                return SunLight :: Base :: stSize2D { ( int ) ( nCells * ( ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR ) ),
                                                       ( int ) ( nCells * ( ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR ) ) };
            }

            SunLight :: Base :: stCoordinate2D IsometricProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                                          tmx_map *pMap,
                                                                                          int /* nTileWidth */,
                                                                                          int /* nTileHeight */,
                                                                                          int nLayerOffsetX,
                                                                                          int nLayerOffsetY ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );

                return SunLight :: Base :: stCoordinate2D { cell.x + nLayerOffsetX, cell.y + nLayerOffsetY };
            }

            SunLight :: Base :: stDimension2D IsometricProjection :: TileViewRect( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                                     tmx_map *pMap,
                                                                                     const SunLight :: Base :: stDimension2D &viewport,
                                                                                     const SunLight :: Base :: stVector2D &camera ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );
                SunLight :: Base :: stDimension2D   tile;

                tile.pos.x        = ( cell.x + viewport.pos.x ) + ( int ) camera.x;
                tile.pos.y        = ( cell.y + viewport.pos.y ) + ( int ) camera.y;
                tile.size.nWidth  = ( int ) pMap -> tile_width;
                tile.size.nHeight = ( int ) pMap -> tile_height;

                return tile;
            }

            bool IsometricProjection :: ViewToTileMatrix( const SunLight :: Base :: stCoordinate2D &coord,
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

                // Invert CellTopLeft: solve the two linear equations for (col, row) given a map-pixel point,
                // then floor to the cell whose tileWidth x tileHeight bounding box contains it (the same
                // AABB-level precision TileMapToTileMatrix already uses for orthogonal - not a true diamond
                // hit-test, which collision/picking does not need today).
                float  fHalfW  = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH  = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                int    nShift  = ( ( int ) pMap -> height ) - __LATTICE_HEIGHT_SHIFT;
                float  fMapX   = ( ( coord.x + viewport.pos.x ) - camera.x );
                float  fMapY   = ( ( coord.y + viewport.pos.y ) - camera.y );
                float  fU      = ( fMapX / fHalfW ) - nShift;
                float  fV      = fMapY / fHalfH;

                pos.nTileCol = ( int ) std :: floor( ( fU + fV ) / __HALF_TILE_DIVISOR );
                pos.nTileRow = ( int ) std :: floor( ( fV - fU ) / __HALF_TILE_DIVISOR );

                return true;
            }

            SunLight :: Base :: stSize2D IsometricProjection :: DefaultScrollStep( tmx_map *pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            stTileRange IsometricProjection :: VisibleTileRange( tmx_map *pMap,
                                                                  const SunLight :: Base :: stRectangle &mapPixelRect ) const  {

                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                int    nShift = ( ( int ) pMap -> height ) - __LATTICE_HEIGHT_SHIFT;

                float  fU0 = ( mapPixelRect.x                         / fHalfW ) - nShift;
                float  fU1 = ( ( mapPixelRect.x + mapPixelRect.width )  / fHalfW ) - nShift;
                float  fV0 = mapPixelRect.y                          / fHalfH;
                float  fV1 = ( mapPixelRect.y + mapPixelRect.height ) / fHalfH;

                // col = (u + v) / 2, row = (v - u) / 2 - evaluate at all four corners of the (u, v) rectangle,
                // since col grows with both u and v while row grows with v but shrinks with u: the corner that
                // minimizes/maximizes each is not the same corner, so checking all four is the safe way to
                // bound both without working out by hand which one to use for which.
                float  fCols[4] = { ( fU0 + fV0 ) / __HALF_TILE_DIVISOR, ( fU1 + fV0 ) / __HALF_TILE_DIVISOR,
                                    ( fU0 + fV1 ) / __HALF_TILE_DIVISOR, ( fU1 + fV1 ) / __HALF_TILE_DIVISOR };
                float  fRows[4] = { ( fV0 - fU0 ) / __HALF_TILE_DIVISOR, ( fV0 - fU1 ) / __HALF_TILE_DIVISOR,
                                    ( fV1 - fU0 ) / __HALF_TILE_DIVISOR, ( fV1 - fU1 ) / __HALF_TILE_DIVISOR };

                float  fColMin = *std :: min_element( fCols, fCols + 4 );
                float  fColMax = *std :: max_element( fCols, fCols + 4 );
                float  fRowMin = *std :: min_element( fRows, fRows + 4 );
                float  fRowMax = *std :: max_element( fRows, fRows + 4 );

                stTileRange  range;

                range.nColStart = ( int ) std :: floor( fColMin );
                range.nColEnd   = ( int ) std :: floor( fColMax ) + 1;
                range.nRowStart = ( int ) std :: floor( fRowMin );
                range.nRowEnd   = ( int ) std :: floor( fRowMax ) + 1;

                range.nColStart = std :: max( range.nColStart, __NO_GRID_CELLS );
                range.nRowStart = std :: max( range.nRowStart, __NO_GRID_CELLS );
                range.nColEnd   = std :: min( range.nColEnd, ( int ) pMap -> width );
                range.nRowEnd   = std :: min( range.nRowEnd, ( int ) pMap -> height );

                return range;
            }
        }
    }
}
