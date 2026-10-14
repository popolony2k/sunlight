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

#ifndef __DRAWCONTEXT_H__
#define __DRAWCONTEXT_H__

#include "base/primitives.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Everything a drawing routine needs to put something given in map units on screen
         * (S1a of the master plan): a point at map position p lands at `(p + camera) x fZoom +
         * viewport.pos`, and is cut at the viewport's rectangle. The renderer builds one for the
         * view and layer being drawn (TileMapRenderer::CurrentDrawContext); a drawing class takes
         * it instead of reaching into the renderer, so it can draw for any renderer.
         *
         * The field types are the ones the renderer itself holds - an int rectangle (Viewport's),
         * a float zoom factor (stZoomProperties::fZoomFactor) and a float camera (stVector2D) -
         * so arithmetic done from a DrawContext is the same arithmetic, bit for bit.
         */
        struct DrawContext  {
            SunLight :: Base :: stDimension2D  viewport;   // the view's rectangle, in screen pixels
            float                              fZoom;      // the view's zoom factor
            SunLight :: Base :: stVector2D     camera;     // the camera offset of the layer being drawn (its parallax applied), in map units
        };
    }
}

#endif  /* __DRAWCONTEXT_H__ */
