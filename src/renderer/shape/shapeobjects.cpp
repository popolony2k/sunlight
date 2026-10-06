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

#include "renderer/shape/shapeobjects.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Shape  {

            namespace  {

                const char * const  __LINE_WIDTH_PROPERTY  = "line_width";
                const double        __DEFAULT_LINE_WIDTH   = 1.0;
            }

            double LineWidthOf( tmx_object *pObject )  {

                tmx_property *pProperty = pObject -> properties ? tmx_get_property( pObject -> properties, __LINE_WIDTH_PROPERTY ) : nullptr;

                if( pProperty == nullptr )
                    return __DEFAULT_LINE_WIDTH;

                switch( pProperty -> type )  {
                    case PT_INT :
                        return ( double ) pProperty -> value.integer;

                    case PT_FLOAT :
                        return ( double ) pProperty -> value.decimal;

                    default :
                        return __DEFAULT_LINE_WIDTH;
                }
            }
        }
    }
}
