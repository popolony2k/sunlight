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

#ifndef __SHAPEPRIMITIVES_H__
#define __SHAPEPRIMITIVES_H__

#include "base/color.h"
#include "base/color.h"
#include "base/primitives.h"

#include <vector>

namespace SunLight  {
    namespace Renderer  {
        namespace ShapePrimitives  {

            /**
             * @brief Clips the drawing of one shape to a viewport rectangle, for as long as it lives.
             * The clip sits one pixel inside the rectangle's top and left edges, so the boundary is
             * strict: a pixel on those edges is not drawn, and the far edge (pos + size) is the first
             * pixel outside. It nests in the clip already in effect, so it can only narrow it.
             */
            class PrimitiveClip  {

                public:

                explicit PrimitiveClip( const SunLight :: Base :: stDimension2D &vp );
                ~PrimitiveClip( void );
            };

            /**
             * @brief Screen width of a line: its width in map units times the zoom, rounded half-up, never
             * less than one pixel. The rounding is done once, on the product, so a width of 3 at zoom 2 is 6
             * pixels, and at zoom 1.5 it is 5 (4.5 rounds up).
             * @param fWidth Width in map units;
             * @param fZoom Zoom factor of the viewport;
             * @return Width in screen pixels, at least 1;
             */
            int ScreenLineWidth( double fWidth, double fZoom );

            /**
             * @brief Draw one straight line between two screen points, in the given width.
             * A width of one is the engine's own line (IEngine::DrawLine). A wider line is drawn as spans:
             * one filled row per scanline, each the run of pixels whose centres lie inside the stroke. The
             * stroke has square caps, and its axis runs through the centres of the two end pixels. The
             * spans are cut by the engine's clip, so the caller must hold a PrimitiveClip around the shape.
             * @param nX0 X of the first end (screen pixels);
             * @param nY0 Y of the first end;
             * @param nX1 X of the second end;
             * @param nY1 Y of the second end;
             * @param nWidth Width in screen pixels;
             * @param color Line color;
             */
            void DrawStrokedLine( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color );

            /**
             * @brief A vertex of a path, in screen pixels.
             */
            struct ScreenPoint  {
                int  nX;
                int  nY;
            };

            /**
             * @brief Draw a path of straight segments in the given width. A width of one is left to the
             * caller (one DrawStrokedLine per segment, so each is the engine's line). A wider path is drawn
             * as spans, like DrawStrokedLine, with a round join - a disc of half the width - at every vertex
             * that has two segments, so a corner has no gap. A closed path joins its last vertex back to its
             * first. A closed path needs more than two points; with two, a closed path is one segment.
             * @param points The vertices, in order;
             * @param bClosed Whether the last vertex joins back to the first;
             * @param nWidth Width in screen pixels, at least 2;
             * @param color Line color;
             */
            void DrawStrokedPath( const std :: vector<ScreenPoint> &points, bool bClosed, int nWidth, SunLight :: Base :: stColor color );

            /**
             * @brief Draw an ellipse outline of the given width as a ring: the pixels inside the outer ellipse (the
             * radius plus half the width) and not inside the inner one (the radius minus half the width). When the
             * inner radius is not positive the ring is a filled ellipse. Drawn as spans, cut by the engine's clip.
             * @param fCenterX Centre X (screen pixels);
             * @param fCenterY Centre Y;
             * @param fRadiusX Radius X of the outline's centre line (screen pixels);
             * @param fRadiusY Radius Y;
             * @param nWidth Width in screen pixels, at least 2;
             * @param color Outline color;
             */
            void DrawStrokedEllipse( double fCenterX, double fCenterY, double fRadiusX, double fRadiusY, int nWidth, SunLight :: Base :: stColor color );
        }
    }
}
#endif  /* __SHAPEPRIMITIVES_H__ */
