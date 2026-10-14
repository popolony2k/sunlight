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
 * S1b of the master plan: FontRegistry, ObjectOpacity, TextObjectRenderer and ObjectRenderer on
 * their own - no TileMapRenderer, only the mock engine and objects libtmx parsed from a small map.
 * The renderer-side suites (test_textobjects, test_objectopacity, test_rotated_tiles_text, ...)
 * still pin what a map's objects look like through the renderer.
 */

#include <doctest/doctest.h>
#include <cstring>
#include <string>
#include <vector>
#include "tmx.h"
#include "renderer/text/fontregistry.h"
#include "renderer/objects/objectopacity.h"
#include "renderer/objects/textobjectrenderer.h"
#include "renderer/objects/objectrenderer.h"
#include "mock_engine.h"

using namespace SunLight :: Renderer;
typedef MockEngine :: Event  Event;

namespace  {

    // One object group ("things", opaque red, opacity 1): a rectangle, a text object in family "Sans" with an
    // "opacity" of 0.5, a text object in an unregistered family, a tile object (gid 1) with an
    // "opacity" of 0.25, a hidden rectangle, and a text object far outside any viewport below.
    const char  *g_szObjects =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"t.png\" width=\"16\" height=\"16\"/></tileset>"
        "<objectgroup id=\"1\" name=\"things\" color=\"#ffff0000\">"
        "<object id=\"1\" name=\"box\" x=\"20\" y=\"30\" width=\"10\" height=\"10\"/>"
        "<object id=\"2\" name=\"label\" x=\"20\" y=\"30\" width=\"0\" height=\"0\">"
        "<properties><property name=\"opacity\" type=\"float\" value=\"0.5\"/></properties>"
        "<text fontfamily=\"Sans\" pixelsize=\"10\" color=\"#ffffffff\">Hello</text></object>"
        "<object id=\"3\" name=\"orphan\" x=\"20\" y=\"60\" width=\"0\" height=\"0\"><text fontfamily=\"Serif\" pixelsize=\"10\">Lost</text></object>"
        "<object id=\"4\" name=\"tile\" gid=\"1\" x=\"0\" y=\"16\" width=\"16\" height=\"16\">"
        "<properties><property name=\"opacity\" type=\"float\" value=\"0.25\"/></properties></object>"
        "<object id=\"5\" name=\"hidden\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" visible=\"0\"/>"
        "<object id=\"6\" name=\"far\" x=\"20\" y=\"100000\" width=\"10\" height=\"10\"><text fontfamily=\"Sans\" pixelsize=\"10\">Far</text></object>"
        "<object id=\"7\" name=\"invalid\" x=\"0\" y=\"0\" width=\"10\" height=\"10\">"
        "<properties><property name=\"opacity\" type=\"int\" value=\"5\"/></properties></object>"
        "</objectgroup></map>";

    // The map above, parsed by libtmx, freed with the fixture. libtmx's image hooks are process-wide,
    // and a renderer that has started leaves its own texture loader in them - which would load the
    // tileset's image through whatever engine is installed. These tests load no images: the hooks
    // are off while the map is loaded and freed, and put back after.
    struct ParsedObjects  {
        tmx_map  *pMap = nullptr;
        void*    ( *pSavedLoad )( const char* ) = tmx_img_load_func;
        void     ( *pSavedFree )( void* )       = tmx_img_free_func;

        ParsedObjects( void )  {
            tmx_img_load_func = nullptr;
            tmx_img_free_func = nullptr;
            pMap = ::tmx_load_buffer( g_szObjects, ( int ) std :: strlen( g_szObjects ) );
            REQUIRE( pMap != nullptr );
        }

        ~ParsedObjects( void )  {
            ::tmx_map_free( pMap );
            tmx_img_load_func = pSavedLoad;
            tmx_img_free_func = pSavedFree;
        }

        tmx_layer* Layer( void )  {
            return pMap -> ly_head;
        }

        tmx_object* Object( const char *szName )  {
            for( tmx_object *pObject = Layer() -> content.objgr -> head; pObject != nullptr; pObject = pObject -> next )
                if( ( pObject -> name != nullptr ) && ( std :: strcmp( pObject -> name, szName ) == 0 ) )
                    return pObject;

            FAIL( "no object named " << szName );
            return nullptr;
        }
    };

    // Zoom 1, no camera: a shape's default line (one map unit) is one pixel, so its edges are the
    // engine's own lines (LINE events). At zoom 2 it would be two pixels, a stroked path of spans.
    DrawContext PlainContext( void )  {
        return DrawContext { { { 10, 10 }, { 1000, 800 } }, 1.0f, { 0.0f, 0.0f } };
    }

    // Zoom 2, a camera of (5, -3), the viewport at (100, 200): (x, y) lands at
    // ((x + 5) x 2 + 100, (y - 3) x 2 + 200).
    DrawContext ScaledContext( void )  {
        return DrawContext { { { 100, 200 }, { 640, 480 } }, 2.0f, { 5.0f, -3.0f } };
    }

    std :: vector<Event> EventsOf( const MockEngine &engine, Event :: Kind kind )  {
        std :: vector<Event>  found;

        for( const Event &evt : engine.events )
            if( evt.kind == kind )
                found.push_back( evt );

        return found;
    }

    const ObjectRenderer :: TILE_OBJECT_DRAWER  NO_TILE_OBJECTS = []( tmx_object*, tmx_layer*, float ) {};
}

TEST_SUITE( "renderer/FontRegistry" )  {

    TEST_CASE( "A registered font is found by its exact family and style, and by nothing else" )  {

        MockEngineFixture  fixture;
        FontRegistry       fonts;

        REQUIRE( fonts.Register( "Sans", "sans.fnt", false, false ) == true );

        const void  *pLoaded = fixture.engine.hLastLoadedFont;

        CHECK( fonts.Find( "Sans", false, false ) == pLoaded );
        CHECK( fonts.Find( "Sans", true, false ) == nullptr );
        CHECK( fonts.Find( "Sans", false, true ) == nullptr );
        CHECK( fonts.Find( "Serif", false, false ) == nullptr );
    }

    TEST_CASE( "Registering a family and style again replaces the font, releasing the old one" )  {

        MockEngineFixture  fixture;
        FontRegistry       fonts;

        REQUIRE( fonts.Register( "Sans", "a.fnt", true, false ) == true );
        REQUIRE( fonts.Register( "Sans", "b.fnt", true, false ) == true );

        CHECK( fixture.engine.nFontsDestroyed == 1 );
        CHECK( fonts.Find( "Sans", true, false ) == fixture.engine.hLastLoadedFont );
    }

    TEST_CASE( "A font that does not load, or a missing name, registers nothing and keeps the earlier font" )  {

        MockEngineFixture  fixture;
        FontRegistry       fonts;

        REQUIRE( fonts.Register( "Sans", "a.fnt", false, false ) == true );

        const void  *pKept = fixture.engine.hLastLoadedFont;

        fixture.engine.bLoadFontResult = false;

        CHECK( fonts.Register( "Sans", "broken.fnt", false, false ) == false );
        CHECK( fonts.Register( nullptr, "a.fnt", false, false ) == false );
        CHECK( fonts.Register( "Sans", nullptr, false, false ) == false );
        CHECK( fonts.Find( "Sans", false, false ) == pKept );
    }

    TEST_CASE( "Clear releases every font" )  {

        MockEngineFixture  fixture;
        FontRegistry       fonts;

        fonts.Register( "Sans", "a.fnt", false, false );
        fonts.Register( "Mono", "b.ttf", false, true );
        fonts.Clear();

        CHECK( fixture.engine.nFontsDestroyed == 2 );
        CHECK( fonts.Find( "Sans", false, false ) == nullptr );
        CHECK( fonts.Find( "Mono", false, true ) == nullptr );
    }
}

TEST_SUITE( "renderer/ObjectOpacity" )  {

    TEST_CASE( "An object's own usable opacity wins; none, or an unusable one, falls back to its layer's" )  {

        ParsedObjects  objects;
        ObjectOpacity  opacity;

        CHECK( opacity.Of( objects.Object( "label" ), 0.75f ) == 0.5f );
        CHECK( opacity.Of( objects.Object( "box" ), 0.75f ) == 0.75f );
        CHECK( opacity.Of( objects.Object( "invalid" ), 0.75f ) == 0.75f );
        CHECK( opacity.Of( objects.Object( "invalid" ), 0.75f ) == 0.75f );      // warned once, answers the same
        opacity.Reset();
        CHECK( opacity.Of( objects.Object( "invalid" ), 0.75f ) == 0.75f );
    }
}

TEST_SUITE( "renderer/TextObjectRenderer" )  {

    TEST_CASE( "A text object is drawn with its registered font, placed and sized by the context" )  {

        MockEngineFixture   fixture;
        ParsedObjects       objects;
        FontRegistry        fonts;
        ObjectOpacity       opacity;
        TextObjectRenderer  text;

        REQUIRE( fonts.Register( "Sans", "sans.fnt", false, false ) == true );

        text.Draw( ScaledContext(), objects.Object( "label" ), objects.Layer(), fonts, opacity );

        std :: vector<Event>  texts = EventsOf( fixture.engine, Event :: TEXT );

        // ((20 + 5) x 2 + 100, (30 - 3) x 2 + 200) = (150, 254); pixelsize 10 x zoom 2 = 20.
        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].text == "Hello" );
        CHECK( texts[0].font == fixture.engine.hLastLoadedFont );
        CHECK( texts[0].x == 150.0f );
        CHECK( texts[0].y == 254.0f );
        CHECK( texts[0].nFontSize == 20 );
        CHECK( texts[0].color.nAlpha == 127 );          // its own "opacity" 0.5 times the colour's 255
    }

    TEST_CASE( "A text object whose font is not registered draws the placeholder with the active font" )  {

        MockEngineFixture   fixture;
        ParsedObjects       objects;
        FontRegistry        fonts;
        ObjectOpacity       opacity;
        TextObjectRenderer  text;

        text.Draw( ScaledContext(), objects.Object( "orphan" ), objects.Layer(), fonts, opacity );

        std :: vector<Event>  texts = EventsOf( fixture.engine, Event :: TEXT );

        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].text == "??????" );
        CHECK( texts[0].font == nullptr );
    }

    TEST_CASE( "A text object entirely outside the viewport is not drawn" )  {

        MockEngineFixture   fixture;
        ParsedObjects       objects;
        FontRegistry        fonts;
        ObjectOpacity       opacity;
        TextObjectRenderer  text;

        fonts.Register( "Sans", "sans.fnt", false, false );
        text.Draw( ScaledContext(), objects.Object( "far" ), objects.Layer(), fonts, opacity );

        CHECK( EventsOf( fixture.engine, Event :: TEXT ).empty() );
    }
}

TEST_SUITE( "renderer/ObjectRenderer" )  {

    TEST_CASE( "An object layer: shapes and text drawn in order, tile objects handed back with their opacity, hidden ones skipped" )  {

        MockEngineFixture   fixture;
        ParsedObjects       objects;
        FontRegistry        fonts;
        ObjectRenderer      renderer;
        std :: vector<std :: pair<tmx_object*, float>>  tileObjects;

        fonts.Register( "Sans", "sans.fnt", false, false );

        renderer.Draw( PlainContext(), objects.Layer(), fonts, [&]( tmx_object *pObject, tmx_layer *pLayer, float fOpacity )  {
            CHECK( pLayer == objects.Layer() );
            tileObjects.push_back( { pObject, fOpacity } );
        } );

        // "box" and "invalid" are rectangles: four edges each. "hidden" is not drawn.
        CHECK( EventsOf( fixture.engine, Event :: LINE ).size() == 8 );

        // "label" (Sans) and "orphan" (the placeholder); "far" is off screen. libtmx keeps an object
        // group's objects in reverse file order (the list's head is the last object in the file), and
        // the layer is drawn in that list order - so "orphan" comes before "label".
        std :: vector<Event>  texts = EventsOf( fixture.engine, Event :: TEXT );

        REQUIRE( texts.size() == 2 );
        CHECK( texts[0].text == "??????" );
        CHECK( texts[1].text == "Hello" );

        REQUIRE( tileObjects.size() == 1 );
        CHECK( tileObjects[0].first == objects.Object( "tile" ) );
        CHECK( tileObjects[0].second == 0.25f );
    }

    TEST_CASE( "A shape object's colour carries its own opacity" )  {

        MockEngineFixture   fixture;
        ParsedObjects       objects;
        FontRegistry        fonts;
        ObjectRenderer      renderer;

        objects.Layer() -> opacity = 0.5;
        renderer.Draw( PlainContext(), objects.Layer(), fonts, NO_TILE_OBJECTS );

        std :: vector<Event>  lines = EventsOf( fixture.engine, Event :: LINE );

        // The group's colour (opaque) times the layer's opacity, 0.5. (A group with no colour of its
        // own gets libtmx's zeroed default: alpha 0.)
        REQUIRE( !lines.empty() );
        CHECK( lines[0].color.nAlpha == ( uint8_t ) ( 0xFF * 0.5f ) );
    }
}
