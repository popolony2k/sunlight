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

#ifndef __ISHAPESURFACE_H__
#define __ISHAPESURFACE_H__

#include "base/primitives.h"
#include "base/color.h"


namespace SunLight {
    namespace DrawSurface  {

        /**
         * @brief Viewport/zoom-aware shape drawing - the thick-line, polygon, ellipse and point
         * primitives Phase C/D built, independent of whether any tile map is currently loaded,
         * the same way @ref IDrawSurface already is for its own, simpler primitives. Deliberately
         * NOT merged into IDrawSurface: every primitive there is explicit that it is screen space,
         * with no viewport/camera transform at all (a HUD layer, position measured against the
         * fixed render resolution). These are the opposite - every call maps its own (x, y) the
         * same way a Tiled object's position already does, (position + camera offset) x zoom +
         * the active viewport's own origin, so a shape drawn here pans and scales with the active
         * view exactly like a map's own tiles and objects do (camera offset is simply 0 when no
         * map is loaded, the same zero-initialized state TileMapRenderer already carries from
         * construction - never set by anything except per-layer map drawing).
         *
         * Each primitive here is the exact same underlying drawing code the Tiled-object path
         * (DrawObjects) already uses and already has test coverage for - this interface exists
         * only to give it a public, mapless door, not to reimplement it. Thickness and point size
         * are given in MAP units (the same unit the TMX `line_width`/`point_size` properties
         * already use), converted to screen pixels internally (Shape::ScreenLineWidth) - a caller
         * never computes that conversion itself.
         */
        class IShapeSurface {

            public:

            virtual ~IShapeSurface( void ) {};

            /**
             * @brief Draw a rectangle outline. A thickness of 1 map unit or less draws a plain
             * hairline (the engine's own line primitive, at the current zoom); anything wider is
             * a closed, round-jointed stroked path (see DrawPolygon).
             * @param fX X of the rectangle's own top-left corner, before rotation, in map units;
             * @param fY Y of the top-left corner;
             * @param fWidth Rectangle width, in map units;
             * @param fHeight Rectangle height, in map units;
             * @param color Outline colour;
             * @param fThickness Outline thickness, in map units (converted to screen pixels via
             * the current zoom) - 1.0 (the default) is a hairline;
             * @param fRotation Clockwise rotation in degrees, about (fX, fY) - 0.0 (the default)
             * is unrotated;
             */
            virtual void DrawRectangle( double fX,
                                        double fY,
                                        double fWidth,
                                        double fHeight,
                                        SunLight :: Base :: stColor color,
                                        double fThickness = 1.0,
                                        double fRotation = 0.0 ) = 0;

            /**
             * @brief Draw an ellipse outline, inscribed in the box (fX, fY, fWidth, fHeight). A
             * thickness of 1 map unit or less at zero rotation draws the engine's own ellipse
             * outline primitive; anything wider, or any non-zero rotation, draws a ring of spans
             * between an outer and inner ellipse, tested in the ellipse's own (rotated) frame.
             * @param fX X of the box's own top-left corner, before rotation, in map units;
             * @param fY Y of the top-left corner;
             * @param fWidth Box width, in map units;
             * @param fHeight Box height, in map units;
             * @param color Outline colour;
             * @param fThickness Outline thickness, in map units - 1.0 (the default) is a hairline;
             * @param fRotation Clockwise rotation in degrees, about the box's own centre - 0.0
             * (the default) is unrotated;
             */
            virtual void DrawEllipse( double fX,
                                      double fY,
                                      double fWidth,
                                      double fHeight,
                                      SunLight :: Base :: stColor color,
                                      double fThickness = 1.0,
                                      double fRotation = 0.0 ) = 0;

            /**
             * @brief Draw an open path through a series of points, relative to (fX, fY). A
             * thickness of 1 map unit or less draws plain hairline segments between consecutive
             * points; anything wider is a stroked path with a round join at every interior vertex
             * and a square cap at each of its two open ends.
             * @param fX X origin every point is relative to, in map units;
             * @param fY Y origin every point is relative to;
             * @param pPoints The path's own points, relative to (fX, fY), in map units;
             * @param nPointsCount Number of entries in pPoints;
             * @param color Line colour;
             * @param fThickness Line thickness, in map units - 1.0 (the default) is a hairline;
             * @param fRotation Clockwise rotation in degrees, about (fX, fY) - 0.0 (the default)
             * is unrotated;
             */
            virtual void DrawPolyline( double fX,
                                       double fY,
                                       const SunLight :: Base :: stVector2D *pPoints,
                                       int nPointsCount,
                                       SunLight :: Base :: stColor color,
                                       double fThickness = 1.0,
                                       double fRotation = 0.0 ) = 0;

            /**
             * @brief Draw a closed path through a series of points, relative to (fX, fY) - the
             * same as DrawPolyline, but with a closing edge back to the first point and a round
             * join there too (no open ends, so no caps).
             * @param fX X origin every point is relative to, in map units;
             * @param fY Y origin every point is relative to;
             * @param pPoints The path's own points, relative to (fX, fY), in map units;
             * @param nPointsCount Number of entries in pPoints;
             * @param color Line colour;
             * @param fThickness Line thickness, in map units - 1.0 (the default) is a hairline;
             * @param fRotation Clockwise rotation in degrees, about (fX, fY) - 0.0 (the default)
             * is unrotated;
             */
            virtual void DrawPolygon( double fX,
                                      double fY,
                                      const SunLight :: Base :: stVector2D *pPoints,
                                      int nPointsCount,
                                      SunLight :: Base :: stColor color,
                                      double fThickness = 1.0,
                                      double fRotation = 0.0 ) = 0;

            /**
             * @brief Draw a point as a filled square, its side the size times the current zoom
             * (rounded half-up, at least one pixel), top-left corner at (fX, fY).
             * @param fX X of the point, in map units;
             * @param fY Y of the point;
             * @param color Fill colour;
             * @param fSize Point size, in map units - 1.0 (the default) is one map unit;
             */
            virtual void DrawPoint( double fX,
                                    double fY,
                                    SunLight :: Base :: stColor color,
                                    double fSize = 1.0 ) = 0;
        };
    }
}

#endif  /* __ISHAPESURFACE_H__ */
