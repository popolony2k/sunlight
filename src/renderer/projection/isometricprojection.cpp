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
                const float  __DIAMOND_TOP_SHIFT   = 1.0f;   // a diamond's top vertex is half a tile (one u step)
                                                              // right of its bounding box's top-left corner (I17)

                // (col, row) -> the cell's own top-left corner, in map pixels. The lattice step always uses the
                // MAP's own declared tile size (pMap->tile_width/height), never a resolved tile's own image size -
                // confirmed by a probe map whose tile images (32x32) were smaller than its declared grid step
                // (64x32): every cell still landed on this lattice, regardless of its own tile image's size.
                SunLight :: Base :: stCoordinate2D CellTopLeft( int nCol, int nRow, tmx_map* pMap )  {

                    float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                    float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                    int    nShift = ( ( int ) pMap -> height ) - __LATTICE_HEIGHT_SHIFT;

                    return SunLight :: Base :: stCoordinate2D { ( int ) ( ( nCol - nRow + nShift ) * fHalfW ),
                                                                 ( int ) ( ( nCol + nRow ) * fHalfH ) };
                }
            }

            /**
             * @brief The diamond's bounding box: (width + height) half tiles along each axis, every cell's diamond inside it.
             * @param pMap The map, as libtmx parsed it;
             * @return The map's pixel size;
             */
            SunLight :: Base :: stSize2D IsometricProjection :: MapPixelSize( tmx_map* pMap ) const  {

                int  nCells = ( int ) pMap -> width + ( int ) pMap -> height;

                return SunLight :: Base :: stSize2D { ( int ) ( nCells * ( ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR ) ),
                                                       ( int ) ( nCells * ( ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR ) ) };
            }

            /**
             * @brief The cell's bounding-box corner on the diamond lattice (CellTopLeft), plus the layer's offset.
             * @param pos The cell's row and column;
             * @param pMap The map, as libtmx parsed it;
             * @param nTileWidth Unused: the lattice steps by the map's own tile size;
             * @param nTileHeight Unused, likewise;
             * @param nLayerOffsetX The layer's own x offset, added to the cell's corner;
             * @param nLayerOffsetY The layer's own y offset, likewise;
             * @return The cell's top-left corner, in map pixels;
             */
            SunLight :: Base :: stCoordinate2D IsometricProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition& pos,
                                                                                          tmx_map* pMap,
                                                                                          int /* nTileWidth */,
                                                                                          int /* nTileHeight */,
                                                                                          int nLayerOffsetX,
                                                                                          int nLayerOffsetY ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );

                return SunLight :: Base :: stCoordinate2D { cell.x + nLayerOffsetX, cell.y + nLayerOffsetY };
            }

            /**
             * @brief A true diamond hit-test (I17): the point in half tiles from the diamond's top vertex, then the two floors of the lattice's inverse. False outside the map's pixel rectangle and in its empty corners, outside every diamond.
             * @param coord The coordinate, in map pixels;
             * @param pMap The map, as libtmx parsed it;
             * @param nMapPixelWidth MapPixelSize's width, already computed;
             * @param nMapPixelHeight MapPixelSize's height, likewise;
             * @param pos Receives the cell's row and column, when the point is in a cell;
             * @return Whether the point is in one of the map's cells;
             */
            bool IsometricProjection :: MapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                                         tmx_map* pMap,
                                                         int nMapPixelWidth,
                                                          int nMapPixelHeight,
                                                          SunLight :: TileMap :: stMatrixPosition& pos ) const  {

                int  nCoordX = coord.x;
                int  nCoordY = coord.y;

                if( !( ( nCoordX >= __NO_GRID_CELLS ) && ( nCoordX < nMapPixelWidth ) &&
                      ( nCoordY >= __NO_GRID_CELLS ) && ( nCoordY < nMapPixelHeight ) ) )
                    return false;

                // A true diamond hit-test (I17 of the master plan), as Tiled's own screenToTileCoords: the
                // point measured in half tiles from the diamond's TOP VERTEX - half a tile right of the cell's
                // bounding-box corner (CellTopLeft), hence the extra __DIAMOND_TOP_SHIFT - then the two floors
                // of the lattice's inverse. Every point of the map falls in exactly one diamond (the floors
                // split the shared edges). It used to measure from the box corner, so a cell's own centre
                // floored into the next column - the point picked a neighbour, never the cell under it
                // (found by Scarab: 0 of 48 cells of the isometric sample picked themselves).
                float  fHalfW  = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH  = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                int    nShift  = ( ( int ) pMap -> height ) - __LATTICE_HEIGHT_SHIFT;
                float  fMapX   = ( float ) coord.x;
                float  fMapY   = ( float ) coord.y;
                float  fU      = ( fMapX / fHalfW ) - nShift - __DIAMOND_TOP_SHIFT;
                float  fV      = fMapY / fHalfH;
                int    nCol    = ( int ) std :: floor( ( fU + fV ) / __HALF_TILE_DIVISOR );
                int    nRow    = ( int ) std :: floor( ( fV - fU ) / __HALF_TILE_DIVISOR );

                // Inside the map's pixel rectangle but outside every diamond (its empty corners): no cell.
                if( ( nCol < __NO_GRID_CELLS ) || ( nCol >= ( int ) pMap -> width ) ||
                    ( nRow < __NO_GRID_CELLS ) || ( nRow >= ( int ) pMap -> height ) )
                    return false;

                pos.nTileCol = nCol;
                pos.nTileRow = nRow;

                return true;
            }

            /**
             * @brief One whole tile of the map (its tile_width by tile_height), the same convention as orthogonal.
             * @param pMap The map, as libtmx parsed it;
             * @return The default step, in map pixels;
             */
            SunLight :: Base :: stSize2D IsometricProjection :: DefaultScrollStep( tmx_map* pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            /**
             * @brief The (col, row) range the rectangle's four corners span on the lattice's inverse, floored, widened by one at the high ends and clamped to the grid.
             * @param pMap The map, as libtmx parsed it;
             * @param mapPixelRect The rectangle to test, in map pixel space;
             * @return The matrix range that could touch it, clamped to the map's grid;
             */
            stTileRange IsometricProjection :: VisibleTileRange( tmx_map* pMap,
                                                                  const SunLight :: Base :: stRectangle& mapPixelRect ) const  {

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
