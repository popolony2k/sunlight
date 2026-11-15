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

/*
 * S1a of the master plan: ShapeRenderer draws on its own, from a DrawContext - no TileMapRenderer
 * anywhere in these tests, only the mock engine. The renderer-side tests (test_shape_engine_calls,
 * test_shapesurface, test_thick_lines, ...) still pin what a map's shapes look like through the
 * renderer; these pin the context arithmetic itself: a map point p lands at
 * (p + camera) x zoom + viewport.pos, inside one clip of the viewport.
 */

#include <doctest/doctest.h>
#include <vector>
#include "renderer/shape/shaperenderer.h"
#include "mock_engine.h"

using namespace SunLight :: Renderer;
typedef MockEngine :: Event  Event;

namespace  {

    const SunLight :: Base :: stColor  RED = { 0xFF, 0x00, 0x00, 0xFF };

    const int  HAIRLINE = 1;
    const int  WIDE     = 3;

    // Zoom 1, no camera, the viewport at (10, 10): a map point (x, y) lands at (10 + x, 10 + y).
    DrawContext PlainContext( void )  {
        return DrawContext { { { 10, 10 }, { 1000, 800 } }, 1.0f, { 0.0f, 0.0f } };
    }

    // Zoom 2, a camera of (5, -3), the viewport at (100, 200): (x, y) lands at
    // ((x + 5) x 2 + 100, (y - 3) x 2 + 200).
    DrawContext ScaledContext( void )  {
        return DrawContext { { { 100, 200 }, { 640, 480 } }, 2.0f, { 5.0f, -3.0f } };
    }

    std :: vector<Event> EventsOf( const MockEngine& engine, Event :: Kind kind )  {
        std :: vector<Event>  found;

        for( const Event& evt : engine.events )
            if( evt.kind == kind )
                found.push_back( evt );

        return found;
    }

    void CheckLine( const Event& evt, float fX0, float fY0, float fX1, float fY1 )  {
        CHECK( evt.x == fX0 );
        CHECK( evt.y == fY0 );
        CHECK( evt.w == fX1 );
        CHECK( evt.h == fY1 );
    }

    // A shape is drawn inside exactly one clip of the context's viewport, one pixel in from its
    // top and left edges (Shape::PrimitiveClip): the first event opens it, the last closes it.
    void CheckClippedOnce( const MockEngine& engine, const DrawContext& context )  {
        REQUIRE( engine.events.size() >= 2 );
        CHECK( engine.events.front().kind == Event :: CLIP_BEGIN );
        CHECK( engine.events.front().x == ( float ) ( context.viewport.pos.x + 1 ) );
        CHECK( engine.events.front().y == ( float ) ( context.viewport.pos.y + 1 ) );
        CHECK( engine.events.front().w == ( float ) ( context.viewport.size.nWidth - 1 ) );
        CHECK( engine.events.front().h == ( float ) ( context.viewport.size.nHeight - 1 ) );
        CHECK( engine.events.back().kind == Event :: CLIP_END );
    }
}

TEST_SUITE( "renderer/ShapeRenderer" )  {

    TEST_CASE( "A hairline rectangle is its four edges - top, bottom, left, right - inside one viewport clip" )  {

        MockEngineFixture  fixture;
        ShapeRenderer      shapes;
        DrawContext        context = PlainContext();

        shapes.DrawRectangle( context, 20.0, 30.0, 40.0, 50.0, 0.0, HAIRLINE, RED );

        CheckClippedOnce( fixture.engine, context );

        std :: vector<Event>  lines = EventsOf( fixture.engine, Event :: LINE );

        REQUIRE( lines.size() == 4 );
        CheckLine( lines[0], 30.0f, 40.0f, 70.0f, 40.0f );
        CheckLine( lines[1], 30.0f, 90.0f, 70.0f, 90.0f );
        CheckLine( lines[2], 30.0f, 40.0f, 30.0f, 90.0f );
        CheckLine( lines[3], 70.0f, 40.0f, 70.0f, 90.0f );
    }

    TEST_CASE( "The context places a shape: camera added, then the zoom, then the viewport position" )  {

        MockEngineFixture  fixture;
        ShapeRenderer      shapes;
        DrawContext        context = ScaledContext();

        shapes.DrawRectangle( context, 20.0, 30.0, 10.0, 10.0, 0.0, HAIRLINE, RED );

        CheckClippedOnce( fixture.engine, context );

        std :: vector<Event>  lines = EventsOf( fixture.engine, Event :: LINE );

        // Corners: ((20 + 5) x 2 + 100, (30 - 3) x 2 + 200) = (150, 254), and 10 map units = 20 pixels.
        REQUIRE( lines.size() == 4 );
        CheckLine( lines[0], 150.0f, 254.0f, 170.0f, 254.0f );
        CheckLine( lines[1], 150.0f, 274.0f, 170.0f, 274.0f );
    }

    TEST_CASE( "An unturned hairline ellipse is the engine's own outline, centred in its box" )  {

        MockEngineFixture  fixture;
        ShapeRenderer      shapes;
        DrawContext        context = ScaledContext();

        shapes.DrawEllipse( context, 20.0, 30.0, 10.0, 6.0, 0.0, HAIRLINE, RED );

        CheckClippedOnce( fixture.engine, context );

        std :: vector<Event>  ellipses = EventsOf( fixture.engine, Event :: ELLIPSE );

        // Centre: ((20 + 5 + 5) x 2 + 100, (30 + 3 - 3) x 2 + 200) = (160, 260); radii 5 x 2 and 3 x 2.
        REQUIRE( ellipses.size() == 1 );
        CHECK( ellipses[0].x == 160.0f );
        CHECK( ellipses[0].y == 260.0f );
        CHECK( ellipses[0].w == 10.0f );
        CHECK( ellipses[0].h == 6.0f );
    }

    TEST_CASE( "A turned or wide ellipse is a ring of spans, not the engine's outline" )  {

        for( auto [ fRotation, nWidth ] : { std :: pair<double, int>{ 30.0, HAIRLINE }, std :: pair<double, int>{ 0.0, WIDE } } )  {

            MockEngineFixture  fixture;
            ShapeRenderer      shapes;
            DrawContext        context = PlainContext();

            shapes.DrawEllipse( context, 20.0, 30.0, 40.0, 20.0, fRotation, nWidth, RED );

            CheckClippedOnce( fixture.engine, context );
            CHECK( EventsOf( fixture.engine, Event :: ELLIPSE ).empty() );
            CHECK( !EventsOf( fixture.engine, Event :: FILL ).empty() );
        }
    }

    TEST_CASE( "A hairline polyline joins its points in order and stays open; a polygon also closes it" )  {

        double   p0[] = { 0.0, 0.0 }, p1[] = { 10.0, 0.0 }, p2[] = { 10.0, 10.0 };
        double  *points[] = { p0, p1, p2 };

        {
            MockEngineFixture  fixture;
            ShapeRenderer      shapes;

            shapes.DrawPolyline( PlainContext(), 20.0, 30.0, points, 3, 0.0, HAIRLINE, RED );

            std :: vector<Event>  lines = EventsOf( fixture.engine, Event :: LINE );

            REQUIRE( lines.size() == 2 );
            CheckLine( lines[0], 30.0f, 40.0f, 40.0f, 40.0f );
            CheckLine( lines[1], 40.0f, 40.0f, 40.0f, 50.0f );
        }
        {
            MockEngineFixture  fixture;
            ShapeRenderer      shapes;

            shapes.DrawPolygon( PlainContext(), 20.0, 30.0, points, 3, 0.0, HAIRLINE, RED );

            std :: vector<Event>  lines = EventsOf( fixture.engine, Event :: LINE );

            REQUIRE( lines.size() == 3 );
            CheckLine( lines[2], 30.0f, 40.0f, 40.0f, 50.0f );            // the closing edge: first point to last
        }
    }

    TEST_CASE( "A wide polyline is a stroked path of spans, with no hairline edges" )  {

        MockEngineFixture  fixture;
        ShapeRenderer      shapes;
        DrawContext        context = PlainContext();
        double             p0[] = { 0.0, 0.0 }, p1[] = { 40.0, 0.0 };
        double            *points[] = { p0, p1 };

        shapes.DrawPolyline( context, 20.0, 30.0, points, 2, 0.0, WIDE, RED );

        CheckClippedOnce( fixture.engine, context );
        CHECK( EventsOf( fixture.engine, Event :: LINE ).empty() );
        CHECK( !EventsOf( fixture.engine, Event :: FILL ).empty() );
    }

    TEST_CASE( "A turned rectangle's corners turn about its stored point" )  {

        MockEngineFixture  fixture;
        ShapeRenderer      shapes;

        // A quarter turn clockwise (y grows downwards): the top edge (20, 30)-(60, 30) becomes (20, 30)-(20, 70).
        shapes.DrawRectangle( PlainContext(), 20.0, 30.0, 40.0, 10.0, 90.0, HAIRLINE, RED );

        std :: vector<Event>  lines = EventsOf( fixture.engine, Event :: LINE );

        REQUIRE( lines.size() == 4 );
        CheckLine( lines[0], 30.0f, 40.0f, 30.0f, 80.0f );
    }

    TEST_CASE( "A point is a filled square, its side the size times the zoom, at its own position" )  {

        MockEngineFixture  fixture;
        ShapeRenderer      shapes;
        DrawContext        context = ScaledContext();

        shapes.DrawPoint( context, 20.0, 30.0, 3.0, RED );

        CheckClippedOnce( fixture.engine, context );

        std :: vector<Event>  fills = EventsOf( fixture.engine, Event :: FILL );

        REQUIRE( fills.size() == 1 );
        CHECK( fills[0].x == 150.0f );
        CHECK( fills[0].y == 254.0f );
        CHECK( fills[0].w == 6.0f );
        CHECK( fills[0].h == 6.0f );
    }
}
