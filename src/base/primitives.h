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

#ifndef __PRIMITIVES_H__
#define __PRIMITIVES_H__


namespace SunLight  {
    namespace Base  {
        /**
         * Rectangle definition structure (backend-agnostic, sub-pixel precision).
         */
        struct stRectangle  {
            float          x;
            float          y;
            float          width;
            float          height;
        };

        /**
         * 2D vector definition structure (backend-agnostic, sub-pixel precision).
         */
        struct stVector2D  {
            float          x;
            float          y;
        };

        /**
         * Opaque handle to a backend-owned texture resource.
         * Only the active @see IEngine implementation knows its real type;
         * every other layer just forwards this pointer around.
         */
        typedef void* TextureHandle;
    }
}

#endif /* __PRIMITIVES_H__ */
