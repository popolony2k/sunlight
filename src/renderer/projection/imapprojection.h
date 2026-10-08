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

#ifndef __IMAPPROJECTION_H__
#define __IMAPPROJECTION_H__

#include "tmx.h"
#include "base/primitives.h"
#include "tilemap/tilemapdefs.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Projection  {

            /**
             * @brief A tile matrix range a draw pass should visit: [nRowStart, nRowEnd) by [nColStart, nColEnd),
             * both ends exclusive on the high side, already clamped to the map's own grid.
             */
            struct stTileRange  {
                int  nRowStart;
                int  nRowEnd;
                int  nColStart;
                int  nColEnd;
            };

            /**
             * @brief How a map's orientation turns a tile matrix position into pixels, and back. TileMapRenderer
             * owns everything that is the same for every orientation - the view/camera/clip machinery, sprites,
             * collision dispatch, shape/text/tile object drawing, animation, the frame loop - and asks the active
             * projection only the handful of questions that genuinely differ between orthogonal, isometric,
             * staggered and hexagonal maps. Camera, zoom, viewport origin and the culling padding are the same
             * arithmetic for every orientation, so they stay in TileMapRenderer; a projection only answers pure
             * tile-grid geometry.
             */
            class IMapProjection  {

                public:

                virtual ~IMapProjection( void ) = default;

                /**
                 * @brief The map's own pixel bounding box - a rectangle for orthogonal, a diamond's bounding box
                 * for isometric. Computed once, at LoadMap; everywhere else it is consumed as an opaque size (the
                 * alignment switch, the scroll clamps, the bounds check in ViewToTileMatrix).
                 * @param pMap The map, as libtmx parsed it;
                 * @return The map's pixel size;
                 */
                virtual SunLight :: Base :: stSize2D MapPixelSize( tmx_map *pMap ) const = 0;

                /**
                 * @brief A tile layer cell's own top-left corner, in map pixel space (no camera, zoom or viewport
                 * origin - those are applied afterwards, identically for every orientation, by DrawTile).
                 * @param pos The cell's row and column;
                 * @param nTileWidth Width of the tile actually drawn there (its own tileset's, not the map's);
                 * @param nTileHeight Height, likewise;
                 * @param nLayerOffsetX The layer's own x offset;
                 * @param nLayerOffsetY The layer's own y offset;
                 * @return The cell's top-left corner, in map pixels;
                 */
                virtual SunLight :: Base :: stCoordinate2D TileDrawPosition( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                              int nTileWidth,
                                                                              int nTileHeight,
                                                                              int nLayerOffsetX,
                                                                              int nLayerOffsetY ) const = 0;

                /**
                 * @brief A tile matrix cell's rectangle in view space (camera and viewport origin applied, zoom
                 * not), used only for collision - CollisionManager compares a sprite's own view-space rectangle
                 * against this one.
                 * @param pos The cell's row and column;
                 * @param pMap The map, as libtmx parsed it;
                 * @param viewport The active view's own viewport rectangle;
                 * @param camera The active view's camera position;
                 * @return The cell's rectangle, in view space;
                 */
                virtual SunLight :: Base :: stDimension2D TileViewRect( const SunLight :: TileMap :: stMatrixPosition &pos,
                                                                         tmx_map *pMap,
                                                                         const SunLight :: Base :: stDimension2D &viewport,
                                                                         const SunLight :: Base :: stVector2D &camera ) const = 0;

                /**
                 * @brief The inverse of TileViewRect: which matrix cell a view-space coordinate falls in. Only
                 * checks that the coordinate is inside the map's own pixel bounds (see MapPixelSize); the final
                 * check against the matrix's actual row/column counts is the same for every orientation, so it
                 * stays in TileMapRenderer.
                 * @param coord The coordinate, in view space;
                 * @param pMap The map, as libtmx parsed it;
                 * @param viewport The active view's own viewport rectangle;
                 * @param camera The active view's camera position;
                 * @param fZoom The active view's zoom factor;
                 * @param nMapPixelWidth MapPixelSize's width, already computed;
                 * @param nMapPixelHeight MapPixelSize's height, likewise;
                 * @param pos Receives the matrix position, when the coordinate is inside the map's pixel bounds;
                 * @return Whether the coordinate is inside the map's pixel bounds;
                 */
                virtual bool ViewToTileMatrix( const SunLight :: Base :: stCoordinate2D &coord,
                                               tmx_map *pMap,
                                               const SunLight :: Base :: stDimension2D &viewport,
                                               const SunLight :: Base :: stVector2D &camera,
                                               float fZoom,
                                               int nMapPixelWidth,
                                               int nMapPixelHeight,
                                               SunLight :: TileMap :: stMatrixPosition &pos ) const = 0;

                /**
                 * @brief The scroll step a view starts with, when its own has not been set.
                 * @param pMap The map, as libtmx parsed it;
                 * @return The default step, in map pixels;
                 */
                virtual SunLight :: Base :: stSize2D DefaultScrollStep( tmx_map *pMap ) const = 0;

                /**
                 * @brief Which tile matrix cells could touch a map-pixel rectangle, clamped to the map's own grid.
                 * The caller has already turned the viewport, camera, zoom and culling padding into this one
                 * rectangle, in the same map-pixel space TileDrawPosition answers in - that bookkeeping is the
                 * same for every orientation, so it is not repeated here.
                 * @param pMap The map, as libtmx parsed it;
                 * @param mapPixelRect The rectangle to test, in map pixel space;
                 * @return The matrix range that could touch it;
                 */
                virtual stTileRange VisibleTileRange( tmx_map *pMap,
                                                      const SunLight :: Base :: stRectangle &mapPixelRect ) const = 0;
            };
        }
    }
}
#endif  /* __IMAPPROJECTION_H__ */
