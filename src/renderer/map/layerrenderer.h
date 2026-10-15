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

#ifndef __LAYERRENDERER_H__
#define __LAYERRENDERER_H__

#include "tmx.h"
#include "base/color.h"
#include "base/primitives.h"
#include "renderer/drawcontext.h"
#include "renderer/map/mapdata.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Draws a loaded map's tile and image layers, and its tile objects, through a
         * DrawContext (S1c2 of the master plan). It holds no state and knows nothing about
         * TileMapRenderer: the map's tiles, projection and tile animation come from a MapData,
         * the view from the context. Each tile is drawn whole at its zoomed size, rounded to whole
         * pixels; a tile entirely off the viewport is skipped (Viewport::ClipRect), and the clip
         * the view's pass set cuts the rest at the viewport's edge.
         */
        class LayerRenderer  {

            void DrawTile( const DrawContext& context,
                           SunLight :: Base :: TextureHandle pImage,
                           int32_t nSourceX,
                           int32_t nSourceY,
                           int32_t nSourceW,
                           int32_t nSourceH,
                           int32_t nDestX,
                           int32_t nDestY,
                           float fOpacity,
                           SunLight :: Base :: stColor tintColor ) const;

            public:

            void DrawTileLayer( const DrawContext& context, MapData& map, tmx_layer* pLayer ) const;
            void DrawImageLayer( const DrawContext& context, tmx_layer* pLayer ) const;
            void DrawTileObject( const DrawContext& context, MapData& map, tmx_object* pObject, tmx_layer* pLayer, float fOpacity ) const;
        };
    }
}

#endif /* __LAYERRENDERER_H__ */
