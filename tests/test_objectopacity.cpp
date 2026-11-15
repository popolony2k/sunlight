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
 * L2 of the master plan: an object's own custom "opacity" property (0 to 1), when it has a usable
 * one, overrides its layer's own opacity for that object alone; otherwise the object uses its
 * layer's opacity, exactly as before this item existed (Tiled objects have no opacity field of
 * their own - a custom property is the only way to set one per object). An unusable property
 * (wrong type, or outside 0 to 1) falls back to the layer's opacity too, the same as having none.
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

    // A tileset of one 16 x 16 tile (gid 1), and a map holding one object group - its own color and
    // opacity given directly, so a test's expected alpha is never at the mercy of libtmx's own
    // zero-filled default (an <objectgroup> with no color attribute parses to alpha 0, fully
    // transparent - useless as an opacity baseline).
    Bytes MakeTmx( const std :: string& strGroupAttrs, const std :: string& strObjects )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"60\" height=\"40\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\" nextobjectid=\"10\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\">"
            << "<image source=\"tiles.png\" width=\"16\" height=\"16\"/></tileset>"
            << "<objectgroup id=\"1\" name=\"objs\" " << strGroupAttrs << ">" << strObjects << "</objectgroup>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // Renders one frame of the map. The viewport is at (10, 10), so an object at (x, y) starts at screen (10 + x, 10 + y).
    struct Frame  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Frame( const std :: string& strGroupAttrs, const std :: string& strObjects )  {

            fsFixture.fs.files["maps/opacity.tmx"] = MakeTmx( strGroupAttrs, strObjects );

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
            REQUIRE( pRenderer -> LoadMap( "maps/opacity.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            REQUIRE( pRenderer -> RegisterFont( "Sans", pRenderer -> LoadFont( "fonts/sans.ttf" ), false, false ) == true );
            pRenderer -> Run();
        }

        ~Frame( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        std :: vector<Event>  Of( Event :: Kind kind ) const  {
            std :: vector<Event>  found;

            for( const Event& evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }
    };

    // A hairline rectangle (width 1, the default): drawn as four engine LINE calls, each carrying
    // the object's own effective colour, including its opacity-derived alpha.
    std :: string RectObject( const std :: string& strPropertyXml = "" )  {
        return "<object id=\"1\" x=\"100\" y=\"100\" width=\"16\" height=\"16\">" + strPropertyXml + "</object>";
    }

    std :: string OpacityProperty( const std :: string& strType, const std :: string& strValue )  {
        return "<properties><property name=\"opacity\" type=\"" + strType + "\" value=\"" + strValue + "\"/></properties>";
    }
}

TEST_SUITE( "renderer/object_opacity" )  {

    TEST_CASE( "With no opacity property, a shape object's alpha equals its layer's opacity" )  {

        // Group colour opaque white (alpha 0xFF); layer opacity 0.5 halves it: (uint8_t)(255 * 0.5) = 127.
        Frame  frame( "color=\"#ffffff\" opacity=\"0.5\"", RectObject() );

        std :: vector<Event>  lines = frame.Of( Event :: LINE );

        REQUIRE( lines.size() == 4 );   // four hairline edges

        for( const Event& line : lines )
            CHECK( line.color.nAlpha == 127 );
    }

    TEST_CASE( "An object's own opacity property overrides its layer's, multiplying into the drawn colour's alpha" )  {

        // Group colour alpha 0x80 (128, via the 8-digit #AARRGGBB form); the object's own 0.5
        // property must MULTIPLY into that, not replace it: (uint8_t)(128 * 0.5) = 64. A
        // replace-not-multiply mutant would give 127 (0xFF * 0.5) instead, and get caught here.
        // The layer's own opacity (1.0, left at its default) would leave alpha at 128 unchanged
        // if the object's override were ignored - also distinct from the expected 64.
        Frame  frame( "color=\"#80ffffff\"", RectObject( OpacityProperty( "float", "0.5" ) ) );

        std :: vector<Event>  lines = frame.Of( Event :: LINE );

        REQUIRE( lines.size() == 4 );

        for( const Event& line : lines )
            CHECK( line.color.nAlpha == 64 );
    }

    TEST_CASE( "An opacity property outside 0 to 1 is invalid: falls back to the layer's opacity" )  {

        Frame  frame( "color=\"#ffffff\" opacity=\"0.5\"", RectObject( OpacityProperty( "float", "2.5" ) ) );

        std :: vector<Event>  lines = frame.Of( Event :: LINE );

        REQUIRE( lines.size() == 4 );

        for( const Event& line : lines )
            CHECK( line.color.nAlpha == 127 );   // the layer's own 0.5, not the invalid 2.5
    }

    TEST_CASE( "An opacity property of the wrong type is invalid: falls back to the layer's opacity" )  {

        Frame  frame( "color=\"#ffffff\" opacity=\"0.5\"", RectObject( OpacityProperty( "string", "half" ) ) );

        std :: vector<Event>  lines = frame.Of( Event :: LINE );

        REQUIRE( lines.size() == 4 );

        for( const Event& line : lines )
            CHECK( line.color.nAlpha == 127 );
    }

    TEST_CASE( "A text object follows the same rule as a shape object" )  {

        Frame  frame( "",
                      "<object id=\"1\" x=\"100\" y=\"100\" width=\"200\" height=\"40\">"
                      "<properties><property name=\"opacity\" type=\"float\" value=\"0.5\"/></properties>"
                      "<text fontfamily=\"Sans\" pixelsize=\"20\" color=\"#ffffff\">Hi</text></object>" );

        std :: vector<Event>  texts = frame.Of( Event :: TEXT );

        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].color.nAlpha == 127 );
    }

    TEST_CASE( "A tile object follows the same rule: its own opacity property overrides the layer's" )  {

        Frame  frame( "opacity=\"1.0\"",
                      "<object id=\"1\" gid=\"1\" x=\"100\" y=\"116\" width=\"16\" height=\"16\">"
                      "<properties><property name=\"opacity\" type=\"float\" value=\"0.5\"/></properties></object>" );

        std :: vector<Event>  tiles = frame.Of( Event :: TILE );

        REQUIRE( tiles.size() == 1 );
        CHECK( tiles[0].color.nAlpha == 127 );   // the object's own 0.5, not the layer's 1.0
    }

    TEST_CASE( "A tile object with no opacity property still uses its layer's, as before L2" )  {

        Frame  frame( "opacity=\"0.5\"",
                      "<object id=\"1\" gid=\"1\" x=\"100\" y=\"116\" width=\"16\" height=\"16\"/>" );

        std :: vector<Event>  tiles = frame.Of( Event :: TILE );

        REQUIRE( tiles.size() == 1 );
        CHECK( tiles[0].color.nAlpha == 127 );
    }
}
