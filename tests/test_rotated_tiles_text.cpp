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

    // A tileset of two 16 x 16 tiles (gids 1 and 2), and a map holding one object group with the given objects.
    Bytes MakeTmx( const std :: string &strObjects )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"60\" height=\"40\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\" nextobjectid=\"10\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"2\" columns=\"2\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"16\"/></tileset>"
            << "<objectgroup id=\"1\" name=\"sprites\">" << strObjects << "</objectgroup>"
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

        explicit Frame( const std :: string &strObjects )  {

            fsFixture.fs.files["maps/rotated.tmx"] = MakeTmx( strObjects );

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
            REQUIRE( pRenderer -> LoadMap( "maps/rotated.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            REQUIRE( pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );
            pRenderer -> Run();
        }

        ~Frame( void )  {
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
}

TEST_SUITE( "Rotated tiles and text" )  {

    TEST_CASE( "A tile turned 90 degrees turns about its bottom-left corner: the anchor is the object's point" )  {

        // The tile at object (100, 100), 16 x 16, turned 90 degrees. Its anchor is that point on screen, (110, 110), and
        // the destination's top-left sits one height above it, so the origin is (0, 16).
        Frame  frame( "<object id=\"1\" gid=\"1\" x=\"100\" y=\"100\" width=\"16\" height=\"16\" rotation=\"90\"/>" );

        std :: vector<Event>  tiles = frame.Of( Event :: TILE_ROTATED );

        REQUIRE( tiles.size() == 1 );
        CHECK( tiles[0].x == 110.0f );  CHECK( tiles[0].y == 110.0f );
        CHECK( tiles[0].w == 16.0f );   CHECK( tiles[0].h == 16.0f );
        CHECK( tiles[0].scale == 90.0f );
        CHECK( tiles[0].srcX == 0.0f );   CHECK( tiles[0].srcY == 16.0f );
        CHECK( frame.Of( Event :: TILE ).empty() );
    }

    TEST_CASE( "A text block turned 90 degrees keeps its first line at the pivot and swings its second line to the left" )  {

        // Two lines at pixel size 20, turned 90 degrees about the block's top-left corner at screen (110, 110). The
        // second line starts 20 pixels below the first, which after the turn is 20 pixels to the left: (90, 110).
        Frame  frame( "<object id=\"1\" x=\"100\" y=\"100\" width=\"200\" height=\"60\" rotation=\"90\">"
                      "<text fontfamily=\"Sans\" pixelsize=\"20\">A\nB</text></object>" );

        std :: vector<Event>  lines = frame.Of( Event :: TEXT_ROTATED );

        REQUIRE( lines.size() == 2 );
        CHECK( lines[0].x == 110.0f );  CHECK( lines[0].y == 110.0f );
        CHECK( lines[1].x == 90.0f );   CHECK( lines[1].y == 110.0f );
        CHECK( lines[0].scale == 90.0f );  CHECK( lines[1].scale == 90.0f );
        CHECK( lines[0].nFontSize == 20 );
        CHECK( frame.Of( Event :: TEXT ).empty() );
    }

    TEST_CASE( "A text block turned into view is drawn, though its unturned box is entirely outside the viewport" )  {

        // The box at object x 1030 is outside the viewport's right edge (at 1010) unturned. Turned 90 degrees about its
        // top-left corner, it hangs to the left of that corner, so it is inside the view and must be drawn.
        Frame  frame( "<object id=\"1\" x=\"1030\" y=\"100\" width=\"200\" height=\"60\" rotation=\"90\">"
                      "<text fontfamily=\"Sans\" pixelsize=\"20\">A</text></object>" );

        std :: vector<Event>  lines = frame.Of( Event :: TEXT_ROTATED );

        REQUIRE( lines.size() == 1 );
        CHECK( lines[0].x == 1040.0f );
        CHECK( lines[0].y == 110.0f );
    }
}
