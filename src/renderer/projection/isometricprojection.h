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

#ifndef __ISOMETRICPROJECTION_H__
#define __ISOMETRICPROJECTION_H__

#include "renderer/projection/imapprojection.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            /**
             * @brief Tiled's isometric diamond lattice (E1a of the master plan): a cell's own top-left corner
             * (the corner of its tileWidth x tileHeight bounding box, not the diamond's own vertices) is at
             *   screenX = (col - row + mapHeightCells - 1) * (tileWidth / 2)
             *   screenY = (col + row) * (tileHeight / 2)
             * found by measuring real Tiled exports (pixel-exact across two map sizes, 18 cells, zero
             * mismatches), not assumed from documentation - a formula half-remembered from Tiled's own source
             * (offsetting by mapHeightCells * tileWidth/2, no "-1") looked plausible but was wrong, caught only
             * by testing a second map height. The position always uses the MAP's own declared tile_width/
             * tile_height for the lattice step, never the resolved tile's own (possibly different) image size -
             * also confirmed empirically: tile images smaller than the map's declared cell size still land on
             * the same lattice, only their drawn footprint differs.
             *
             * Tile *objects* use a related but distinct formula (also measured, not derived from the above):
             *   objScreenX = storedX - storedY + (mapHeightCells - 1) * (tileWidth / 2) + tileHeight / 2
             *   objScreenY = (storedX + storedY) / 2
             * at the object's own bottom-left corner, Tiled's usual tile-object anchor (see DrawTileObject).
             * This projection does not implement object placement yet - no IMapProjection method covers it
             * (TileViewRect is collision-only); it is tracked as a follow-up once isometric objects are needed.
             */
            class IsometricProjection : public IMapProjection  {

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
#endif  /* __ISOMETRICPROJECTION_H__ */
