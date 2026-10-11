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

#ifndef __STAGGEREDPROJECTION_H__
#define __STAGGEREDPROJECTION_H__

#include "renderer/projection/imapprojection.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            /**
             * @brief Tiled's staggered grid (E2a of the master plan): axis-aligned rectangular cells, but every
             * other row or column (libtmx's `stagger_axis`, SA_X/SA_Y) is shifted by half a tile along the OTHER
             * axis, and which parity is shifted is libtmx's `stagger_index` (SI_EVEN/SI_ODD). Found by measuring
             * real Tiled exports (`tmxrasterizer`, pixel-exact across all 4 axis/index combinations and 3 map
             * sizes each - a row-count-only and a column-count-only variant, so a row-dependent and a
             * column-dependent term in the map's own pixel bounds could not be confused with each other), not
             * assumed:
             *   SA_Y: screenY = row * (tileHeight / 2); screenX = col * tileWidth, plus tileWidth / 2 when the
             *         row's own parity matches stagger_index (an even row for SI_EVEN, an odd row for SI_ODD).
             *   SA_X: screenX = col * (tileWidth / 2); screenY = row * tileHeight, plus tileHeight / 2 when the
             *         column's own parity matches stagger_index - the exact mirror, confirmed independently
             *         rather than assumed from the symmetry.
             * The row (SA_Y) or column (SA_X) pitch is only half a tile step, so consecutive cells on the
             * staggered axis genuinely overlap by half their own rectangle - the same shape of ambiguity E1b
             * found for isometric's diamond lattice. ViewToTileMatrix resolves it the same way isometric's own
             * doc comment describes: AABB-level precision, correct for a point near a cell's own anchor corner,
             * not a true per-shape hit-test (no consumer needs one today).
             *
             * Tile *object* coordinates were not measured (same deferral E1a made for isometric): no projection
             * consumer needs staggered object placement yet.
             */
            class StaggeredProjection : public IMapProjection  {

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
#endif  /* __STAGGEREDPROJECTION_H__ */
