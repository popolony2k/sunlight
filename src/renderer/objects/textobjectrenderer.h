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

#ifndef __TEXTOBJECTRENDERER_H__
#define __TEXTOBJECTRENDERER_H__

#include "tmx.h"
#include "renderer/drawcontext.h"
#include "renderer/text/fontregistry.h"
#include "renderer/objects/objectopacity.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Draws Tiled's text objects (OT_TEXT) through a DrawContext (S1b of the master
         * plan). It holds no state and knows nothing about TileMapRenderer: the fonts come from a
         * FontRegistry and the opacity rule from an ObjectOpacity, both passed in.
         */
        class TextObjectRenderer  {

            public:

            void Draw( const DrawContext &context,
                       tmx_object *pObject,
                       tmx_layer *pLayer,
                       FontRegistry &fonts,
                       ObjectOpacity &opacity ) const;
        };
    }
}

#endif  /* __TEXTOBJECTRENDERER_H__ */
