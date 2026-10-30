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

#ifndef __SHAPEOBJECTS_H__
#define __SHAPEOBJECTS_H__

#include "tmx.h"
#include "base/color.h"

namespace SunLight  {
    namespace Renderer  {
        namespace Shape  {

            /**
             * @brief Width of a shape object's line, in map units: its line_width property (Tiled stores it as
             * an int or a float), or one map unit when the object has none. A group's property is not consulted,
             * because libtmx 1.10 does not keep an object group's properties.
             * @param pObject The object, as libtmx parsed it;
             * @return Width in map units;
             */
            double LineWidthOf( tmx_object* pObject );

            /**
             * @brief A colour as libtmx stores one (a packed 0xAARRGGBB, e.g. an object group's
             * color or a text object's), as an stColor - the conversion every drawing class
             * shares (S1b of the master plan; TileMapRenderer::IntToColor too).
             * @param nColor The packed colour;
             * @return The colour;
             */
            SunLight :: Base :: stColor ColorOf( uint32_t nColor );

            // The inverse of ColorOf: a colour packed back into libtmx's ARGB uint32_t (I8).
            uint32_t ArgbOf( SunLight :: Base :: stColor color );

            /**
             * @brief Size of a point object, in map units: its point_size property, or one map unit when the object
             * has none. The same reading as LineWidthOf.
             * @param pObject The object, as libtmx parsed it;
             * @return Size in map units;
             */
            double PointSizeOf( tmx_object* pObject );

            /**
             * @brief Outcome of reading an object's own "opacity" property (L2 of the master plan).
             * NONE: the object has no such property - not an error, the caller falls back to its
             * layer's own opacity silently, the same as today. VALID: the object has it, and it is
             * a number in 0 to 1 - the caller uses it in place of the layer's opacity. INVALID: the
             * object has it, but it is not a usable number (wrong type, or outside 0 to 1) - the
             * caller should warn once and fall back to the layer's opacity, not silently.
             */
            enum class OpacityProperty { NONE, VALID, INVALID };

            /**
             * @brief Reads an object's own "opacity" property into fValue, when it has one and it is
             * usable. See OpacityProperty for what each outcome means and what the caller should do.
             * @param pObject The object, as libtmx parsed it;
             * @param fValue Set to the property's own value (0 to 1) only when the return is VALID;
             * @return Which of the three outcomes above;
             */
            OpacityProperty ObjectOpacityOf( tmx_object* pObject, double& fValue );

            /**
             * @brief A point in map units, relative to an object's stored point.
             */
            struct MapPoint  {
                double  fX;
                double  fY;
            };

            /**
             * @brief Rotates a point about its origin, by degrees clockwise on screen (y grows downwards, as Tiled
             * writes rotation). Cosine and sine within a tiny tolerance of zero are taken as zero, so a quarter turn
             * lands on whole pixels.
             * @param point The point to turn;
             * @param fDegrees Clockwise angle in degrees;
             * @return The turned point;
             */
            MapPoint Rotate( MapPoint point, double fDegrees );

            /**
             * @brief Rotation of a shape object, in degrees clockwise (Tiled's rotation attribute). Zero when the object has none.
             * @param pObject The object, as libtmx parsed it;
             * @return Degrees;
             */
            double RotationOf( tmx_object* pObject );
        }
    }
}
#endif /* __SHAPEOBJECTS_H__ */
