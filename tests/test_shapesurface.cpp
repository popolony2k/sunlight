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
 * M1 of the master plan: the Phase C/D shape primitives, promoted onto a public, mapless
 * IShapeSurface - the same underlying drawing code DrawObjects already uses for Tiled shape
 * objects, given a public door. These tests cover the two things M1 itself adds: that the door
 * actually opens with no map ever loaded, and that walking through it reaches the exact same
 * drawing code a TMX object would (not a fork of it).
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: Base :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    const SunLight :: Base :: stColor  __WHITE { 0xFF, 0xFF, 0xFF, 0xFF };

    // A renderer with the given viewport, no map ever loaded - Start() only.
    struct MaplessScene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        MaplessScene( void )  {

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 10, 10, 1000, 800 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            // Deliberately never LoadMap: this is the whole point of M1.
        }

        ~MaplessScene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        std :: vector<Event>  Of( Event :: Kind kind ) const  {
            std :: vector<Event>  found;

            for( const Event &evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }
    };

    // A one-object TMX map (the polyline object XML spliced in as given), loaded and rendered -
    // the same fixture shape tests/test_thick_lines.cpp already uses, for the regression guard
    // below (it needs a real TMX object to compare the public API's own spans against).
    Bytes MakeLineTmx( const std :: string &strObject )  {

        std :: string  str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                             "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
                             " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
                             "<objectgroup id=\"2\" name=\"lines\">" + strObject + "</objectgroup></map>";

        return Bytes( str.begin(), str.end() );
    }

    struct TmxScene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        explicit TmxScene( const std :: string &strObject )  {

            fsFixture.fs.files["maps/line.tmx"] = MakeLineTmx( strObject );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 10, 10, 1000, 800 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/line.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            windowFixture.window.nFramesUntilShouldClose = 1;
            pRenderer -> Run();
        }

        ~TmxScene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        std :: vector<Event>  Of( Event :: Kind kind ) const  {
            std :: vector<Event>  found;

            for( const Event &evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }
    };

    bool SameFillSpans( const std :: vector<Event> &left, const std :: vector<Event> &right )  {

        if( left.size() != right.size() )
            return false;

        for( size_t nIdx = 0; nIdx < left.size(); nIdx++ )
            if( ( left[nIdx].x != right[nIdx].x ) || ( left[nIdx].y != right[nIdx].y ) ||
                ( left[nIdx].w != right[nIdx].w ) || ( left[nIdx].h != right[nIdx].h ) )
                return false;

        return true;
    }
}

TEST_SUITE( "renderer/shapesurface" )  {

    TEST_CASE( "A renderer that never loads a map can still draw a hairline rectangle through IShapeSurface" )  {

        MaplessScene  scene;

        // Default thickness (1.0) and rotation (0.0): a hairline, the engine's own 4 edge lines.
        scene.pRenderer -> DrawRectangle( 10, 10, 50, 30, __WHITE );

        CHECK( scene.Of( Event :: LINE ).size() == 4 );
        CHECK( scene.Of( Event :: FILL ).empty() );
    }

    TEST_CASE( "A renderer that never loads a map can still draw a thick polygon through IShapeSurface" )  {

        MaplessScene  scene;

        SunLight :: Base :: stVector2D  points[] = { { 0, 0 }, { 10, 0 }, { 10, 10 }, { 0, 10 } };

        scene.pRenderer -> DrawPolygon( 50, 50, points, 4, __WHITE, 3.0 );

        // A closed, thick (3px) square: spans, not hairline edges - the exact count isn't the
        // point here (test_thick_lines.cpp already pins that for the private path this delegates
        // to), only that calling it through the public, mapless door produces real span output.
        CHECK( !scene.Of( Event :: FILL ).empty() );
        CHECK( scene.Of( Event :: LINE ).empty() );
    }

    TEST_CASE( "A renderer that never loads a map can still draw a point through IShapeSurface" )  {

        MaplessScene  scene;

        scene.pRenderer -> DrawPoint( 20, 20, __WHITE, 4.0 );

        REQUIRE( scene.Of( Event :: FILL ).size() == 1 );
        CHECK( scene.Of( Event :: FILL )[0].w == 4.0f );   // 4 map units x zoom 1.0
    }

    TEST_CASE( "Thickness is given in map units and converted to screen pixels by the current zoom, not passed through raw" )  {

        // The exact scenario tests/test_thick_lines.cpp's own "zoom scales the width" case pins
        // for a TMX polyline object (width-3 line at zoom 2 is six rows) - reproduced here through
        // the public wrapper instead, so this specifically proves the ZOOM CONVERSION happens
        // inside the wrapper, not just whatever a caller already pre-converted (a mutation
        // skipping that conversion entirely would still pass every OTHER case in this file, since
        // they all run at the default zoom 1.0, where a raw pass-through and a real conversion
        // coincide - found live, confirmed by the mutation check below).
        MaplessScene  scene;

        scene.pRenderer -> GetViewport().SetZoom( 31u );   // zoom position 31 = factor 2.0
        REQUIRE( scene.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor == 2.0f );

        SunLight :: Base :: stVector2D  points[] = { { 0, 0 }, { 10, 0 } };

        scene.pRenderer -> DrawPolyline( 100, 100, points, 2, __WHITE, 3.0 );

        std :: vector<Event>  spans = scene.Of( Event :: FILL );

        REQUIRE( spans.size() == 6 );

        for( int nRow = 0; nRow < 6; nRow++ )  {
            CHECK( spans[nRow].x == 208.0f );
            CHECK( spans[nRow].y == ( float ) ( 207 + nRow ) );
            CHECK( spans[nRow].w == 24.0f );
        }
    }

    TEST_CASE( "A rotated, thick polyline drawn through IShapeSurface produces the exact same spans as the same shape drawn via a TMX object" )  {

        // EngineFactory's mock override is a single global slot (see MockEngineFixture), not a
        // stack: two Scene fixtures alive at once would have the second one's constructor clobber
        // the first's install, and whichever is destroyed first (always the second, C++'s own
        // reverse local-destruction order) would clear the slot out from under the first, still
        // live one - its own later Stop() would then reach the REAL engine with a mock-issued
        // handle (confirmed live: a real SEGV in RaylibEngine::UnloadRenderTarget, caught before
        // this test was trusted). The TMX side is therefore fully torn down (its own block ends,
        // and with it tmxScene's destructor and its fixture's engine-slot reset) before the
        // mapless side's own Scene - carrying its own fresh fixture - is ever constructed.
        std :: vector<Event>  reference;

        {
            // The TMX side: a width-3 polyline turned 45 degrees, the same object
            // tests/test_rotated_tiles_text.cpp-style cases already exercise.
            TmxScene  tmxScene( "<object id=\"1\" x=\"100\" y=\"100\" width=\"0\" height=\"0\" rotation=\"45\">"
                                "<properties><property name=\"line_width\" type=\"int\" value=\"3\"/></properties>"
                                "<polyline points=\"0,0 20,0\"/></object>" );

            reference = tmxScene.Of( Event :: FILL );
        }

        REQUIRE( !reference.empty() );   // sanity: the TMX side actually produced spans

        // The public side: the exact same shape, position and thickness, called directly.
        MaplessScene  scene;
        SunLight :: Base :: stVector2D  points[] = { { 0, 0 }, { 20, 0 } };

        scene.pRenderer -> DrawPolyline( 100, 100, points, 2, __WHITE, 3.0, 45.0 );

        CHECK( SameFillSpans( scene.Of( Event :: FILL ), reference ) );
    }

    TEST_CASE( "A rotated, thick ellipse drawn through IShapeSurface produces the exact same spans as the same shape drawn via a TMX object" )  {

        // See the previous case's own comment: the TMX side must be fully torn down (its own
        // fixture's engine-slot reset) before the mapless side's Scene is constructed.
        std :: vector<Event>  reference;

        {
            TmxScene  tmxScene( "<object id=\"1\" x=\"100\" y=\"100\" width=\"40\" height=\"20\" rotation=\"45\">"
                                "<properties><property name=\"line_width\" type=\"int\" value=\"4\"/></properties>"
                                "<ellipse/></object>" );

            reference = tmxScene.Of( Event :: FILL );
        }

        REQUIRE( !reference.empty() );

        MaplessScene  scene;

        scene.pRenderer -> DrawEllipse( 100, 100, 40, 20, __WHITE, 4.0, 45.0 );

        CHECK( SameFillSpans( scene.Of( Event :: FILL ), reference ) );
    }

    TEST_CASE( "Promoting the shape primitives to public did not change the existing TMX-object-driven path" )  {

        // The exact width-2 horizontal line case tests/test_thick_lines.cpp already pins - if
        // promoting DrawRectangle/DrawPolyline/etc. to also be callable publicly had accidentally
        // forked or altered the private bodies DrawObjects calls, this would catch it.
        TmxScene  tmxScene( "<object id=\"1\" x=\"100\" y=\"100\" width=\"0\" height=\"0\">"
                            "<properties><property name=\"line_width\" type=\"int\" value=\"2\"/></properties>"
                            "<polyline points=\"0,0 10,0\"/></object>" );

        std :: vector<Event>  spans = tmxScene.Of( Event :: FILL );

        REQUIRE( spans.size() == 2 );
        CHECK( spans[0].x == 110.0f );  CHECK( spans[0].y == 109.0f );  CHECK( spans[0].w == 10.0f );
        CHECK( spans[1].x == 110.0f );  CHECK( spans[1].y == 110.0f );  CHECK( spans[1].w == 10.0f );
    }
}
