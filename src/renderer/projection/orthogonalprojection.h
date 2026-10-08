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

#ifndef __ORTHOGONALPROJECTION_H__
#define __ORTHOGONALPROJECTION_H__

#include "renderer/projection/imapprojection.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            /**
             * @brief The grid every orthogonal map already drew with, moved here unchanged (E0 of the master
             * plan): a cell's screen position is col * tileWidth, row * tileHeight. Every formula here is the
             * same arithmetic TileMapRenderer used to do inline, so an orthogonal map's output does not change.
             */
            class OrthogonalProjection : public IMapProjection  {

                public:

                SunLight :: Base :: stSize2D MapPixelSize( tmx_map *pMap ) const override;

                SunLight :: Base :: stCoordinate2D TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
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
#endif  /* __ORTHOGONALPROJECTION_H__ */
