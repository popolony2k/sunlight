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

#include <doctest/doctest.h>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // The viewport is at (10, 10) with the camera at (0, 0). A polyline at object (100, 100) starts at
    // screen (110, 110) at zoom 1.
    const int  kViewportOrigin = 10;
    const int  kObjectX        = 100;
    const int  kObjectY        = 100;

    // A one-object map. The object's XML (its properties and shape) is spliced in as given.
    Bytes MakeLineTmx( const std :: string &strObject )  {

        std :: string  str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                             "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
                             " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
                             "<objectgroup id=\"2\" name=\"lines\">" + strObject + "</objectgroup></map>";

        return Bytes( str.begin(), str.end() );
    }

    // The polyline object XML for a line with the given points and line_width (an int, Tiled's int property).
    std :: string PolylineObject( const std :: string &strPoints, int nLineWidth )  {

        return "<object id=\"1\" x=\"" + std :: to_string( kObjectX ) + "\" y=\"" + std :: to_string( kObjectY ) + "\" width=\"0\" height=\"0\">"
               "<properties><property name=\"line_width\" type=\"int\" value=\"" + std :: to_string( nLineWidth ) + "\"/></properties>"
               "<polyline points=\"" + strPoints + "\"/></object>";
    }

    // Renders one frame of a one-object map at the given zoom position (31 is zoom 2, 15 is zoom 1), and keeps the engine's events.
    struct Frame  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Frame( const std :: string &strObject, unsigned nZoomPos = 15u )  {

            fsFixture.fs.files["maps/line.tmx"] = MakeLineTmx( strObject );

            RendererConfig  config;

            config.fWidth  = 1260.0f;
            config.fHeight = 920.0f;

            SunLight :: Base :: stDimension2D  viewport {};

            viewport.pos.x = kViewportOrigin;  viewport.pos.y = kViewportOrigin;  viewport.size.nWidth = 1000;  viewport.size.nHeight = 800;
            config.viewport = viewport;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            pRenderer -> GetViewport().SetZoom( nZoomPos );
            windowFixture.window.nFramesUntilShouldClose = 1;
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/line.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            pRenderer -> Run();
        }

        ~Frame( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        std :: vector<MockEngine :: Event>  Of( MockEngine :: Event :: Kind kind ) const  {
            std :: vector<MockEngine :: Event>  found;

            for( const MockEngine :: Event &evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }

        // Every pixel the filled spans cover.
        std :: set<std :: pair<int, int>>  Covered( void ) const  {
            std :: set<std :: pair<int, int>>  pixels;

            for( const MockEngine :: Event &span : Of( MockEngine :: Event :: FILL ) )
                for( int nX = ( int ) span.x; nX < ( int ) ( span.x + span.w ); nX++ )
                    for( int nY = ( int ) span.y; nY < ( int ) ( span.y + span.h ); nY++ )
                        pixels.insert( std :: make_pair( nX, nY ) );

            return pixels;
        }
    };

    // A row of spans as the test expects it: x, y of its left pixel, and its width. Height is always one.
    struct Span  {
        int nX, nY, nWidth;
    };

    bool SameSpans( const std :: vector<MockEngine :: Event> &found, const std :: vector<Span> &expected )  {

        if( found.size() != expected.size() )
            return false;

        for( size_t nIdx = 0; nIdx < expected.size(); nIdx++ )
            if( ( found[nIdx].x != expected[nIdx].nX ) || ( found[nIdx].y != expected[nIdx].nY ) ||
                ( found[nIdx].w != expected[nIdx].nWidth ) || ( found[nIdx].h != 1.0f ) )
                return false;

        return true;
    }

    // The reference: a pixel is in the stroke when its centre is inside the rectangle around the segment
    // from (x0, y0) to (x1, y1), with the same half-open edges. Computed from the raw endpoints, not from
    // unit vectors, so it shares no arithmetic with the renderer's span code.
    // eBack and eFront are the caps at the segment's two ends: a full cap at a path's end, none at a join.
    bool ReferenceInside( double x0, double y0, double x1, double y1, int nWidth, double eBack, double eFront, double cx, double cy )  {

        double  dx = x1 - x0;
        double  dy = y1 - y0;
        double  L  = std :: sqrt( dx * dx + dy * dy );
        double  h  = nWidth / 2.0;
        double  ax = x0 + 0.5;
        double  ay = y0 + 0.5;
        double  u  = ( ( cx - ax ) * dx + ( cy - ay ) * dy ) / L;
        double  v  = ( ( cx - ax ) * ( -dy ) + ( cy - ay ) * dx ) / L;

        const double  eps = 1e-9;   // the same boundary tolerance the renderer uses: a centre this close to an edge is on it

        return ( v >= -h - eps ) && ( v < h - eps ) && ( u >= -eBack - eps ) && ( u < L + eFront - eps );
    }
}

TEST_SUITE( "Thick lines" )  {

    TEST_CASE( "A width-2 horizontal line is two rows of ten pixels, centred on its axis" )  {

        // Screen (110, 110) to (120, 110). The axis is at y = 110.5, so the stroke is y in [109.5, 111.5): rows 109 and 110.
        // Along the line it is [110.5, 120.5): columns 110 to 119. There are no caps at width 2.
        Frame  frame( PolylineObject( "0,0 10,0", 2 ) );

        CHECK( SameSpans( frame.Of( MockEngine :: Event :: FILL ),
                          { { 110, 109, 10 }, { 110, 110, 10 } } ) );
    }

    TEST_CASE( "A width-3 line gets a one-pixel square cap at each end" )  {

        // Half width 1.5, cap 1. The axis is at y = 110.5: rows 109, 110 and 111 (centres 109.5 to 111.5).
        // Along the line [109.5, 121.5): columns 109 to 120, twelve pixels, which is the ten-pixel line plus a pixel either end.
        Frame  frame( PolylineObject( "0,0 10,0", 3 ) );

        CHECK( SameSpans( frame.Of( MockEngine :: Event :: FILL ),
                          { { 109, 109, 12 }, { 109, 110, 12 }, { 109, 111, 12 } } ) );
    }

    TEST_CASE( "The zoom scales the width: a width-3 line at zoom 2 is six rows" )  {

        // Zoom 2 (position 31): the line runs from screen (210, 210) to (230, 210). The width is 3 x 2 = 6 pixels,
        // half width 3, cap 2. Rows 207 to 212 (six rows), columns 208 to 231 (the line's 20 pixels plus a cap of 2 at each end).
        Frame  frame( PolylineObject( "0,0 10,0", 3 ), 31u );

        REQUIRE( frame.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor == 2.0f );

        CHECK( SameSpans( frame.Of( MockEngine :: Event :: FILL ),
                          { { 208, 207, 24 }, { 208, 208, 24 }, { 208, 209, 24 },
                            { 208, 210, 24 }, { 208, 211, 24 }, { 208, 212, 24 } } ) );
    }

    TEST_CASE( "A fractional zoom rounds the width half-up: width 3 at zoom 1.5 is 4.5 pixels, so 5" )  {

        // Zoom 1.5 (position 23): screen (160, 160) to (175, 160). Width 5 gives half width 2.5 and cap 2:
        // rows 158 to 162 (five rows), columns 158 to 176 (19 pixels: the 15-pixel line plus a cap of 2 each end).
        Frame  frame( PolylineObject( "0,0 10,0", 3 ), 23u );

        REQUIRE( frame.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor == 1.5f );

        CHECK( SameSpans( frame.Of( MockEngine :: Event :: FILL ),
                          { { 158, 158, 19 }, { 158, 159, 19 }, { 158, 160, 19 }, { 158, 161, 19 }, { 158, 162, 19 } } ) );
    }

    TEST_CASE( "A width-1 line is still the engine's own line, not spans" )  {

        Frame  frame( PolylineObject( "0,0 10,0", 1 ) );

        CHECK( frame.Of( MockEngine :: Event :: FILL ).empty() );
        CHECK( frame.Of( MockEngine :: Event :: LINE ).size() == 1 );
    }

    TEST_CASE( "A line without a line_width property is one pixel wide, as before" )  {

        Frame  frame( "<object id=\"1\" x=\"100\" y=\"100\" width=\"0\" height=\"0\"><polyline points=\"0,0 10,0\"/></object>" );

        CHECK( frame.Of( MockEngine :: Event :: FILL ).empty() );
        CHECK( frame.Of( MockEngine :: Event :: LINE ).size() == 1 );
    }

    TEST_CASE( "A thick diagonal line is one span per row, far fewer than its pixels" )  {

        Frame  frame( PolylineObject( "0,0 60,40", 4 ) );

        std :: vector<MockEngine :: Event>  spans = frame.Of( MockEngine :: Event :: FILL );
        std :: set<int>                     rows;
        long                                nPixels = 0;

        for( const MockEngine :: Event &span : spans )  {
            CHECK( span.h == 1.0f );
            rows.insert( ( int ) span.y );
            nPixels += ( long ) span.w;
        }

        CHECK( rows.size() == spans.size() );                 // one span per row, no row twice
        CHECK( spans.size() < ( size_t ) nPixels / 3 );       // each span is several pixels wide on average
    }

    TEST_CASE( "The spans are exactly the pixels whose centre is inside the stroke, for several angles and widths" )  {

        const char * const  aLines[] = { "0,0 60,40", "0,60 60,0", "0,0 0,50", "5,5 45,35" };

        for( const char *szLine : aLines )  {
            for( int nWidth = 2; nWidth <= 5; nWidth++ )  {

                INFO( "line " << szLine << " width " << nWidth );

                Frame  frame( PolylineObject( szLine, nWidth ) );

                // The endpoints, in screen pixels (the object is at (100, 100) and the viewport at (10, 10)).
                double  fPoints[4] = { 0, 0, 0, 0 };
                std :: string  strPoints = szLine;
                int  nPoint = 0;
                size_t  nPos = 0;

                while( nPoint < 4 )  {
                    size_t  nEnd = strPoints.find_first_of( " ,", nPos );
                    fPoints[nPoint++] = std :: stod( strPoints.substr( nPos, nEnd - nPos ) );
                    if( nEnd == std :: string :: npos )  break;
                    nPos = nEnd + 1;
                }

                double  x0 = kViewportOrigin + kObjectX + fPoints[0];
                double  y0 = kViewportOrigin + kObjectY + fPoints[1];
                double  x1 = kViewportOrigin + kObjectX + fPoints[2];
                double  y1 = kViewportOrigin + kObjectY + fPoints[3];

                std :: set<std :: pair<int, int>>  expected;

                for( int nX = -10; nX < 200; nX++ )
                    for( int nY = -10; nY < 200; nY++ )
                        if( ReferenceInside( x0, y0, x1, y1, nWidth, ( nWidth - 1 ) / 2, ( nWidth - 1 ) / 2, nX + 0.5, nY + 0.5 ) )
                            expected.insert( std :: make_pair( nX, nY ) );

                std :: set<std :: pair<int, int>>  covered = frame.Covered();
                std :: vector<std :: pair<int, int>>  missing, extra;

                std :: set_difference( expected.begin(), expected.end(), covered.begin(), covered.end(), std :: back_inserter( missing ) );
                std :: set_difference( covered.begin(), covered.end(), expected.begin(), expected.end(), std :: back_inserter( extra ) );

                INFO( "missing " << missing.size() << " first (" << ( missing.empty() ? -1 : missing[0].first ) << ", "
                      << ( missing.empty() ? -1 : missing[0].second ) << "), extra " << extra.size() << " first ("
                      << ( extra.empty() ? -1 : extra[0].first ) << ", " << ( extra.empty() ? -1 : extra[0].second ) << ")" );
                CHECK( !expected.empty() );
                CHECK( missing.empty() );
                CHECK( extra.empty() );
            }
        }
    }

    TEST_CASE( "The renderer does not cut a thick line: the engine's clip does, so the spans reach past the viewport" )  {

        // Object at (-200, 100) is left of the viewport at x = 10, so its spans start left of the viewport.
        Frame  frame( "<object id=\"1\" x=\"-200\" y=\"100\" width=\"0\" height=\"0\">"
                      "<properties><property name=\"line_width\" type=\"int\" value=\"2\"/></properties>"
                      "<polyline points=\"0,0 300,0\"/></object>" );

        std :: vector<MockEngine :: Event>  spans = frame.Of( MockEngine :: Event :: FILL );

        REQUIRE( !spans.empty() );
        CHECK( spans[0].x < kViewportOrigin );
    }
}

namespace  {

    // The polygon object XML for a closed shape with the given points and line_width (an int).
    std :: string PolygonObject( const std :: string &strPoints, int nLineWidth )  {

        return "<object id=\"1\" x=\"" + std :: to_string( kObjectX ) + "\" y=\"" + std :: to_string( kObjectY ) + "\" width=\"0\" height=\"0\">"
               "<properties><property name=\"line_width\" type=\"int\" value=\"" + std :: to_string( nLineWidth ) + "\"/></properties>"
               "<polygon points=\"" + strPoints + "\"/></object>";
    }

    // The reference for a path: a pixel is in it when its centre is inside a segment's stroke, or inside the round
    // join of a vertex that has two segments (every vertex of a closed path with more than two points; the
    // interior vertices of an open one). The join is a disc of half the width, strictly inside, as in the renderer.
    std :: set<std :: pair<int, int>>  ReferencePath( const std :: vector<std :: pair<double, double>> &points, bool bClosed, int nWidth )  {

        const double  eps = 1e-9;
        bool          bLoop = bClosed && ( points.size() > 2 );
        size_t        nSegments = bLoop ? points.size() : points.size() - 1;
        double        h = nWidth / 2.0;
        std :: set<std :: pair<int, int>>  pixels;

        for( int nY = 0; nY < 260; nY++ )
            for( int nX = 0; nX < 260; nX++ )  {
                double  cx = nX + 0.5;
                double  cy = nY + 0.5;
                bool    bInside = false;

                for( size_t nIdx = 0; nIdx < nSegments && !bInside; nIdx++ )  {
                    const std :: pair<double, double>  &a = points[nIdx];
                    const std :: pair<double, double>  &b = points[( nIdx + 1 ) % points.size()];

                    // A cap only at the two ends of an open path; none at a join or on a loop.
                    double  eCap    = ( nWidth - 1 ) / 2;
                    double  eBack   = ( !bLoop && nIdx == 0 ) ? eCap : 0;
                    double  eFront  = ( !bLoop && nIdx + 1 == nSegments ) ? eCap : 0;

                    bInside = ReferenceInside( a.first, a.second, b.first, b.second, nWidth, eBack, eFront, cx, cy );
                }

                size_t  nJointFirst = bLoop ? 0 : 1;
                size_t  nJointEnd   = bLoop ? points.size() : points.size() - 1;

                for( size_t nIdx = nJointFirst; nIdx < nJointEnd && !bInside; nIdx++ )  {
                    double  r  = h - eps;
                    // The join's centre is the vertex's pixel centre, as the axis of a segment is.
                    double  dx = cx - ( points[nIdx].first + 0.5 );
                    double  dy = cy - ( points[nIdx].second + 0.5 );

                    bInside = ( dx * dx + dy * dy ) < ( r * r );
                }

                if( bInside )
                    pixels.insert( std :: make_pair( nX, nY ) );
            }

        return pixels;
    }

    // The screen points of object-relative offsets, at zoom 1 with the viewport at (10, 10) and the object at (100, 100).
    std :: vector<std :: pair<double, double>>  ScreenOf( const std :: vector<std :: pair<int, int>> &offsets )  {

        std :: vector<std :: pair<double, double>>  points;

        for( const std :: pair<int, int> &offset : offsets )
            points.push_back( std :: make_pair( kViewportOrigin + kObjectX + offset.first, kViewportOrigin + kObjectY + offset.second ) );

        return points;
    }
}

TEST_SUITE( "Thick joins" )  {

    TEST_CASE( "A sharp corner has no gap: the spans are the strokes plus the round join at the corner" )  {

        // Out to (40, 0) and straight back to (0, 6): the second segment doubles back over the first, so the
        // corner at (40, 0) is a sharp turn - the case where two butt ends leave a notch.
        Frame  frame( PolylineObject( "0,0 40,0 0,6", 4 ) );

        std :: set<std :: pair<int, int>>  expected = ReferencePath( ScreenOf( { { 0, 0 }, { 40, 0 }, { 0, 6 } } ), false, 4 );

        CHECK( !expected.empty() );
        CHECK( frame.Covered() == expected );
    }

    TEST_CASE( "A closed polygon joins every corner, including the one where the closing edge meets the first point" )  {

        Frame  frame( PolygonObject( "0,0 50,0 25,40", 3 ) );

        std :: set<std :: pair<int, int>>  expected = ReferencePath( ScreenOf( { { 0, 0 }, { 50, 0 }, { 25, 40 } } ), true, 3 );
        std :: set<std :: pair<int, int>>  covered  = frame.Covered();
        std :: vector<std :: pair<int, int>>  missing, extra;

        std :: set_difference( expected.begin(), expected.end(), covered.begin(), covered.end(), std :: back_inserter( missing ) );
        std :: set_difference( covered.begin(), covered.end(), expected.begin(), expected.end(), std :: back_inserter( extra ) );

        INFO( "expected " << expected.size() << " covered " << covered.size() << " missing " << missing.size() << " extra " << extra.size() );
        CHECK( !expected.empty() );
        CHECK( missing.empty() );
        CHECK( extra.empty() );
    }

    TEST_CASE( "A polygon of two points is one stroke: no closing edge and no join" )  {

        Frame  frame( PolygonObject( "0,0 50,0", 3 ) );

        std :: set<std :: pair<int, int>>  expected = ReferencePath( ScreenOf( { { 0, 0 }, { 50, 0 } } ), true, 3 );

        CHECK( !expected.empty() );
        CHECK( frame.Covered() == expected );
    }
}
