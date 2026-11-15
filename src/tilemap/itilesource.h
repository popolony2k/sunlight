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

#ifndef __ITILESOURCE_H__
#define __ITILESOURCE_H__

#include <vector>
#include "base/primitives.h"
#include "tilemap/tilemapdefs.h"


namespace SunLight  {
    namespace TileMap  {

        /**
         * @brief What a collision manager asks about the map its rules are tested against (I19c of
         * the master plan): a layer by id, and the tiles of a tile layer a collider touches. Each
         * world answers for its own map (IWorld, through its CollisionManager), so a tile rule added
         * to one world's manager never reads another world's map. ITileMap is one too - the
         * renderer answers for its default world - so a manager made over a renderer, or over a
         * test double of one, works as before.
         */
        class ITileSource  {

            public:

            virtual ~ITileSource( void ) {}

            /**
             * Get the layer parameters.
             * @param nLayerId The layer id to get layer parameters;
             * @param layer reference to layer parameters structure to get;
             */
            virtual bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer ) = 0;

            /**
             * @brief The tiles of a tile layer that a collider touches (I7 of the master plan): every
             * tile whose Tiled collision rectangle overlaps rect, in MAP space, each reported once with
             * its dimension in map space (as GetTile gives it). Rectangles that only share an edge do
             * not overlap, so a sprite resting flush against a wall does not touch it. Only rectangle
             * collision shapes are tested.
             * @param rect The collider's rectangle: a map position when bWorldSpace, otherwise relative
             * to the view (a sprite's own position), which is turned into a map position with the camera
             * the layer is drawn with;
             * @param bWorldSpace Whether rect is already a map position;
             * @param layer The tile layer;
             * @param tiles Receives the tiles touched, in row then column order (emptied first);
             * @return Whether the layer could be tested (a map is loaded and layer is a tile layer);
             */
            virtual bool GetCollidingTiles( const SunLight :: Base :: stRectangle& rect,
                                            bool bWorldSpace,
                                            const SunLight :: TileMap :: stLayer& layer,
                                            std :: vector<SunLight :: TileMap :: stTile>& tiles ) = 0;
        };
    }
}

#endif /* __ITILESOURCE_H__ */
