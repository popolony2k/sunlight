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
                tmx_stagger_axis EffectiveAxis( tmx_map *pMap )  {

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
                SunLight :: Base :: stCoordinate2D CellTopLeft( int nCol, int nRow, tmx_map *pMap )  {

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

            SunLight :: Base :: stSize2D HexagonalProjection :: MapPixelSize( tmx_map *pMap ) const  {

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

            SunLight :: Base :: stCoordinate2D HexagonalProjection :: TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                                         tmx_map *pMap,
                                                                                         int /* nTileWidth */,
                                                                                         int /* nTileHeight */,
                                                                                         int nLayerOffsetX,
                                                                                         int nLayerOffsetY ) const  {

                SunLight :: Base :: stCoordinate2D  cell = CellTopLeft( pos.nTileCol, pos.nTileRow, pMap );

                return SunLight :: Base :: stCoordinate2D { cell.x + nLayerOffsetX, cell.y + nLayerOffsetY };
            }

            SunLight :: Base :: stDimension2D HexagonalProjection :: TileViewRect( const SunLight :: TileMap :: stMatrixPosition &pos,
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

            bool HexagonalProjection :: ViewToTileMatrix( const SunLight :: Base :: stCoordinate2D &coord,
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

                // A single direct floor against the cell's own line shift, no second overlap candidate - the
                // same simplification StaggeredProjection settled on after mutation-testing showed the extra
                // candidate was dead weight for the only precision this method promises (a point near a cell's
                // own anchor corner). Re-verified here, not assumed to carry over unchanged (E3a's own note):
                // see test_hexagonalprojection.cpp's round-trip suite and its own mutation check.
                if( EffectiveAxis( pMap ) == SA_Y )  {
                    float  fPitchY = StaggerPitch( pMap -> tile_height, pMap -> hexsidelength );
                    int    nRow    = ( int ) std :: floor( fMapY / fPitchY );
                    float  fShiftX = IsShiftedLine( nRow, pMap -> stagger_index ) ? fHalfW : 0.0f;
                    int    nCol    = ( int ) std :: floor( ( fMapX - fShiftX ) / ( float ) pMap -> tile_width );

                    pos = { nRow, nCol };
                }
                else  {
                    float  fPitchX = StaggerPitch( pMap -> tile_width, pMap -> hexsidelength );
                    int    nCol    = ( int ) std :: floor( fMapX / fPitchX );
                    float  fShiftY = IsShiftedLine( nCol, pMap -> stagger_index ) ? fHalfH : 0.0f;
                    int    nRow    = ( int ) std :: floor( ( fMapY - fShiftY ) / ( float ) pMap -> tile_height );

                    pos = { nRow, nCol };
                }

                return true;
            }

            SunLight :: Base :: stSize2D HexagonalProjection :: DefaultScrollStep( tmx_map *pMap ) const  {

                return SunLight :: Base :: stSize2D { ( int ) pMap -> tile_width, ( int ) pMap -> tile_height };
            }

            stTileRange HexagonalProjection :: VisibleTileRange( tmx_map *pMap,
                                                                  const SunLight :: Base :: stRectangle &mapPixelRect ) const  {

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
