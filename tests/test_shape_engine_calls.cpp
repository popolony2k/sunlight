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
#include <memory>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // The viewport is at (10, 10) with the camera at (0, 0) and zoom 1, so a shape at object (x, y)
    // starts at screen (10 + x, 10 + y). Shapes are drawn at their own position: the clip, not the
    // viewport size, is what cuts them, so the engine calls below carry the full, unclipped geometry.
    const float  kViewportOrigin = 10.0f;
    const float  kEdgeThickness  = 1.0f;     // __EDGE_LINE_THICKNESS in tilemaprenderer.cpp

    // A one-object map: the object's XML (its shape child, if any) is spliced in as given.
    Bytes MakeShapeTmx( const std :: string &strObject )  {

        std :: string  str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                             "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
                             " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
                             "<objectgroup id=\"2\" name=\"shapes\">" + strObject + "</objectgroup></map>";

        return Bytes( str.begin(), str.end() );
    }

    // Renders one frame of the map with a viewport at (10, 10, 1000, 800) and returns the mock engine's events.
    struct Frame  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        explicit Frame( const std :: string &strObject )  {

            fsFixture.fs.files["maps/shape.tmx"] = MakeShapeTmx( strObject );

            RendererConfig  config;

            config.fWidth  = 1260.0f;
            config.fHeight = 920.0f;

            SunLight :: Base :: stDimension2D  viewport {};

            viewport.pos.x = 10;  viewport.pos.y = 10;  viewport.size.nWidth = 1000;  viewport.size.nHeight = 800;
            config.viewport = viewport;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            windowFixture.window.nFramesUntilShouldClose = 1;
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/shape.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            pRenderer -> Run();
        }

        ~Frame( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        // Every event of the given kind, in the order the renderer drew them.
        std :: vector<MockEngine :: Event>  Of( MockEngine :: Event :: Kind kind ) const  {
            std :: vector<MockEngine :: Event>  found;

            for( const MockEngine :: Event &evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }
    };
}

TEST_SUITE( "Shape engine calls" )  {

    TEST_CASE( "A rectangle is four engine lines of thickness 1: top, bottom, left, right" )  {

        // Object at (50, 50), 200 x 200: its edges are at x = 60 and 260, y = 60 and 260 on screen.
        Frame  frame( "<object id=\"1\" x=\"50\" y=\"50\" width=\"200\" height=\"200\"/>" );

        std :: vector<MockEngine :: Event>  lines = frame.Of( MockEngine :: Event :: LINE );

        REQUIRE( lines.size() == 4 );

        const float  x0 = kViewportOrigin + 50.0f, x1 = kViewportOrigin + 250.0f;
        const float  y0 = kViewportOrigin + 50.0f, y1 = kViewportOrigin + 250.0f;

        // Each event: x, y = first end; w, h = second end; scale = thickness.
        CHECK( lines[0].x == x0 );  CHECK( lines[0].y == y0 );  CHECK( lines[0].w == x1 );  CHECK( lines[0].h == y0 );  // top
        CHECK( lines[1].x == x0 );  CHECK( lines[1].y == y1 );  CHECK( lines[1].w == x1 );  CHECK( lines[1].h == y1 );  // bottom
        CHECK( lines[2].x == x0 );  CHECK( lines[2].y == y0 );  CHECK( lines[2].w == x0 );  CHECK( lines[2].h == y1 );  // left
        CHECK( lines[3].x == x1 );  CHECK( lines[3].y == y0 );  CHECK( lines[3].w == x1 );  CHECK( lines[3].h == y1 );  // right

        for( const MockEngine :: Event &line : lines )
            CHECK( line.scale == kEdgeThickness );
    }

    TEST_CASE( "An ellipse is one engine outline centred on its box, with radii of half its size" )  {

        // Object at (50, 50), 200 x 200: centre at (10 + 50 + 100, 10 + 50 + 100), radii 100.
        Frame  frame( "<object id=\"1\" x=\"50\" y=\"50\" width=\"200\" height=\"200\"><ellipse/></object>" );

        std :: vector<MockEngine :: Event>  ellipses = frame.Of( MockEngine :: Event :: ELLIPSE );

        REQUIRE( ellipses.size() == 1 );
        CHECK( ellipses[0].x == kViewportOrigin + 150.0f );   // centre x
        CHECK( ellipses[0].y == kViewportOrigin + 150.0f );   // centre y
        CHECK( ellipses[0].w == 100.0f );                     // radius x
        CHECK( ellipses[0].h == 100.0f );                     // radius y
    }

    TEST_CASE( "A polyline is one line per segment, with no closing edge" )  {

        // Points (0, 0), (50, 0), (50, 50) on an object at (100, 100): segments (110,110)-(160,110) and (160,110)-(160,160).
        Frame  frame( "<object id=\"1\" x=\"100\" y=\"100\" width=\"0\" height=\"0\">"
                      "<polyline points=\"0,0 50,0 50,50\"/></object>" );

        std :: vector<MockEngine :: Event>  lines = frame.Of( MockEngine :: Event :: LINE );

        REQUIRE( lines.size() == 2 );
        CHECK( lines[0].x == 110.0f );  CHECK( lines[0].y == 110.0f );  CHECK( lines[0].w == 160.0f );  CHECK( lines[0].h == 110.0f );
        CHECK( lines[1].x == 160.0f );  CHECK( lines[1].y == 110.0f );  CHECK( lines[1].w == 160.0f );  CHECK( lines[1].h == 160.0f );
    }

    TEST_CASE( "A polygon adds its closing edge, from the first point to the last" )  {

        // The same three points as a polygon: the two polyline segments, then the closing edge (110,110)-(160,160).
        Frame  frame( "<object id=\"1\" x=\"100\" y=\"100\" width=\"0\" height=\"0\">"
                      "<polygon points=\"0,0 50,0 50,50\"/></object>" );

        std :: vector<MockEngine :: Event>  lines = frame.Of( MockEngine :: Event :: LINE );

        REQUIRE( lines.size() == 3 );
        CHECK( lines[2].x == 110.0f );  CHECK( lines[2].y == 110.0f );  CHECK( lines[2].w == 160.0f );  CHECK( lines[2].h == 160.0f );
    }

    TEST_CASE( "A polygon of two points has no closing edge: it is the single segment" )  {

        Frame  frame( "<object id=\"1\" x=\"100\" y=\"100\" width=\"0\" height=\"0\">"
                      "<polygon points=\"0,0 50,0\"/></object>" );

        std :: vector<MockEngine :: Event>  lines = frame.Of( MockEngine :: Event :: LINE );

        REQUIRE( lines.size() == 1 );
        CHECK( lines[0].x == 110.0f );  CHECK( lines[0].y == 110.0f );  CHECK( lines[0].w == 160.0f );  CHECK( lines[0].h == 110.0f );
    }
}
