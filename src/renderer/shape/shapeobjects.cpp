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
                const char * const  __OPACITY_PROPERTY     = "opacity";
                const double        __DEFAULT_LINE_WIDTH   = 1.0;
                const double        __DEFAULT_POINT_SIZE   = 1.0;
                const double        __OPACITY_MIN          = 0.0;
                const double        __OPACITY_MAX          = 1.0;
                const double        __DEGREES_TO_RADIANS   = 3.14159265358979323846 / 180.0;
                const double        __ROTATION_EPSILON     = 1e-12;   // a cosine or sine this close to zero is zero
                const unsigned      __ALPHA_SHIFT          = 24;      // libtmx's packed colour: 0xAARRGGBB
                const unsigned      __RED_SHIFT            = 16;
                const unsigned      __GREEN_SHIFT          = 8;

                // A numeric property of the object (Tiled stores it as an int or a float), or the default when it has none.
                double NumberPropertyOf( tmx_object* pObject, const char* szName, double fDefault )  {

                    tmx_property* pProperty = pObject -> properties ? tmx_get_property( pObject -> properties, szName ) : nullptr;

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

            SunLight :: Base :: stColor ColorOf( uint32_t nColor )  {

                tmx_col_bytes  res = ::tmx_col_to_bytes( nColor );

                return *( ( SunLight :: Base :: stColor * ) &res );
            }

            /**
             * @brief The inverse of ColorOf: libtmx's ARGB layout (tmx_col_to_bytes reads alpha from the
             * top byte, then red, green and blue).
             * @param color The colour;
             * @return The packed ARGB value;
             */
            uint32_t ArgbOf( SunLight :: Base :: stColor color )  {

                return ( ( uint32_t ) color.nAlpha << __ALPHA_SHIFT ) | ( ( uint32_t ) color.nRed << __RED_SHIFT ) |
                       ( ( uint32_t ) color.nGreen << __GREEN_SHIFT ) | ( uint32_t ) color.nBlue;
            }


            double LineWidthOf( tmx_object* pObject )  {

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

            double RotationOf( tmx_object* pObject )  {

                return pObject -> rotation;
            }

            double PointSizeOf( tmx_object* pObject )  {

                return NumberPropertyOf( pObject, __POINT_SIZE_PROPERTY, __DEFAULT_POINT_SIZE );
            }

            OpacityProperty ObjectOpacityOf( tmx_object* pObject, double& fValue )  {

                tmx_property* pProperty = pObject -> properties ? tmx_get_property( pObject -> properties, __OPACITY_PROPERTY ) : nullptr;

                if( pProperty == nullptr )
                    return OpacityProperty :: NONE;

                double  fRaw;

                switch( pProperty -> type )  {
                    case PT_INT :
                        fRaw = ( double ) pProperty -> value.integer;
                        break;

                    case PT_FLOAT :
                        fRaw = ( double ) pProperty -> value.decimal;
                        break;

                    default :
                        return OpacityProperty :: INVALID;
                }

                if( ( fRaw < __OPACITY_MIN ) || ( fRaw > __OPACITY_MAX ) )
                    return OpacityProperty :: INVALID;

                fValue = fRaw;
                return OpacityProperty :: VALID;
            }
        }
    }
}
