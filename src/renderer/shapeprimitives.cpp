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
#include <vector>

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
                const int     __MIN_JOIN_VERTICES      = 3;       // a closed path needs at least this many vertices

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
                    double  eBack;      // the cap at the start: (width - 1) / 2 pixels, or none at a join
                    double  eFront;     // the cap at the end, likewise
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
                           ( along >= -g.eBack - __BOUNDARY_EPSILON ) && ( along < g.length + g.eFront - __BOUNDARY_EPSILON );
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

                // The stroke of one segment of a path, with the segment's endpoints as end centres.
                bool MakeStroke( const ScreenPoint &a, const ScreenPoint &b, double h, double eBack, double eFront, StrokeGeometry &g )  {

                    double  dx = ( double ) ( b.nX - a.nX );
                    double  dy = ( double ) ( b.nY - a.nY );

                    g.length = std :: sqrt( dx * dx + dy * dy );

                    if( g.length == __ORIGIN )
                        return false;

                    g.ax = a.nX + __HALF;
                    g.ay = a.nY + __HALF;
                    g.ux = dx / g.length;
                    g.uy = dy / g.length;
                    g.nx = -g.uy;
                    g.ny = g.ux;
                    g.h  = h;
                    g.eBack  = eBack;
                    g.eFront = eFront;

                    return true;
                }

                // A vertex centre where two segments meet: the round join is a disc of half the width around it.
                struct Joint  {
                    double  vx, vy;
                };

                // A pixel centre is in the join when it is inside the disc (strictly, with the boundary tolerance).
                bool InsideJoint( const Joint &j, double h, double cx, double cy )  {

                    double  r  = h - __BOUNDARY_EPSILON;
                    double  dx = cx - j.vx;
                    double  dy = cy - j.vy;

                    return ( dx * dx + dy * dy ) < ( r * r );
                }

                // A run of pixels on one row, from nFirst to nLast inclusive.
                struct Run  {
                    int  nFirst;
                    int  nLast;
                };

                /**
                 * Draw a path as spans: one filled row per scanline. On each row the pixels of every segment and
                 * every join are found, the runs are merged where they touch or overlap, and each merged run is one
                 * DrawFilledRectangle. The spans are exactly the pixels that some segment or join accepts. The
                 * engine's clip cuts them at the viewport, so nothing is clipped here.
                 */
                void DrawPathSpans( const std :: vector<ScreenPoint> &points, bool bClosed, int nWidth, SunLight :: Base :: stColor color )  {

                    size_t  nCount = points.size();
                    bool    bLoop  = bClosed && ( nCount >= __MIN_JOIN_VERTICES );

                    double  h = nWidth * __HALF;
                    double  e = ( double ) ( ( nWidth - __ONE_PIXEL ) / __CAP_DIVISOR );

                    std :: vector<StrokeGeometry>  segments;
                    std :: vector<Joint>           joints;

                    size_t  nSegmentCount = bLoop ? nCount : nCount - 1;

                    // A segment is capped only at the two ends of an open path. At a join the round join is the
                    // corner, so a cap there would cut a flat step into its arc.
                    for( size_t nIdx = 0; nIdx < nSegmentCount; nIdx++ )  {
                        StrokeGeometry  g;
                        const ScreenPoint  &a = points[nIdx];
                        const ScreenPoint  &b = points[ ( nIdx + 1 ) % nCount ];
                        double  eBack  = ( !bLoop && ( nIdx == 0 ) ) ? e : 0.0;
                        double  eFront = ( !bLoop && ( nIdx + 1 == nSegmentCount ) ) ? e : 0.0;

                        if( MakeStroke( a, b, h, eBack, eFront, g ) )
                            segments.push_back( g );
                    }

                    // Every vertex of a loop joins two segments; an open path joins only its interior vertices.
                    size_t  nJointFirst = bLoop ? 0 : 1;
                    size_t  nJointEnd   = bLoop ? nCount : nCount - 1;

                    for( size_t nIdx = nJointFirst; nIdx < nJointEnd; nIdx++ )
                        joints.push_back( Joint { points[nIdx].nX + __HALF, points[nIdx].nY + __HALF } );

                    if( segments.empty() )
                        return;

                    // The rows the path can touch: the corners of every segment, and every joint's disc.
                    double  fMinY = 0.0, fMaxY = 0.0;
                    bool    bFirst = true;

                    auto  Include = [&]( double fY )  {
                        if( bFirst || ( fY < fMinY ) )  fMinY = fY;
                        if( bFirst || ( fY > fMaxY ) )  fMaxY = fY;
                        bFirst = false;
                    };

                    for( const StrokeGeometry &g : segments )  {
                        for( double fAlong : { -g.eBack, g.length + g.eFront } )
                            for( double fAcross : { -g.h, g.h } )
                                Include( g.ay + g.uy * fAlong + g.ny * fAcross );
                    }

                    for( const Joint &j : joints )  {
                        Include( j.vy - h );
                        Include( j.vy + h );
                    }

                    int  nFirstRow = ( int ) std :: floor( fMinY - __HALF ) - __ROW_MARGIN;
                    int  nLastRow  = ( int ) std :: ceil( fMaxY - __HALF ) + __ROW_MARGIN;

                    for( int nRow = nFirstRow; nRow <= nLastRow; nRow++ )  {

                        double  cy = nRow + __HALF;
                        std :: vector<Run>  runs;

                        for( const StrokeGeometry &g : segments )  {
                            double  fLow  = -std :: numeric_limits<double> :: max();
                            double  fHigh =  std :: numeric_limits<double> :: max();

                            // Relative to ax: the across condition, then the along condition, each as c * t in [lo, hi).
                            double  kAcross = g.ny * ( cy - g.ay );
                            double  kAlong  = g.uy * ( cy - g.ay );

                            NarrowRange( g.nx, -g.h - kAcross, g.h - kAcross, fLow, fHigh );
                            NarrowRange( g.ux, -g.eBack - kAlong, g.length + g.eFront - kAlong, fLow, fHigh );

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
                                runs.push_back( Run { nLeft, nRight } );
                        }

                        for( const Joint &j : joints )  {
                            double  dy = cy - j.vy;

                            if( std :: fabs( dy ) >= h )
                                continue;

                            double  half   = std :: sqrt( h * h - dy * dy );
                            int     nFirst = ( int ) std :: floor( j.vx - half - __HALF ) - __COLUMN_MARGIN;
                            int     nLast  = ( int ) std :: ceil( j.vx + half - __HALF ) + __COLUMN_MARGIN;
                            int     nLeft  = nLast + __ONE_PIXEL;
                            int     nRight = nFirst - __ONE_PIXEL;

                            for( int nX = nFirst; nX <= nLast; nX++ )  {
                                if( InsideJoint( j, h, nX + __HALF, cy ) )  {
                                    if( nX < nLeft )   nLeft  = nX;
                                    if( nX > nRight )  nRight = nX;
                                }
                            }

                            if( nLeft <= nRight )
                                runs.push_back( Run { nLeft, nRight } );
                        }

                        std :: sort( runs.begin(), runs.end(), []( const Run &a, const Run &b ) { return a.nFirst < b.nFirst; } );

                        size_t  nIdx = 0;

                        while( nIdx < runs.size() )  {
                            int     nFirst = runs[nIdx].nFirst;
                            int     nLast  = runs[nIdx].nLast;
                            size_t  nNext  = nIdx + 1;

                            while( ( nNext < runs.size() ) && ( runs[nNext].nFirst <= nLast + __ONE_PIXEL ) )  {
                                nLast = std :: max( nLast, runs[nNext].nLast );
                                nNext++;
                            }

                            SunLight :: Engines :: EngineFactory :: GetEngine().DrawFilledRectangle( nFirst, nRow, nLast - nFirst + __ONE_PIXEL,
                                                                                                    __SPAN_HEIGHT, color );
                            nIdx = nNext;
                        }
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

            void DrawStrokedPath( const std :: vector<ScreenPoint> &points, bool bClosed, int nWidth, SunLight :: Base :: stColor color )  {

                if( points.size() < __MIN_JOIN_VERTICES - __ONE_PIXEL )
                    return;

                DrawPathSpans( points, bClosed, nWidth, color );
            }

            void DrawStrokedLine( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color )  {

                if( nWidth > __ENGINE_LINE_MAX_WIDTH )  {
                    DrawPathSpans( { ScreenPoint { nX0, nY0 }, ScreenPoint { nX1, nY1 } }, false, nWidth, color );
                    return;
                }

                SunLight :: Engines :: EngineFactory :: GetEngine().DrawLine( ( float ) nX0, ( float ) nY0,
                                                                              ( float ) nX1, ( float ) nY1,
                                                                              __EDGE_LINE_THICKNESS, color );
            }
        }
    }
}
