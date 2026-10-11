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
                tmx_stagger_axis EffectiveAxis( tmx_map *pMap )  {

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
                SunLight :: Base :: stCoordinate2D CellTopLeft( int nCol, int nRow, tmx_map *pMap )  {

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

            SunLight :: Base :: stSize2D StaggeredProjection :: MapPixelSize( tmx_map *pMap ) const  {

                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                if( EffectiveAxis( pMap ) == SA_Y )  {
                    return SunLight :: Base :: stSize2D { ( int ) ( pMap -> width * pMap -> tile_width + fHalfW ),
                                                           ( int ) ( ( pMap -> height + 1 ) * fHalfH ) };
                }

                return SunLight :: Base :: stSize2D { ( int ) ( ( pMap -> width + 1 ) * fHalfW ),
                                                       ( int ) ( pMap -> height * pMap -> tile_height + fHalfH ) };
            }

            SunLight :: Base :: stCoordinate2D StaggeredProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                                         tmx_map *pMap,
                                                                                         int /* nTileWidth */,
                                                                                         int /* nTileHeight */,
                                                                                         int nLayerOffsetX,
                                                                                         int nLayerOffsetY ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );

                return SunLight :: Base :: stCoordinate2D { cell.x + nLayerOffsetX, cell.y + nLayerOffsetY };
            }

            SunLight :: Base :: stDimension2D StaggeredProjection :: TileViewRect( const SunLight :: TileMap :: stMatrixPosition &pos,
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

            bool StaggeredProjection :: ViewToTileMatrix( const SunLight :: Base :: stCoordinate2D &coord,
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

                float  fMapX  = ( coord.x + viewport.pos.x ) - camera.x;
                float  fMapY  = ( coord.y + viewport.pos.y ) - camera.y;
                float  fHalfW = ( float ) pMap -> tile_width  / __HALF_TILE_DIVISOR;
                float  fHalfH = ( float ) pMap -> tile_height / __HALF_TILE_DIVISOR;

                // The staggered lattice's cells overlap their neighbour on the staggered axis by half (E2a),
                // the same shape of ambiguity IsometricProjection's own ViewToTileMatrix has - but unlike
                // that lattice's coupled (col, row) solve, a single floor directly against the OWN line's
                // shift is already exact for this method's only required precision, a point near a cell's
                // own anchor corner: a first version of this method tried a neighbouring second candidate
                // and picked whichever one's own corner was closer, reasoning the overlap could make the
                // naive floor land one line short; mutation-testing that reasoning (forcing the first,
                // un-disambiguated candidate on every call) left the full per-cell round-trip suite - every
                // cell of two differently-staggered 4x3 grids - still passing, proving the second candidate
                // never actually won the tie-break for the precision this interface promises. Removed rather
                // than kept as untested, unexercised complexity.
                if( EffectiveAxis( pMap ) == SA_Y )  {
                    int    nRow    = ( int ) std :: floor( fMapY / fHalfH );
                    float  fShiftX = IsShiftedLine( nRow, pMap -> stagger_index ) ? fHalfW : 0.0f;
                    int    nCol    = ( int ) std :: floor( ( fMapX - fShiftX ) / ( float ) pMap -> tile_width );

                    pos = { nRow, nCol };
                }
                else  {
                    int    nCol    = ( int ) std :: floor( fMapX / fHalfW );
                    float  fShiftY = IsShiftedLine( nCol, pMap -> stagger_index ) ? fHalfH : 0.0f;
                    int    nRow    = ( int ) std :: floor( ( fMapY - fShiftY ) / ( float ) pMap -> tile_height );

                    pos = { nRow, nCol };
                }

                return true;
            }

            SunLight :: Base :: stSize2D StaggeredProjection :: DefaultScrollStep( tmx_map *pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            stTileRange StaggeredProjection :: VisibleTileRange( tmx_map *pMap,
                                                                  const SunLight :: Base :: stRectangle &mapPixelRect ) const  {

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
