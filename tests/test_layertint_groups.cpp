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
 * I8 of the master plan: a group layer's opacity and tint reach everything inside it, as Tiled
 * draws a group - opacities multiply down the nesting, tints multiply channel by channel. Tile
 * layers, image layers and tile objects take both; shapes and text take only the opacity. An
 * image layer applies its own opacity too (it was always drawn opaque before). And a tint or an
 * opacity set at runtime with SetLayer - on any layer type, groups included - shows from the
 * next frame.
 */

#include <doctest/doctest.h>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    // The extra attributes of each layer of the map below (a tintcolor, an opacity), "" for none.
    struct Attrs  {
        std :: string  ground, sky, things, fog, marks, deep;
        std :: string  rectangle;   // XML inside the rectangle object of "marks" (its properties)
    };

    // In draw order, the TILE events of one frame:
    enum TileEvent  {
        GROUND_0, GROUND_1,     // "ground", a 2 x 1 tile layer
        SKY,                    // "sky", an image layer
        THING,                  // a tile object of "things"
        INNER_0, INNER_1,       // "inner", a tile layer inside the group "fog"
        DEEPEST_0, DEEPEST_1,   // "deepest", inside the group "deep", itself inside "fog"
        AFTER_0, AFTER_1,       // "after", the tile layer after the groups
        TILE_EVENT_COUNT
    };

    // The rectangle in "marks" (inside "fog") is drawn as this many hairline LINE events.
    const size_t  __RECTANGLE_LINES = 4;

    Bytes MakeTmx( const Attrs& attrs )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"1\""
            << " tilewidth=\"32\" tileheight=\"32\" nextlayerid=\"10\" nextobjectid=\"4\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"32\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"1\" " << attrs.ground << "><data encoding=\"csv\">1,1</data></layer>"
            << "<imagelayer id=\"2\" name=\"sky\" " << attrs.sky << "><image source=\"sky.png\" width=\"40\" height=\"20\"/></imagelayer>"
            << "<objectgroup id=\"3\" name=\"things\" " << attrs.things << ">"
            << "<object id=\"1\" gid=\"1\" x=\"0\" y=\"128\" width=\"32\" height=\"32\"/></objectgroup>"
            << "<group id=\"4\" name=\"fog\" " << attrs.fog << ">"
            <<   "<layer id=\"5\" name=\"inner\" width=\"2\" height=\"1\"><data encoding=\"csv\">1,1</data></layer>"
            <<   "<objectgroup id=\"6\" name=\"marks\" color=\"#ff0000\" " << attrs.marks << ">"
            <<     "<object id=\"2\" x=\"10\" y=\"200\" width=\"16\" height=\"16\">" << attrs.rectangle << "</object>"
            <<     "<object id=\"3\" x=\"10\" y=\"250\" width=\"200\" height=\"40\">"
            <<       "<text fontfamily=\"Sans\" pixelsize=\"20\" color=\"#00ff00\">Hi</text></object>"
            <<   "</objectgroup>"
            <<   "<group id=\"7\" name=\"deep\" " << attrs.deep << ">"
            <<     "<layer id=\"8\" name=\"deepest\" width=\"2\" height=\"1\"><data encoding=\"csv\">1,1</data></layer>"
            <<   "</group>"
            << "</group>"
            << "<layer id=\"9\" name=\"after\" width=\"2\" height=\"1\"><data encoding=\"csv\">1,1</data></layer>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( const Attrs& attrs )  {
            fsFixture.fs.files["maps/groups.tmx"] = MakeTmx( attrs );

            RendererConfig                     config;
            SunLight :: Base :: stDimension2D  viewport {};

            viewport.size.nWidth  = 1000;
            viewport.size.nHeight = 800;
            config.fWidth         = 1260.0f;
            config.fHeight        = 920.0f;
            config.viewport       = viewport;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/groups.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            REQUIRE( pRenderer -> RegisterFont( "Sans", pRenderer -> LoadFont( "fonts/sans.ttf" ), false, false ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        void RunFrame( void )  {
            engineFixture.engine.events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = 1;
            pRenderer -> Run();
        }

        std :: vector<Event> Of( Event :: Kind kind ) const  {
            std :: vector<Event>  found;

            for( const Event& evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }

        // One frame's TILE events, checked to be the map's TILE_EVENT_COUNT.
        std :: vector<Event> Tiles( void )  {
            RunFrame();

            std :: vector<Event>  tiles = Of( Event :: TILE );

            REQUIRE( tiles.size() == TILE_EVENT_COUNT );

            return tiles;
        }

        // Reads a layer, lets the test change it, and writes it back.
        template<typename Change> void Set( const char* szName, Change change )  {
            SunLight :: TileMap :: stLayer  layer {};

            REQUIRE( pRenderer -> GetLayer( szName, layer ) == true );
            change( layer );
            REQUIRE( pRenderer -> SetLayer( szName, layer ) == true );
        }
    };

    void CheckColor( const SunLight :: Base :: stColor& color, int nRed, int nGreen, int nBlue, int nAlpha )  {
        CHECK( color.nRed   == nRed );
        CHECK( color.nGreen == nGreen );
        CHECK( color.nBlue  == nBlue );
        CHECK( color.nAlpha == nAlpha );
    }
}

TEST_SUITE( "renderer/layer_tint_groups" )  {

    TEST_CASE( "With no tint or opacity anywhere, everything draws white and opaque" )  {

        Scene                 scene( Attrs {} );
        std :: vector<Event>  tiles = scene.Tiles();

        for( size_t i = 0; i < tiles.size(); i++ )  {
            INFO( "tile event " << i );
            CheckColor( tiles[i].color, 0xFF, 0xFF, 0xFF, 0xFF );
        }
    }

    TEST_CASE( "A group's opacity and tint reach the tile layers inside it, and nothing outside it" )  {

        Attrs  attrs;

        attrs.fog = "opacity=\"0.5\" tintcolor=\"#ff8000\"";

        Scene                 scene( attrs );
        std :: vector<Event>  tiles = scene.Tiles();

        CheckColor( tiles[INNER_0].color,   0xFF, 0x80, 0x00, 127 );   // 255 x 0.5, truncated
        CheckColor( tiles[INNER_1].color,   0xFF, 0x80, 0x00, 127 );
        CheckColor( tiles[DEEPEST_0].color, 0xFF, 0x80, 0x00, 127 );   // "deep" adds nothing of its own
        CheckColor( tiles[GROUND_0].color,  0xFF, 0xFF, 0xFF, 0xFF );   // before the group: untouched
        CheckColor( tiles[AFTER_0].color,   0xFF, 0xFF, 0xFF, 0xFF );   // after it: nothing left over
        CheckColor( tiles[AFTER_1].color,   0xFF, 0xFF, 0xFF, 0xFF );
    }

    TEST_CASE( "Nested groups multiply: opacities together, tints channel by channel" )  {

        Attrs  attrs;

        attrs.fog  = "opacity=\"0.5\" tintcolor=\"#ff8000\"";
        attrs.deep = "opacity=\"0.5\" tintcolor=\"#8080ff\"";

        Scene                 scene( attrs );
        std :: vector<Event>  tiles = scene.Tiles();

        // Opacity 0.5 x 0.5 = 0.25, 255 x 0.25 = 63.75, truncated. Tint (c x t + 127) / 255:
        // red 255 x 128 -> 128, green 128 x 128 -> 64, blue 0 x 255 -> 0.
        CheckColor( tiles[DEEPEST_0].color, 0x80, 0x40, 0x00, 63 );
        CheckColor( tiles[DEEPEST_1].color, 0x80, 0x40, 0x00, 63 );
        CheckColor( tiles[INNER_0].color,   0xFF, 0x80, 0x00, 127 );   // only "fog" above it
    }

    TEST_CASE( "Shapes and text in a group take its opacity but not its tint" )  {

        Attrs  attrs;

        attrs.fog = "opacity=\"0.5\" tintcolor=\"#0000ff\"";

        Scene  scene( attrs );

        scene.RunFrame();

        std :: vector<Event>  lines = scene.Of( Event :: LINE );
        std :: vector<Event>  texts = scene.Of( Event :: TEXT );

        REQUIRE( lines.size() == __RECTANGLE_LINES );
        REQUIRE( texts.size() == 1 );

        for( const Event& line : lines )
            CheckColor( line.color, 0xFF, 0x00, 0x00, 127 );   // "marks"' own red, the group's opacity

        CheckColor( texts[0].color, 0x00, 0xFF, 0x00, 127 );   // the text's own green, the group's opacity
    }

    TEST_CASE( "An object's own opacity replaces its layer's, and its groups' still multiplies on top" )  {

        Attrs  attrs;

        attrs.fog       = "opacity=\"0.5\"";
        attrs.marks     = "opacity=\"0.2\"";
        attrs.rectangle = "<properties><property name=\"opacity\" type=\"float\" value=\"1.0\"/></properties>";

        Scene  scene( attrs );

        scene.RunFrame();

        std :: vector<Event>  lines = scene.Of( Event :: LINE );
        std :: vector<Event>  texts = scene.Of( Event :: TEXT );

        REQUIRE( lines.size() == __RECTANGLE_LINES );
        REQUIRE( texts.size() == 1 );

        for( const Event& line : lines )
            CHECK( line.color.nAlpha == 127 );   // its own 1.0 replaces "marks"' 0.2: 255 x 1.0 x 0.5 = 127.5, truncated

        CHECK( texts[0].color.nAlpha == 25 );   // no property: 255 x 0.2 x 0.5 = 25.5, truncated
    }

    TEST_CASE( "An image layer applies its own opacity" )  {

        Attrs  attrs;

        attrs.sky = "opacity=\"0.5\"";

        Scene                 scene( attrs );
        std :: vector<Event>  tiles = scene.Tiles();

        CheckColor( tiles[SKY].color, 0xFF, 0xFF, 0xFF, 127 );   // drawn opaque (255) before I8
    }

    TEST_CASE( "A tint set with SetLayer shows from the next frame on an image layer and a tile object" )  {

        Scene  scene( Attrs {} );

        scene.Set( "sky",    []( SunLight :: TileMap :: stLayer& layer )  { layer.tint = { 0x40, 0x80, 0xC0, 0xFF }; } );
        scene.Set( "things", []( SunLight :: TileMap :: stLayer& layer )  { layer.tint = { 0xC0, 0x80, 0x40, 0xFF }; } );

        std :: vector<Event>  tiles = scene.Tiles();

        CheckColor( tiles[SKY].color,   0x40, 0x80, 0xC0, 0xFF );
        CheckColor( tiles[THING].color, 0xC0, 0x80, 0x40, 0xFF );
    }

    TEST_CASE( "A group's tint and opacity set with SetLayer show from the next frame" )  {

        const int  __GROUP_OPACITY = 51;   // of 255: written back as 0.2, drawn as 255 x 0.2 = 51

        Scene  scene( Attrs {} );

        scene.Set( "fog", [&]( SunLight :: TileMap :: stLayer& layer )  {
            layer.tint     = { 0x00, 0xFF, 0x80, 0xFF };
            layer.nOpacity = __GROUP_OPACITY;
        } );

        std :: vector<Event>  tiles = scene.Tiles();

        CheckColor( tiles[INNER_0].color,   0x00, 0xFF, 0x80, __GROUP_OPACITY );
        CheckColor( tiles[DEEPEST_1].color, 0x00, 0xFF, 0x80, __GROUP_OPACITY );
        CheckColor( tiles[AFTER_0].color,   0xFF, 0xFF, 0xFF, 0xFF );

        for( const Event& line : scene.Of( Event :: LINE ) )
            CheckColor( line.color, 0xFF, 0x00, 0x00, __GROUP_OPACITY );   // the opacity, not the tint
    }

    TEST_CASE( "GetLayer reads a group's tint" )  {

        Attrs  attrs;

        attrs.fog = "tintcolor=\"#c0ff8000\"";

        Scene                           scene( attrs );
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( 4, layer ) == true );
        CheckColor( layer.tint, 0xFF, 0x80, 0x00, 0xC0 );
    }
}
