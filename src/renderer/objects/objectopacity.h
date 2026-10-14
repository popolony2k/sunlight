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

#ifndef __OBJECTOPACITY_H__
#define __OBJECTOPACITY_H__

#include <tmx.h>
#include <set>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief The opacity an object is drawn with (L2 of the master plan; a class of its own
         * since S1b): its own "opacity" custom property (0 to 1) when it has a usable one,
         * otherwise its layer's. An unusable property - the wrong type, or outside 0 to 1 -
         * falls back to the layer's too, with a warning logged once per object id; Reset()
         * forgets those warnings.
         */
        class ObjectOpacity  {

            std :: set<unsigned int>  m_WarnedObjects;   // object ids already warned about an invalid "opacity" property

            void WarnInvalid( unsigned int nObjectId );

            public:

            float Of( tmx_object *pObject, float fLayerOpacity );
            void Reset( void );
        };
    }
}

#endif  /* __OBJECTOPACITY_H__ */
