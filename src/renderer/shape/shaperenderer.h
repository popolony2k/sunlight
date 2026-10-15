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

#ifndef __SHAPERENDERER_H__
#define __SHAPERENDERER_H__

#include "base/color.h"
#include "renderer/drawcontext.h"
#include "renderer/shape/shapeprimitives.h"
#include <vector>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Draws the shapes Tiled can put on an object layer - polyline, polygon, rectangle,
         * ellipse and point - given in map units, through a DrawContext (S1a of the master plan).
         * It holds no state and knows nothing about TileMapRenderer, so any renderer can use it:
         * TileMapRenderer draws its shape objects and its IShapeSurface methods through one.
         *
         * Each shape is drawn inside a Shape::PrimitiveClip of the context's viewport, so it never
         * spills out of the view. A line width is in screen pixels (Shape::ScreenLineWidth turns a
         * width in map units into one); a width of one pixel is a hairline, drawn edge by edge with
         * the engine's own line, anything wider is a stroked path. Rotations are in degrees,
         * clockwise, about the shape's stored point - Tiled's rotation. Point arrays are the
         * double** libtmx keeps (tmx_shape::points), each entry an { x, y } pair relative to the
         * stored point.
         */
        class ShapeRenderer  {

            // The vertices of a shape in screen pixels: each point turned and scaled by the zoom,
            // from an origin already in screen pixels.
            std :: vector<Shape :: ScreenPoint> ScreenPointsOf( double fOriginX,
                                                                double fOriginY,
                                                                double** fPoints,
                                                                int nPointsCount,
                                                                double fZoom,
                                                                double fRotation ) const;

            // One straight edge of a shape: the engine's own line (IEngine::DrawLine).
            void DrawEdge( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color ) const;

            public:

            void DrawPolyline( const DrawContext& context,
                               double fOffset_x,
                               double fOffset_y,
                               double** fPoints,
                               int nPointsCount,
                               double fRotation,
                               int nLineWidth,
                               SunLight :: Base :: stColor color ) const;
            void DrawPolygon( const DrawContext& context,
                              double fOffset_x,
                              double fOffset_y,
                              double** fPoints,
                              int nPointsCount,
                              double fRotation,
                              int nLineWidth,
                              SunLight :: Base :: stColor color ) const;
            void DrawRectangle( const DrawContext& context,
                                double fOffset_x,
                                double fOffset_y,
                                double fWidth,
                                double fHeight,
                                double fRotation,
                                int nLineWidth,
                                SunLight :: Base :: stColor color ) const;
            void DrawEllipse( const DrawContext& context,
                              double fOffset_x,
                              double fOffset_y,
                              double fWidth,
                              double fHeight,
                              double fRotation,
                              int nLineWidth,
                              SunLight :: Base :: stColor color ) const;
            void DrawPoint( const DrawContext& context,
                            double fOffset_x,
                            double fOffset_y,
                            double fSize,
                            SunLight :: Base :: stColor color ) const;
        };
    }
}

#endif /* __SHAPERENDERER_H__ */
