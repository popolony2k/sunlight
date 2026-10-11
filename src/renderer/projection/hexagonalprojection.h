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

#ifndef __HEXAGONALPROJECTION_H__
#define __HEXAGONALPROJECTION_H__

#include "renderer/projection/imapprojection.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            /**
             * @brief Tiled's hexagonal grid (E3a of the master plan): the same shape as `StaggeredProjection`
             * on the axis that is NOT staggered (libtmx's `stagger_axis`/`stagger_index`, exactly as staggered
             * uses them), but the staggered axis's own pitch is no longer a fixed half-tile step - it depends
             * on `hexsidelength`, the length of the hexagon's own two parallel "flat" edges, found by measuring
             * real Tiled exports (pixel-exact across both axes, both stagger indices, two different
             * `hexsidelength` values and 3 map sizes each - a row-count-only and a column-count-only variant,
             * so a row/column-dependent term in the map's own pixel bounds could not be confused with the
             * `hexsidelength` term), not assumed:
             *   SA_Y (pointy-top): screenY = row * (tileHeight + hexsidelength) / 2; screenX = col * tileWidth,
             *         plus tileWidth / 2 when the row's own parity matches stagger_index - unaffected by
             *         hexsidelength, confirmed identical to StaggeredProjection's own SA_Y formula.
             *   SA_X (flat-top): screenX = col * (tileWidth + hexsidelength) / 2; screenY = row * tileHeight,
             *         plus tileHeight / 2 when the column's own parity matches stagger_index - the exact
             *         mirror, confirmed independently rather than assumed from the symmetry.
             * Every formula here reduces exactly to StaggeredProjection's own, already-verified formula when
             * hexsidelength is 0 - not designed that way, but a reassuring cross-check that fell out of the
             * independently-measured numbers.
             *
             * Cell bounding boxes overlap their neighbour on the staggered axis whenever hexsidelength is less
             * than the tile's own dimension on that axis (the normal case for an actual hexagon) - the same
             * shape of ambiguity StaggeredProjection's own doc comment describes for its lattice.
             *
             * Tile *object* coordinates were not measured (same deferral E1a/E2a made for isometric and
             * staggered): no projection consumer needs hexagonal object placement yet.
             */
            class HexagonalProjection : public IMapProjection  {

                public:

                SunLight :: Base :: stSize2D MapPixelSize( tmx_map *pMap ) const override;

                SunLight :: Base :: stCoordinate2D TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                      tmx_map *pMap,
                                                                      int nTileWidth,
                                                                      int nTileHeight,
                                                                      int nLayerOffsetX,
                                                                      int nLayerOffsetY ) const override;

                SunLight :: Base :: stDimension2D TileViewRect( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                 tmx_map *pMap,
                                                                 const SunLight :: Base :: stDimension2D &viewport,
                                                                 const SunLight :: Base :: stVector2D &camera ) const override;

                bool ViewToTileMatrix( const SunLight :: Base :: stCoordinate2D &coord,
                                      tmx_map *pMap,
                                      const SunLight :: Base :: stDimension2D &viewport,
                                      const SunLight :: Base :: stVector2D &camera,
                                      float fZoom,
                                      int nMapPixelWidth,
                                      int nMapPixelHeight,
                                      SunLight :: TileMap :: stMatrixPosition &pos ) const override;

                SunLight :: Base :: stSize2D DefaultScrollStep( tmx_map *pMap ) const override;

                stTileRange VisibleTileRange( tmx_map *pMap,
                                              const SunLight :: Base :: stRectangle &mapPixelRect ) const override;
            };
        }
    }
}
#endif  /* __HEXAGONALPROJECTION_H__ */
