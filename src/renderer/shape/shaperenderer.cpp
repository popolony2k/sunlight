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

#include "renderer/shape/shaperenderer.h"
#include "renderer/shape/shapeobjects.h"
#include "engines/enginefactory.h"


// A line this wide or thinner is a hairline: its edges are the engine's own one-pixel lines (and an
// unturned ellipse is the engine's own outline); anything wider is a stroked path.
#define __HAIRLINE_WIDTH              1


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Draw one straight edge of a shape, in screen pixels (see Shape::DrawStrokedLine). The
         * shape's PrimitiveClip keeps it inside the viewport.
         * @param nWidth Width in screen pixels;
         */
        void ShapeRenderer :: DrawEdge( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color ) const  {
            Shape :: DrawStrokedLine( nX0, nY0, nX1, nY1, nWidth, color );
        }

        /**
         * The vertices of a shape in screen pixels: each point scaled by the zoom, from an origin already in
         * screen pixels. Converted to whole pixels the same way the hairline edges are.
         */
        std :: vector<Shape :: ScreenPoint> ShapeRenderer :: ScreenPointsOf( double fOriginX,
                                                                             double fOriginY,
                                                                             double **fPoints,
                                                                             int nPointsCount,
                                                                             double fZoom,
                                                                             double fRotation ) const  {
            std :: vector<Shape :: ScreenPoint>  points;

            // Each point turns about the shape's stored point in map units, before the zoom is applied.
            for( int i = 0; i < nPointsCount; i++ ) {
                Shape :: MapPoint  turned = Shape :: Rotate( Shape :: MapPoint { fPoints[i][0], fPoints[i][1] }, fRotation );

                points.push_back( Shape :: ScreenPoint { ( int ) ( fOriginX + ( turned.fX * fZoom ) ),
                                                         ( int ) ( fOriginY + ( turned.fY * fZoom ) ) } );
            }

            return points;
        }

        /**
         * Draw an open polyline (the last point is not joined back to the first).
         * @param context The view and layer to draw for;
         * @param fOffset_x X of the shape's stored point, in map units;
         * @param fOffset_y Y of the shape's stored point;
         * @param fPoints The points, each { x, y } relative to the stored point, in map units;
         * @param nPointsCount Number of points;
         * @param fRotation Clockwise rotation in degrees, about the stored point (Tiled's rotation);
         * @param nLineWidth Line width in screen pixels (already scaled by the zoom);
         * @param color The colour;
         */
        void ShapeRenderer :: DrawPolyline( const DrawContext &context,
                                            double fOffset_x,
                                            double fOffset_y,
                                            double **fPoints,
                                            int nPointsCount,
                                            double fRotation,
                                            int nLineWidth,
                                            SunLight :: Base :: stColor color ) const  {
            Shape :: PrimitiveClip  primitiveClip( context.viewport );

            fOffset_x = ( ( fOffset_x + context.camera.x ) * context.fZoom ) + context.viewport.pos.x;
            fOffset_y = ( ( fOffset_y + context.camera.y ) * context.fZoom ) + context.viewport.pos.y;

            std :: vector<Shape :: ScreenPoint>  points = ScreenPointsOf( fOffset_x, fOffset_y, fPoints, nPointsCount, context.fZoom, fRotation );

            if( nLineWidth > __HAIRLINE_WIDTH ) {
                Shape :: DrawStrokedPath( points, false, nLineWidth, color );
                return;
            }

            for( int i=1; i < nPointsCount; i++ ) {

                DrawEdge( points[i-1].nX, points[i-1].nY, points[i].nX, points[i].nY, nLineWidth, color );
            }
        }

        /**
         * Draw a closed polygon outline (the last point is joined back to the first).
         * @param context The view and layer to draw for;
         * @param fOffset_x X of the shape's stored point, in map units;
         * @param fOffset_y Y of the shape's stored point;
         * @param fPoints The points, each { x, y } relative to the stored point, in map units;
         * @param nPointsCount Number of points;
         * @param fRotation Clockwise rotation in degrees, about the stored point (Tiled's rotation);
         * @param nLineWidth Line width in screen pixels (already scaled by the zoom);
         * @param color The colour;
         */
        void ShapeRenderer :: DrawPolygon( const DrawContext &context,
                                           double fOffset_x,
                                           double fOffset_y,
                                           double **fPoints,
                                           int nPointsCount,
                                           double fRotation,
                                           int nLineWidth,
                                           SunLight :: Base :: stColor color ) const  {
            Shape :: PrimitiveClip  primitiveClip( context.viewport );

            if( nLineWidth > __HAIRLINE_WIDTH ) {
                Shape :: DrawStrokedPath( ScreenPointsOf( ( ( fOffset_x + context.camera.x ) * context.fZoom ) + context.viewport.pos.x,
                                                          ( ( fOffset_y + context.camera.y ) * context.fZoom ) + context.viewport.pos.y,
                                                          fPoints, nPointsCount, context.fZoom, fRotation ),
                                          true, nLineWidth, color );
                return;
            }

            DrawPolyline( context,
                          fOffset_x,
                          fOffset_y,
                          fPoints,
                          nPointsCount,
                          fRotation,
                          __HAIRLINE_WIDTH,
                          color );

            if( nPointsCount > 2 ) {

                fOffset_x = ( ( fOffset_x + context.camera.x ) * context.fZoom ) + context.viewport.pos.x;
                fOffset_y = ( ( fOffset_y + context.camera.y ) * context.fZoom ) + context.viewport.pos.y;

                std :: vector<Shape :: ScreenPoint>  points = ScreenPointsOf( fOffset_x, fOffset_y, fPoints, nPointsCount, context.fZoom, fRotation );

                DrawEdge( points[0].nX, points[0].nY, points[nPointsCount-1].nX, points[nPointsCount-1].nY, __HAIRLINE_WIDTH, color );
            }
        }

        /**
         * Draw a rectangle outline.
         * @param context The view and layer to draw for;
         * @param fOffset_x X of the top-left corner (the stored point), in map units;
         * @param fOffset_y Y of the top-left corner;
         * @param fWidth The width, in map units;
         * @param fHeight The height, in map units;
         * @param fRotation Clockwise rotation in degrees, about the stored point (Tiled's rotation);
         * @param nLineWidth Line width in screen pixels (already scaled by the zoom);
         * @param color The colour;
         */
        void ShapeRenderer :: DrawRectangle( const DrawContext &context,
                                             double fOffset_x,
                                             double fOffset_y,
                                             double fWidth,
                                             double fHeight,
                                             double fRotation,
                                             int nLineWidth,
                                             SunLight :: Base :: stColor color ) const  {
            Shape :: PrimitiveClip  primitiveClip( context.viewport );

            // The corners, relative to the stored point (the top-left corner), turned about it in map units.
            Shape :: MapPoint  local[] = { { 0.0, 0.0 }, { fWidth, 0.0 }, { fWidth, fHeight }, { 0.0, fHeight } };
            std :: vector<Shape :: ScreenPoint>  corners;

            for( const Shape :: MapPoint &point : local )  {
                Shape :: MapPoint  turned = Shape :: Rotate( point, fRotation );

                corners.push_back( Shape :: ScreenPoint { ( int ) ( ( ( fOffset_x + turned.fX + context.camera.x ) * context.fZoom ) + context.viewport.pos.x ),
                                                          ( int ) ( ( ( fOffset_y + turned.fY + context.camera.y ) * context.fZoom ) + context.viewport.pos.y ) } );
            }

            // A wide edge is the closed path of the four corners, so each corner is a round join.
            if( nLineWidth > __HAIRLINE_WIDTH ) {
                Shape :: DrawStrokedPath( corners, true, nLineWidth, color );
                return;
            }

            // The four edges, as before: top, bottom, left and right, in the order of the corners.
            DrawEdge( corners[0].nX, corners[0].nY, corners[1].nX, corners[1].nY, __HAIRLINE_WIDTH, color );
            DrawEdge( corners[3].nX, corners[3].nY, corners[2].nX, corners[2].nY, __HAIRLINE_WIDTH, color );
            DrawEdge( corners[0].nX, corners[0].nY, corners[3].nX, corners[3].nY, __HAIRLINE_WIDTH, color );
            DrawEdge( corners[1].nX, corners[1].nY, corners[2].nX, corners[2].nY, __HAIRLINE_WIDTH, color );
        }

        /**
         * Draw a point object as a filled square. Its side is the point's size times the zoom, rounded half-up and
         * at least one pixel (the same rule as a line's width), with the top-left corner at the point's position.
         * @param context The view and layer to draw for;
         * @param fOffset_x X of the point, in map units;
         * @param fOffset_y Y of the point;
         * @param fSize Size of the point, in map units;
         * @param color The colour;
         */
        void ShapeRenderer :: DrawPoint( const DrawContext &context,
                                         double fOffset_x,
                                         double fOffset_y,
                                         double fSize,
                                         SunLight :: Base :: stColor color ) const  {
            Shape :: PrimitiveClip  primitiveClip( context.viewport );

            double  fScreenX = ( ( fOffset_x + context.camera.x ) * context.fZoom ) + context.viewport.pos.x;
            double  fScreenY = ( ( fOffset_y + context.camera.y ) * context.fZoom ) + context.viewport.pos.y;

            Shape :: DrawFilledSquare( ( int ) fScreenX, ( int ) fScreenY, Shape :: ScreenLineWidth( fSize, context.fZoom ), color );
        }

        /**
         * Draw an ellipse outline, inscribed in its box.
         * @param context The view and layer to draw for;
         * @param fOffset_x X of the box's top-left corner (the stored point), in map units;
         * @param fOffset_y Y of the box's top-left corner;
         * @param fWidth The box's width, in map units;
         * @param fHeight The box's height, in map units;
         * @param fRotation Clockwise rotation in degrees, about the stored point (Tiled's rotation);
         * @param nLineWidth Line width in screen pixels (already scaled by the zoom);
         * @param color The colour;
         */
        void ShapeRenderer :: DrawEllipse( const DrawContext &context,
                                           double fOffset_x,
                                           double fOffset_y,
                                           double fWidth,
                                           double fHeight,
                                           double fRotation,
                                           int nLineWidth,
                                           SunLight :: Base :: stColor color ) const  {
            Shape :: PrimitiveClip  primitiveClip( context.viewport );

            // The centre of the box, relative to its stored point (the top-left corner), turned about that point in map units.
            Shape :: MapPoint  centre = Shape :: Rotate( Shape :: MapPoint { fWidth / 2.0, fHeight / 2.0 }, fRotation );

            double  fCenterX = ( ( fOffset_x + centre.fX + context.camera.x ) * context.fZoom ) + context.viewport.pos.x;
            double  fCenterY = ( ( fOffset_y + centre.fY + context.camera.y ) * context.fZoom ) + context.viewport.pos.y;
            double  fRadiusX = ( fWidth / 2.0 ) * context.fZoom;
            double  fRadiusY = ( fHeight / 2.0 ) * context.fZoom;

            // An unturned one-pixel outline is the engine's own ellipse. Anything wider, or turned, is a ring of spans;
            // the shape's PrimitiveClip keeps either inside the viewport.
            if( ( fRotation == 0.0 ) && ( nLineWidth <= __HAIRLINE_WIDTH ) )  {
                SunLight :: Engines :: EngineFactory :: GetEngine().DrawEllipseOutline( ( float ) fCenterX, ( float ) fCenterY,
                                                                                        ( float ) fRadiusX, ( float ) fRadiusY, color );
                return;
            }

            Shape :: DrawStrokedEllipse( fCenterX, fCenterY, fRadiusX, fRadiusY, fRotation, nLineWidth, color );
        }
    }
}
