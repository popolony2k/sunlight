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

#include <cmath>

namespace SunLight  {
    namespace Renderer  {
        namespace Shape  {

            namespace  {

                const char * const  __LINE_WIDTH_PROPERTY  = "line_width";
                const char * const  __POINT_SIZE_PROPERTY  = "point_size";
                const double        __DEFAULT_LINE_WIDTH   = 1.0;
                const double        __DEFAULT_POINT_SIZE   = 1.0;
                const double        __DEGREES_TO_RADIANS   = 3.14159265358979323846 / 180.0;
                const double        __ROTATION_EPSILON     = 1e-12;   // a cosine or sine this close to zero is zero

                // A numeric property of the object (Tiled stores it as an int or a float), or the default when it has none.
                double NumberPropertyOf( tmx_object *pObject, const char *szName, double fDefault )  {

                    tmx_property *pProperty = pObject -> properties ? tmx_get_property( pObject -> properties, szName ) : nullptr;

                    if( pProperty == nullptr )
                        return fDefault;

                    switch( pProperty -> type )  {
                        case PT_INT :
                            return ( double ) pProperty -> value.integer;

                        case PT_FLOAT :
                            return ( double ) pProperty -> value.decimal;

                        default :
                            return fDefault;
                    }
                }
            }

            double LineWidthOf( tmx_object *pObject )  {

                return NumberPropertyOf( pObject, __LINE_WIDTH_PROPERTY, __DEFAULT_LINE_WIDTH );
            }

            MapPoint Rotate( MapPoint point, double fDegrees )  {

                double  fCos = std :: cos( fDegrees * __DEGREES_TO_RADIANS );
                double  fSin = std :: sin( fDegrees * __DEGREES_TO_RADIANS );

                if( std :: fabs( fCos ) < __ROTATION_EPSILON )
                    fCos = 0.0;

                if( std :: fabs( fSin ) < __ROTATION_EPSILON )
                    fSin = 0.0;

                return MapPoint { ( point.fX * fCos ) - ( point.fY * fSin ), ( point.fX * fSin ) + ( point.fY * fCos ) };
            }

            double RotationOf( tmx_object *pObject )  {

                return pObject -> rotation;
            }

            double PointSizeOf( tmx_object *pObject )  {

                return NumberPropertyOf( pObject, __POINT_SIZE_PROPERTY, __DEFAULT_POINT_SIZE );
            }
        }
    }
}
