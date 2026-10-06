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

#include "renderer/shapeprimitives.h"
#include "engines/enginefactory.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace SunLight  {
    namespace Renderer  {
        namespace ShapePrimitives  {

            namespace  {

                // The clip is moved in one pixel from the viewport's top and left edges (see PrimitiveClip).
                const int     __PRIMITIVE_EDGE_INSET   = 1;
                const int     __ONE_PIXEL              = 1;
                const int     __CAP_DIVISOR            = 2;       // the cap of a stroke is (width - 1) / 2 pixels
                const int     __MIN_SCREEN_WIDTH       = 1;
                // Up to this width a line is the engine's own line; wider lines are drawn as spans.
                const int     __ENGINE_LINE_MAX_WIDTH  = 1;
                const float   __EDGE_LINE_THICKNESS    = 1.0f;    // the engine's line is one pixel thick
                const int     __ROW_MARGIN             = 1;       // rows either side of the stroke's rows that are tested
                const int     __COLUMN_MARGIN          = 1;       // columns either side of a row's chord that are tested
                const int     __SPAN_HEIGHT            = 1;       // a span is one pixel tall
                const double  __HALF                   = 0.5;     // half a pixel: a pixel's centre, a half width, and half-up rounding
                const double  __ORIGIN                 = 0.0;
                const double  __EMPTY_RANGE_LOW        = 1.0;     // an empty [low, high] is marked with low > high
                const double  __EMPTY_RANGE_HIGH       = 0.0;
                const double  __AXIS_EPSILON           = 1e-12;   // a direction component this small counts as zero
                // A pixel centre this close to a stroke edge is ON the edge, so the half-open rule decides it. Without
                // this, a centre that is exactly on an edge in exact arithmetic lands a rounding error inside, and
                // comes out either way.
                const double  __BOUNDARY_EPSILON       = 1e-9;

                /**
                 * The stroke of one thick line, as geometry in screen pixels. The axis runs from the centre of the first
                 * end pixel towards the second; the stroke is the rectangle around it, of half width h, extended at each
                 * end by a square cap of e pixels.
                 */
                struct StrokeGeometry  {
                    double  ax, ay;     // centre of the first end pixel
                    double  ux, uy;     // unit vector along the line
                    double  nx, ny;     // unit vector across the line
                    double  length;     // distance between the two end centres
                    double  h;          // half the width
                    double  e;          // the cap: (width - 1) / 2 pixels
                };

                /**
                 * Whether the centre of one pixel is inside the stroke. The test is half-open on all four sides - the
                 * across-axis edges include -h but not +h, the cap edges include -e but not length + e - so every width
                 * gives exactly that many rows or columns, and no pixel centre sits on a tie that could go either way.
                 */
                bool InsideStroke( const StrokeGeometry &g, double cx, double cy )  {

                    double  across = g.nx * ( cx - g.ax ) + g.ny * ( cy - g.ay );
                    double  along  = g.ux * ( cx - g.ax ) + g.uy * ( cy - g.ay );

                    return ( across >= -g.h - __BOUNDARY_EPSILON ) && ( across < g.h - __BOUNDARY_EPSILON ) &&
                           ( along >= -g.e - __BOUNDARY_EPSILON ) && ( along < g.length + g.e - __BOUNDARY_EPSILON );
                }

                // Narrows [xLow, xHigh] to the t for which c * t lies in [lo, hi). An empty result is left as xLow > xHigh.
                void NarrowRange( double c, double lo, double hi, double &xLow, double &xHigh )  {

                    if( std :: fabs( c ) < __AXIS_EPSILON )  {
                        if( !( ( lo <= __ORIGIN ) && ( __ORIGIN < hi ) ) )  {
                            xLow  = __EMPTY_RANGE_LOW;
                            xHigh = __EMPTY_RANGE_HIGH;
                        }
                        return;
                    }

                    double  a = lo / c;
                    double  b = hi / c;

                    if( c < __ORIGIN )
                        std :: swap( a, b );

                    xLow  = std :: max( xLow, a );
                    xHigh = std :: min( xHigh, b );
                }

                /**
                 * Draw one thick line as spans: one filled row per scanline, each the run of pixels whose centres are
                 * inside the stroke. For each row the chord is found by solving the two edge pairs, then the pixels
                 * either side of it are tested one by one with InsideStroke - so the spans are exactly the pixels
                 * InsideStroke accepts. The engine's clip cuts the spans at the viewport, so nothing is clipped here.
                 */
                void DrawSpans( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color )  {

                    StrokeGeometry  g;
                    double          dx = ( double ) ( nX1 - nX0 );
                    double          dy = ( double ) ( nY1 - nY0 );

                    g.length = std :: sqrt( dx * dx + dy * dy );

                    if( g.length == __ORIGIN )
                        return;

                    g.ax = nX0 + __HALF;
                    g.ay = nY0 + __HALF;
                    g.ux = dx / g.length;
                    g.uy = dy / g.length;
                    g.nx = -g.uy;
                    g.ny = g.ux;
                    g.h  = nWidth * __HALF;
                    g.e  = ( double ) ( ( nWidth - __ONE_PIXEL ) / __CAP_DIVISOR );

                    // The rows the stroke can touch: the y of its four corners.
                    double  fMinY = __ORIGIN, fMaxY = __ORIGIN;
                    bool    bFirst = true;

                    for( double fAlong : { -g.e, g.length + g.e } )  {
                        for( double fAcross : { -g.h, g.h } )  {
                            double  fY = g.ay + g.uy * fAlong + g.ny * fAcross;

                            if( bFirst || ( fY < fMinY ) )  fMinY = fY;
                            if( bFirst || ( fY > fMaxY ) )  fMaxY = fY;
                            bFirst = false;
                        }
                    }

                    int  nFirstRow = ( int ) std :: floor( fMinY - __HALF ) - __ROW_MARGIN;
                    int  nLastRow  = ( int ) std :: ceil( fMaxY - __HALF ) + __ROW_MARGIN;

                    for( int nRow = nFirstRow; nRow <= nLastRow; nRow++ )  {

                        double  cy    = nRow + __HALF;
                        double  fLow  = -std :: numeric_limits<double> :: max();
                        double  fHigh =  std :: numeric_limits<double> :: max();

                        // Relative to ax: the across condition, then the along condition, each as c * t in [lo, hi).
                        double  kAcross = g.ny * ( cy - g.ay );
                        double  kAlong  = g.uy * ( cy - g.ay );

                        NarrowRange( g.nx, -g.h - kAcross, g.h - kAcross, fLow, fHigh );
                        NarrowRange( g.ux, -g.e - kAlong, g.length + g.e - kAlong, fLow, fHigh );

                        if( fLow > fHigh )
                            continue;

                        int  nFirst = ( int ) std :: floor( g.ax + fLow - __HALF ) - __COLUMN_MARGIN;
                        int  nLast  = ( int ) std :: ceil( g.ax + fHigh - __HALF ) + __COLUMN_MARGIN;
                        int  nLeft  = nLast + __ONE_PIXEL;
                        int  nRight = nFirst - __ONE_PIXEL;

                        for( int nX = nFirst; nX <= nLast; nX++ )  {
                            if( InsideStroke( g, nX + __HALF, cy ) )  {
                                if( nX < nLeft )   nLeft  = nX;
                                if( nX > nRight )  nRight = nX;
                            }
                        }

                        if( nLeft <= nRight )
                            SunLight :: Engines :: EngineFactory :: GetEngine().DrawFilledRectangle( nLeft, nRow, nRight - nLeft + __ONE_PIXEL,
                                                                                                    __SPAN_HEIGHT, color );
                    }
                }
            }

            PrimitiveClip :: PrimitiveClip( const SunLight :: Base :: stDimension2D &vp )  {
                SunLight :: Engines :: EngineFactory :: GetEngine().BeginClip( SunLight :: Base :: stRectangle {
                    ( float ) ( vp.pos.x + __PRIMITIVE_EDGE_INSET ), ( float ) ( vp.pos.y + __PRIMITIVE_EDGE_INSET ),
                    ( float ) ( vp.size.nWidth - __PRIMITIVE_EDGE_INSET ), ( float ) ( vp.size.nHeight - __PRIMITIVE_EDGE_INSET ) } );
            }

            PrimitiveClip :: ~PrimitiveClip( void )  {
                SunLight :: Engines :: EngineFactory :: GetEngine().EndClip();
            }

            int ScreenLineWidth( double fWidth, double fZoom )  {

                int  nWidth = ( int ) std :: floor( fWidth * fZoom + __HALF );

                return ( nWidth < __MIN_SCREEN_WIDTH ) ? __MIN_SCREEN_WIDTH : nWidth;
            }

            void DrawStrokedLine( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color )  {

                if( nWidth > __ENGINE_LINE_MAX_WIDTH )  {
                    DrawSpans( nX0, nY0, nX1, nY1, nWidth, color );
                    return;
                }

                SunLight :: Engines :: EngineFactory :: GetEngine().DrawLine( ( float ) nX0, ( float ) nY0,
                                                                              ( float ) nX1, ( float ) nY1,
                                                                              __EDGE_LINE_THICKNESS, color );
            }
        }
    }
}
