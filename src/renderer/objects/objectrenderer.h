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

#ifndef __OBJECTRENDERER_H__
#define __OBJECTRENDERER_H__

#include "tmx.h"
#include "renderer/drawcontext.h"
#include "renderer/shape/shaperenderer.h"
#include "renderer/objects/textobjectrenderer.h"
#include "renderer/objects/objectopacity.h"
#include "renderer/text/fontregistry.h"
#include <functional>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Draws a Tiled object layer (S1b of the master plan): each visible object, in the
         * layer's order, by its kind - shapes through a ShapeRenderer, text through a
         * TextObjectRenderer - with its effective opacity (ObjectOpacity: its own "opacity"
         * property, or its layer's). It owns the opacity rule and its warnings; the fonts are
         * the caller's (FontRegistry, passed in).
         *
         * A tile object is handed back to the caller (TILE_OBJECT_DRAWER), with its effective
         * opacity: drawing one needs the map's tiles and their animation, which belong to the
         * renderer that holds the map.
         */
        class ObjectRenderer  {

            ShapeRenderer       m_Shapes;
            TextObjectRenderer  m_Text;
            ObjectOpacity       m_Opacity;

            public:

            /**
             * @brief Draws one tile object (OT_TILE): the object, its object group, and the
             * opacity to draw it with.
             */
            typedef std :: function<void( tmx_object*, tmx_layer*, float )> TILE_OBJECT_DRAWER;

            void Draw( const DrawContext &context,
                       tmx_layer *pLayer,
                       FontRegistry &fonts,
                       const TILE_OBJECT_DRAWER &drawTileObject );

            void ResetWarnings( void );
        };
    }
}

#endif  /* __OBJECTRENDERER_H__ */
