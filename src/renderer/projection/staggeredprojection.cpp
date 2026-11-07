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

#include "renderer/projection/staggeredprojection.h"
#include <algorithm>
#include <cmath>

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            namespace  {

                const int    __NO_GRID_CELLS       = 0;    // the near edge of every map's grid
                const float  __HALF_TILE_DIVISOR   = 2.0f; // the staggered axis steps by half a tile
                const int    __NEXT_LINE           = 1;    // the line (row or column) next to another
                const int    __LINES_PER_PAIR      = 2;    // an unshifted line and a shifted one make one whole tile
                const float  __DIAMOND_RADIUS      = 1.0f; // |dx| / halfW + |dy| / halfH within a diamond (I17)
                const int    __SHIFT_LINE_MARGIN   = 1;    // VisibleTileRange's extra line of slack on the
                                                            // staggered axis' lower bound, to cover both
                                                            // possible shifts (0 and half a tile) - see below

                // Whether the row (SA_Y) or column (SA_X) at nLineIndex is one of the shifted ones, per
                // libtmx's stagger_index. A missing/unparsed index defaults to SI_ODD - the same default
                // libtmx's own parser uses for a missing "staggerindex" attribute (tmx_utils.c:
                // parse_stagger_index(NULL) == SI_ODD) - so a hand-built tmx_map (SI_NONE, zero-initialized)
                // behaves the same as a real Tiled file that never set the attribute.
                bool IsShiftedLine( int nLineIndex, tmx_stagger_index index )  {

                    bool  bEven = ( ( nLineIndex % 2 ) == 0 );

                    return ( index == SI_EVEN ) ? bEven : !bEven;
                }

                // libtmx's own default for a missing "staggeraxis" attribute (tmx_utils.c:
                // parse_stagger_axis(NULL) == SA_Y), applied the same way to a hand-built tmx_map.
                tmx_stagger_axis EffectiveAxis( tmx_map* pMap )  {

                    return ( pMap -> stagger_axis == SA_X ) ? SA_X : SA_Y;
                }

                // (col, row) -> the cell's own top-left corner, in map pixels (E2a of the master plan):
                //   SA_Y: screenY = row * (tileHeight / 2); screenX = col * tileWidth, plus tileWidth / 2
                //         when the row's own parity matches stagger_index.
                //   SA_X: screenX = col * (tileWidth / 2); screenY = row * tileHeight, plus tileHeight / 2
                //         when the column's own parity matches stagger_index.
                // Always the MAP's own declared tile_width/tile_height, never a resolved tile's own image
                // size - the same convention E1a confirmed for isometric, not separately re-measured here
                // (no probe used a mismatched tileset), so this follows isometric's precedent rather than
                // its own independent confirmation.
                SunLight :: Base :: stCoordinate2D CellTopLeft( int nCol, int nRow, tmx_map* pMap )  {

                    float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                    float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                    if( EffectiveAxis( pMap ) == SA_Y )  {
                        float  fShiftX = IsShiftedLine( nRow, pMap -> stagger_index ) ? fHalfW : 0.0f;

                        return SunLight :: Base :: stCoordinate2D { ( int ) ( nCol * ( float ) pMap -> tile_width + fShiftX ),
                                                                     ( int ) ( nRow * fHalfH ) };
                    }

                    float  fShiftY = IsShiftedLine( nCol, pMap -> stagger_index ) ? fHalfH : 0.0f;

                    return SunLight :: Base :: stCoordinate2D { ( int ) ( nCol * fHalfW ),
                                                                 ( int ) ( nRow * ( float ) pMap -> tile_height + fShiftY ) };
                }
            }

            /**
             * @brief The staggered grid's box: on the staggered axis (height + 1) half tiles, on the other whole tiles plus the half a tile the shifted lines stick out.
             * @param pMap The map, as libtmx parsed it;
             * @return The map's pixel size;
             */
            SunLight :: Base :: stSize2D StaggeredProjection :: MapPixelSize( tmx_map* pMap ) const  {

                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                if( EffectiveAxis( pMap ) == SA_Y )  {
                    return SunLight :: Base :: stSize2D { ( int ) ( pMap -> width * pMap -> tile_width + fHalfW ),
                                                           ( int ) ( ( pMap -> height + 1 ) * fHalfH ) };
                }

                return SunLight :: Base :: stSize2D { ( int ) ( ( pMap -> width + 1 ) * fHalfW ),
                                                       ( int ) ( pMap -> height * pMap -> tile_height + fHalfH ) };
            }

            /**
             * @brief The cell's top-left corner (CellTopLeft): whole tiles on the unstaggered axis, half tiles on the staggered one, with each shifted line (stagger_index) moved half a tile along the other axis; plus the layer's offset.
             * @param pos The cell's row and column;
             * @param pMap The map, as libtmx parsed it;
             * @param nTileWidth Unused: the lattice steps by the map's own tile size;
             * @param nTileHeight Unused, likewise;
             * @param nLayerOffsetX The layer's own x offset, added to the cell's corner;
             * @param nLayerOffsetY The layer's own y offset, likewise;
             * @return The cell's top-left corner, in map pixels;
             */
            SunLight :: Base :: stCoordinate2D StaggeredProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition& pos,
                                                                                         tmx_map* pMap,
                                                                                         int /* nTileWidth */,
                                                                                         int /* nTileHeight */,
                                                                                         int nLayerOffsetX,
                                                                                         int nLayerOffsetY ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );

                return SunLight :: Base :: stCoordinate2D { cell.x + nLayerOffsetX, cell.y + nLayerOffsetY };
            }

            /**
             * @brief A true diamond hit-test (I17): the whole-tile rectangle anchored at an unshifted line, its inscribed diamond for that line's cell, and the rectangle's corners for the shifted lines either side. False outside the map's pixel rectangle and on its ragged edges, outside every diamond.
             * @param coord The coordinate, in map pixels;
             * @param pMap The map, as libtmx parsed it;
             * @param nMapPixelWidth MapPixelSize's width, already computed;
             * @param nMapPixelHeight MapPixelSize's height, likewise;
             * @param pos Receives the cell's row and column, when the point is in a cell;
             * @return Whether the point is in one of the map's cells;
             */
            bool StaggeredProjection :: MapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                                         tmx_map* pMap,
                                                         int nMapPixelWidth,
                                                          int nMapPixelHeight,
                                                          SunLight :: TileMap :: stMatrixPosition& pos ) const  {

                int  nCoordX = coord.x;
                int  nCoordY = coord.y;

                if( !( ( nCoordX >= __NO_GRID_CELLS ) && ( nCoordX < nMapPixelWidth ) &&
                      ( nCoordY >= __NO_GRID_CELLS ) && ( nCoordY < nMapPixelHeight ) ) )
                    return false;

                float  fMapX  = ( float ) coord.x;
                float  fMapY  = ( float ) coord.y;
                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                // A true diamond hit-test (I17 of the master plan). Each cell's diamond is inscribed in its
                // tileWidth x tileHeight box (CellTopLeft), and on the staggered axis the lines are only half
                // a tile apart, so the boxes overlap and a box cannot tell which diamond a point is in. The
                // lines that are NOT shifted tile the plane with whole-tile rectangles, each holding one of
                // their diamonds; the four corners of a rectangle outside that diamond belong to the shifted
                // lines either side (rows above/below for SA_Y, columns left/right for SA_X), whose cells
                // start half a tile further along. So: the rectangle, then the diamond, then the corner.
                // The diamond test is inclusive and the corners take the rest, so every point falls in
                // exactly one cell. It used to floor the point against the staggered axis's half-tile step,
                // which put a cell's own centre in the next line (found by Scarab: 0 of 48 cells of the
                // staggered sample picked themselves).
                bool   bAxisY    = ( EffectiveAxis( pMap ) == SA_Y );
                float  fTileW    = ( float ) pMap -> tile_width;
                float  fTileH    = ( float ) pMap -> tile_height;
                int    nFirst    = IsShiftedLine( __NO_GRID_CELLS, pMap -> stagger_index ) ? __NEXT_LINE : __NO_GRID_CELLS;
                int    nRow;
                int    nCol;

                if( bAxisY )  {
                    int    nPair  = ( int ) std :: floor( ( fMapY - nFirst * fHalfH ) / fTileH );
                    int    nLine  = nFirst + ( nPair * __LINES_PER_PAIR );
                    int    nRectX = ( int ) std :: floor( fMapX / fTileW );
                    float  fPx    = fMapX - ( nRectX * fTileW );
                    float  fPy    = fMapY - ( nLine * fHalfH );

                    nRow = nLine;
                    nCol = nRectX;

                    if( ( std :: fabs( fPx - fHalfW ) / fHalfW ) + ( std :: fabs( fPy - fHalfH ) / fHalfH ) > __DIAMOND_RADIUS )  {
                        nRow = nLine + ( ( fPy < fHalfH ) ? -__NEXT_LINE : __NEXT_LINE );
                        nCol = nRectX - ( ( fPx < fHalfW ) ? __NEXT_LINE : __NO_GRID_CELLS );   // a shifted cell starts half a tile right
                    }
                }
                else  {
                    int    nPair  = ( int ) std :: floor( ( fMapX - nFirst * fHalfW ) / fTileW );
                    int    nLine  = nFirst + ( nPair * __LINES_PER_PAIR );
                    int    nRectY = ( int ) std :: floor( fMapY / fTileH );
                    float  fPx    = fMapX - ( nLine * fHalfW );
                    float  fPy    = fMapY - ( nRectY * fTileH );

                    nCol = nLine;
                    nRow = nRectY;

                    if( ( std :: fabs( fPx - fHalfW ) / fHalfW ) + ( std :: fabs( fPy - fHalfH ) / fHalfH ) > __DIAMOND_RADIUS )  {
                        nCol = nLine + ( ( fPx < fHalfW ) ? -__NEXT_LINE : __NEXT_LINE );
                        nRow = nRectY - ( ( fPy < fHalfH ) ? __NEXT_LINE : __NO_GRID_CELLS );   // a shifted cell starts half a tile down
                    }
                }

                // Inside the map's pixel rectangle but outside every diamond (its ragged edges): no cell.
                if( ( nCol < __NO_GRID_CELLS ) || ( nCol >= ( int ) pMap -> width ) ||
                    ( nRow < __NO_GRID_CELLS ) || ( nRow >= ( int ) pMap -> height ) )
                    return false;

                pos = { nRow, nCol };

                return true;
            }

            /**
             * @brief One whole tile of the map (its tile_width by tile_height), the same convention as orthogonal.
             * @param pMap The map, as libtmx parsed it;
             * @return The default step, in map pixels;
             */
            SunLight :: Base :: stSize2D StaggeredProjection :: DefaultScrollStep( tmx_map* pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            /**
             * @brief Bounds loose enough for either shift of a staggered line (0 or half a tile), clamped to the grid.
             * @param pMap The map, as libtmx parsed it;
             * @param mapPixelRect The rectangle to test, in map pixel space;
             * @return The matrix range that could touch it, clamped to the map's grid;
             */
            stTileRange StaggeredProjection :: VisibleTileRange( tmx_map* pMap,
                                                                  const SunLight :: Base :: stRectangle& mapPixelRect ) const  {

                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                float  fLeft  = mapPixelRect.x;
                float  fRight = mapPixelRect.x + mapPixelRect.width;
                float  fTop   = mapPixelRect.y;
                float  fBottom = mapPixelRect.y + mapPixelRect.height;

                stTileRange  range;

                // Derived (not guessed) from each axis's own two possible shifts (0 or half a tile): the
                // loosest bound a shift of zero and a shift of half a tile can each produce, so neither
                // possible row/column parity in range is missed - see doc/MASTER_PLAN.md's E2b writeup for
                // the derivation. The axis that is NOT staggered (e.g. row, for SA_Y) has no shift at all,
                // so its bound reduces to the same exact floor/ceil orthogonal already uses.
                if( EffectiveAxis( pMap ) == SA_Y )  {
                    range.nRowStart = ( int ) std :: floor( fTop / fHalfH ) - __SHIFT_LINE_MARGIN;
                    range.nRowEnd   = ( int ) std :: ceil( fBottom / fHalfH );
                    range.nColStart = ( int ) std :: floor( ( fLeft - fHalfW ) / ( float ) pMap -> tile_width );
                    range.nColEnd   = ( int ) std :: ceil( fRight / ( float ) pMap -> tile_width );
                }
                else  {
                    range.nColStart = ( int ) std :: floor( fLeft / fHalfW ) - __SHIFT_LINE_MARGIN;
                    range.nColEnd   = ( int ) std :: ceil( fRight / fHalfW );
                    range.nRowStart = ( int ) std :: floor( ( fTop - fHalfH ) / ( float ) pMap -> tile_height );
                    range.nRowEnd   = ( int ) std :: ceil( fBottom / ( float ) pMap -> tile_height );
                }

                range.nColStart = std :: max( range.nColStart, __NO_GRID_CELLS );
                range.nRowStart = std :: max( range.nRowStart, __NO_GRID_CELLS );
                range.nColEnd   = std :: min( range.nColEnd, ( int ) pMap -> width );
                range.nRowEnd   = std :: min( range.nRowEnd, ( int ) pMap -> height );

                return range;
            }
        }
    }
}
