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

#include "renderer/projection/hexagonalprojection.h"
#include <algorithm>
#include <cmath>

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            namespace  {

                const int    __NO_GRID_CELLS     = 0;      // the near edge of every map's grid
                const float  __HALF_TILE_DIVISOR = 2.0f;   // the non-staggered axis still steps by half a tile
                const int    __NEXT_LINE         = 1;      // the line (row or column) next to another
                const int    __LINES_PER_PAIR    = 2;      // an unshifted line and a shifted one make one pair

                // Whether a point (px, py), relative to a cell's tileWidth x tileHeight box, is inside that
                // cell's hexagon (I17 of the master plan), inclusive. The hexagon's two sides of hexsidelength
                // run along the staggered axis (vertical for SA_Y, horizontal for SA_X) through the box's
                // middle; its two points sit on the box's edges across that axis. With hexsidelength 0 it is
                // the staggered map's diamond.
                bool InHexagon( float fPx, float fPy, float fHalfW, float fHalfH, float fHalfSide, bool bAxisY )  {

                    float  fDx = std :: fabs( fPx - fHalfW );
                    float  fDy = std :: fabs( fPy - fHalfH );

                    if( bAxisY )
                        return ( fDx <= fHalfW ) && ( ( fDy <= fHalfSide ) || ( ( fDy - fHalfSide ) * fHalfW <= ( fHalfH - fHalfSide ) * ( fHalfW - fDx ) ) );

                    return ( fDy <= fHalfH ) && ( ( fDx <= fHalfSide ) || ( ( fDx - fHalfSide ) * fHalfH <= ( fHalfW - fHalfSide ) * ( fHalfH - fDy ) ) );
                }

                // Whether the row (SA_Y) or column (SA_X) at nLineIndex is one of the shifted ones, per
                // libtmx's stagger_index - identical to StaggeredProjection's own helper (hexsidelength plays
                // no part in this axis at all, confirmed in E3a). A missing/unparsed index defaults to SI_ODD,
                // libtmx's own default for a missing "staggerindex" attribute.
                bool IsShiftedLine( int nLineIndex, tmx_stagger_index index )  {

                    bool  bEven = ( ( nLineIndex % 2 ) == 0 );

                    return ( index == SI_EVEN ) ? bEven : !bEven;
                }

                // libtmx's own default for a missing "staggeraxis" attribute (SA_Y), applied the same way to
                // a hand-built tmx_map.
                tmx_stagger_axis EffectiveAxis( tmx_map* pMap )  {

                    return ( pMap -> stagger_axis == SA_X ) ? SA_X : SA_Y;
                }

                // The staggered axis's own pitch (E3a): half the tile's dimension on that axis PLUS half of
                // hexsidelength - confirmed exact against two different hexsidelength values, not derived from
                // first principles. Reduces to StaggeredProjection's own fixed half-tile pitch when
                // hexsidelength is 0.
                float StaggerPitch( int nTileDimension, int nHexSideLength )  {

                    return ( ( float ) nTileDimension + ( float ) nHexSideLength ) / __HALF_TILE_DIVISOR;
                }

                // (col, row) -> the cell's own top-left corner, in map pixels. The non-staggered axis's
                // formula is identical to StaggeredProjection's (confirmed hexsidelength-independent in E3a);
                // the staggered axis uses StaggerPitch instead of a fixed half-tile step.
                SunLight :: Base :: stCoordinate2D CellTopLeft( int nCol, int nRow, tmx_map* pMap )  {

                    float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                    float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                    if( EffectiveAxis( pMap ) == SA_Y )  {
                        float  fPitchY = StaggerPitch( pMap -> tile_height, pMap -> hexsidelength );
                        float  fShiftX = IsShiftedLine( nRow, pMap -> stagger_index ) ? fHalfW : 0.0f;

                        return SunLight :: Base :: stCoordinate2D { ( int ) ( nCol * ( float ) pMap -> tile_width + fShiftX ),
                                                                     ( int ) ( nRow * fPitchY ) };
                    }

                    float  fPitchX = StaggerPitch( pMap -> tile_width, pMap -> hexsidelength );
                    float  fShiftY = IsShiftedLine( nCol, pMap -> stagger_index ) ? fHalfH : 0.0f;

                    return SunLight :: Base :: stCoordinate2D { ( int ) ( nCol * fPitchX ),
                                                                 ( int ) ( nRow * ( float ) pMap -> tile_height + fShiftY ) };
                }
            }

            /**
             * @brief The hexagonal grid's box: on the staggered axis the lines a pitch ((tile + hexsidelength) / 2) apart plus the last cell's remainder, on the other whole tiles plus the half a tile the shifted lines stick out.
             * @param pMap The map, as libtmx parsed it;
             * @return The map's pixel size;
             */
            SunLight :: Base :: stSize2D HexagonalProjection :: MapPixelSize( tmx_map* pMap ) const  {

                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                if( EffectiveAxis( pMap ) == SA_Y )  {
                    float  fHeight = ( ( pMap -> height * ( ( float ) pMap -> tile_height + pMap -> hexsidelength ) ) +
                                        ( ( float ) pMap -> tile_height - pMap -> hexsidelength ) ) / __HALF_TILE_DIVISOR;

                    return SunLight :: Base :: stSize2D { ( int ) ( pMap -> width * pMap -> tile_width + fHalfW ),
                                                           ( int ) fHeight };
                }

                float  fWidth = ( ( pMap -> width * ( ( float ) pMap -> tile_width + pMap -> hexsidelength ) ) +
                                   ( ( float ) pMap -> tile_width - pMap -> hexsidelength ) ) / __HALF_TILE_DIVISOR;

                return SunLight :: Base :: stSize2D { ( int ) fWidth,
                                                       ( int ) ( pMap -> height * pMap -> tile_height + fHalfH ) };
            }

            /**
             * @brief The cell's top-left corner (CellTopLeft): whole tiles on the unstaggered axis, a pitch per line on the staggered one, with each shifted line (stagger_index) moved half a tile along the other axis; plus the layer's offset.
             * @param pos The cell's row and column;
             * @param pMap The map, as libtmx parsed it;
             * @param nTileWidth Unused: the lattice steps by the map's own tile size;
             * @param nTileHeight Unused, likewise;
             * @param nLayerOffsetX The layer's own x offset, added to the cell's corner;
             * @param nLayerOffsetY The layer's own y offset, likewise;
             * @return The cell's top-left corner, in map pixels;
             */
            SunLight :: Base :: stCoordinate2D HexagonalProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition& pos,
                                                                                         tmx_map* pMap,
                                                                                         int /* nTileWidth */,
                                                                                         int /* nTileHeight */,
                                                                                         int nLayerOffsetX,
                                                                                         int nLayerOffsetY ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );

                return SunLight :: Base :: stCoordinate2D { cell.x + nLayerOffsetX, cell.y + nLayerOffsetY };
            }

            /**
             * @brief A true hexagon hit-test (I17), StaggeredProjection's method with the pitch: the rectangle two pitches long anchored at an unshifted line, its hexagon (InHexagon) for that line's cell, and the rest of the rectangle for the shifted lines either side. False outside the map's pixel rectangle and on its ragged edges, outside every hexagon.
             * @param coord The coordinate, in map pixels;
             * @param pMap The map, as libtmx parsed it;
             * @param nMapPixelWidth MapPixelSize's width, already computed;
             * @param nMapPixelHeight MapPixelSize's height, likewise;
             * @param pos Receives the cell's row and column, when the point is in a cell;
             * @return Whether the point is in one of the map's cells;
             */
            bool HexagonalProjection :: MapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
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

                // A true hexagon hit-test (I17 of the master plan), the same method as StaggeredProjection's:
                // on the staggered axis the lines are a pitch apart (StaggerPitch) and the boxes overlap there,
                // so a box cannot tell which hexagon a point is in. The unshifted lines tile the plane with
                // rectangles one tile wide and two pitches long (the hexagon, then the shifted cells' sides
                // beside it); a point in the rectangle's hexagon is that cell, and every other point belongs
                // to the shifted line before or after it, whose cells start half a tile further along. The
                // hexagon test is inclusive and the rest goes to the neighbours, so every point falls in
                // exactly one cell. It used to floor against the pitch and the shifted line's half tile, which
                // got a cell's centre and its sides right but put the tip past the pitch in the next line
                // (found checking Scarab's report: the bottom tip of every hexagon on axis Y, the right one
                // on axis X, picked a neighbour).
                bool   bAxisY    = ( EffectiveAxis( pMap ) == SA_Y );
                float  fTileW    = ( float ) pMap -> tile_width;
                float  fTileH    = ( float ) pMap -> tile_height;
                float  fHalfSide = ( float ) pMap -> hexsidelength / __HALF_TILE_DIVISOR;
                int    nFirst    = IsShiftedLine( __NO_GRID_CELLS, pMap -> stagger_index ) ? __NEXT_LINE : __NO_GRID_CELLS;
                int    nRow;
                int    nCol;

                if( bAxisY )  {
                    float  fPitch = StaggerPitch( pMap -> tile_height, pMap -> hexsidelength );
                    int    nPair  = ( int ) std :: floor( ( fMapY - nFirst * fPitch ) / ( fPitch * __LINES_PER_PAIR ) );
                    int    nLine  = nFirst + ( nPair * __LINES_PER_PAIR );
                    int    nRectX = ( int ) std :: floor( fMapX / fTileW );
                    float  fPx    = fMapX - ( nRectX * fTileW );
                    float  fPy    = fMapY - ( nLine * fPitch );

                    nRow = nLine;
                    nCol = nRectX;

                    if( !InHexagon( fPx, fPy, fHalfW, fHalfH, fHalfSide, true ) )  {
                        nRow = nLine + ( ( fPy < fHalfH ) ? -__NEXT_LINE : __NEXT_LINE );
                        nCol = nRectX - ( ( fPx < fHalfW ) ? __NEXT_LINE : __NO_GRID_CELLS );   // a shifted cell starts half a tile right
                    }
                }
                else  {
                    float  fPitch = StaggerPitch( pMap -> tile_width, pMap -> hexsidelength );
                    int    nPair  = ( int ) std :: floor( ( fMapX - nFirst * fPitch ) / ( fPitch * __LINES_PER_PAIR ) );
                    int    nLine  = nFirst + ( nPair * __LINES_PER_PAIR );
                    int    nRectY = ( int ) std :: floor( fMapY / fTileH );
                    float  fPx    = fMapX - ( nLine * fPitch );
                    float  fPy    = fMapY - ( nRectY * fTileH );

                    nCol = nLine;
                    nRow = nRectY;

                    if( !InHexagon( fPx, fPy, fHalfW, fHalfH, fHalfSide, false ) )  {
                        nCol = nLine + ( ( fPx < fHalfW ) ? -__NEXT_LINE : __NEXT_LINE );
                        nRow = nRectY - ( ( fPy < fHalfH ) ? __NEXT_LINE : __NO_GRID_CELLS );   // a shifted cell starts half a tile down
                    }
                }

                // Inside the map's pixel rectangle but outside every hexagon (its ragged edges): no cell.
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
            SunLight :: Base :: stSize2D HexagonalProjection :: DefaultScrollStep( tmx_map* pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            /**
             * @brief Each axis's exact bound against a cell's whole tile rectangle - on the staggered axis through the pitch - clamped to the grid.
             * @param pMap The map, as libtmx parsed it;
             * @param mapPixelRect The rectangle to test, in map pixel space;
             * @return The matrix range that could touch it, clamped to the map's grid;
             */
            stTileRange HexagonalProjection :: VisibleTileRange( tmx_map* pMap,
                                                                  const SunLight :: Base :: stRectangle& mapPixelRect ) const  {

                float  fHalfW  = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH  = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;
                float  fLeft   = mapPixelRect.x;
                float  fRight  = mapPixelRect.x + mapPixelRect.width;
                float  fTop    = mapPixelRect.y;
                float  fBottom = mapPixelRect.y + mapPixelRect.height;

                stTileRange  range;

                // Derived (not guessed) from each axis's own intersection condition against a cell's full
                // tileWidth x tileHeight rectangle: the non-staggered axis is unaffected by hexsidelength (the
                // same formula StaggeredProjection already uses); the staggered axis uses the general,
                // unsimplified exact bound (not StaggeredProjection's "-1" shortcut, which relied on its pitch
                // being exactly half the tile dimension - not true here once hexsidelength isn't 0).
                if( EffectiveAxis( pMap ) == SA_Y )  {
                    float  fPitchY = StaggerPitch( pMap -> tile_height, pMap -> hexsidelength );

                    range.nRowStart = ( int ) std :: floor( ( fTop - ( float ) pMap -> tile_height ) / fPitchY ) + 1;
                    range.nRowEnd   = ( int ) std :: ceil( fBottom / fPitchY );
                    range.nColStart = ( int ) std :: floor( ( fLeft - fHalfW ) / ( float ) pMap -> tile_width );
                    range.nColEnd   = ( int ) std :: ceil( fRight / ( float ) pMap -> tile_width );
                }
                else  {
                    float  fPitchX = StaggerPitch( pMap -> tile_width, pMap -> hexsidelength );

                    range.nColStart = ( int ) std :: floor( ( fLeft - ( float ) pMap -> tile_width ) / fPitchX ) + 1;
                    range.nColEnd   = ( int ) std :: ceil( fRight / fPitchX );
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
