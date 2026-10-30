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
 * A Tiled layer's own "tintcolor" property (already read by libtmx into tmx_layer::tintcolor,
 * an ARGB uint32_t, defaulting to opaque white when the TMX does not declare one - never
 * applied anywhere by this renderer until now) multiplies into every tile that layer draws -
 * tile layers, image layers, and tile objects alike. Its own alpha channel is deliberately
 * ignored: layer opacity (the existing, separate "opacity" attribute) is still the only alpha
 * control, exactly as it was before tintcolor existed.
 *
 * I8 of the master plan: stLayer::tint reads the authored tintcolor (alpha included) through
 * GetLayer and writes it back through SetLayer. Groups and runtime changes on every layer type
 * are in test_layertint_groups.cpp.
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

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: Base :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // One 2x1 tile layer, "tinted" - its own tintcolor is substituted per test via a
    // placeholder, and one 2x1 "untinted" layer with no tintcolor attribute at all (the
    // default-omitted case).
    Bytes MakeLayerTintTmx( const char* szTintAttr )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"1\""
            << " tilewidth=\"32\" tileheight=\"32\" nextlayerid=\"3\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"32\"/></tileset>"
            << "<layer id=\"1\" name=\"tinted\" width=\"2\" height=\"1\"" << szTintAttr << ">"
            << "<data encoding=\"csv\">1,1</data></layer>"
            << "<layer id=\"2\" name=\"plain\" width=\"2\" height=\"1\">"
            << "<data encoding=\"csv\">1,1</data></layer>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( const char* szTintAttr )  {
            fsFixture.fs.files["maps/tint.tmx"] = MakeLayerTintTmx( szTintAttr );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 0, 0, 2000, 2000 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/tint.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // libtmx's own packed ARGB value of a layer, as SetLayer left it.
        uint32_t RawTint( int nLayerId )  {
            SunLight :: TileMap :: stLayer  layer {};

            REQUIRE( pRenderer -> GetLayer( nLayerId, layer ) == true );

            return layer.pLayer -> tintcolor;
        }

        std :: vector<Event> TileEvents( void )  {
            std :: vector<Event>  tiles;

            for( const Event& evt : engine().events )
                if( evt.kind == Event :: TILE )
                    tiles.push_back( evt );

            return tiles;
        }
    };
}

TEST_SUITE( "renderer/layer_tint" )  {

    TEST_CASE( "A layer with no tintcolor draws with the untinted white default" )  {

        Scene  scene( "" );   // no tintcolor attribute at all

        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.TileEvents();

        REQUIRE( tiles.size() == 4 );   // both layers, 2 tiles each

        for( const Event& tile : tiles )  {
            CHECK( tile.color.nRed   == 0xFF );
            CHECK( tile.color.nGreen == 0xFF );
            CHECK( tile.color.nBlue  == 0xFF );
            CHECK( tile.color.nAlpha == 0xFF );
        }
    }

    TEST_CASE( "A layer's own tintcolor multiplies into its own tiles, and no others" )  {

        Scene  scene( " tintcolor=\"#ff8000\"" );   // opaque orange, on the "tinted" layer only

        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.TileEvents();

        REQUIRE( tiles.size() == 4 );

        // "tinted" (layer id 1) draws first - its own 2 tiles carry the tint.
        for( size_t i = 0; i < 2; i++ )  {
            INFO( "tinted tile " << i );
            CHECK( tiles[i].color.nRed   == 0xFF );
            CHECK( tiles[i].color.nGreen == 0x80 );
            CHECK( tiles[i].color.nBlue  == 0x00 );
            CHECK( tiles[i].color.nAlpha == 0xFF );
        }

        // "plain" (layer id 2, no tintcolor of its own) stays untinted white.
        for( size_t i = 2; i < 4; i++ )  {
            INFO( "plain tile " << i );
            CHECK( tiles[i].color.nRed   == 0xFF );
            CHECK( tiles[i].color.nGreen == 0xFF );
            CHECK( tiles[i].color.nBlue  == 0xFF );
        }
    }

    TEST_CASE( "A tintcolor's own alpha channel is ignored - layer opacity is still the only alpha control" )  {

        // #80ff8000: alpha byte 0x80 (half) in the tintcolor itself - must NOT affect the
        // drawn alpha, which should still come from the layer's own opacity (1.0 here, since
        // none is declared) instead.
        Scene  scene( " tintcolor=\"#80ff8000\"" );

        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.TileEvents();

        REQUIRE( tiles.size() == 4 );

        for( size_t i = 0; i < 2; i++ )  {
            CHECK( tiles[i].color.nRed   == 0xFF );
            CHECK( tiles[i].color.nGreen == 0x80 );
            CHECK( tiles[i].color.nBlue  == 0x00 );
            CHECK( tiles[i].color.nAlpha == 0xFF );    // from opacity (1.0), NOT the tint's own 0x80
        }
    }

    TEST_CASE( "GetLayer reads a layer's authored tint, alpha included, by id and by name" )  {

        Scene  scene( " tintcolor=\"#80ff8000\"" );

        SunLight :: TileMap :: stLayer  byId {};
        SunLight :: TileMap :: stLayer  byName {};

        REQUIRE( scene.pRenderer -> GetLayer( 1, byId ) == true );
        REQUIRE( scene.pRenderer -> GetLayer( "tinted", byName ) == true );

        for( const SunLight :: TileMap :: stLayer& layer : { byId, byName } )  {
            CHECK( layer.tint.nRed   == 0xFF );
            CHECK( layer.tint.nGreen == 0x80 );
            CHECK( layer.tint.nBlue  == 0x00 );
            CHECK( layer.tint.nAlpha == 0x80 );
        }
    }

    TEST_CASE( "GetLayer reads opaque white for a layer with no tintcolor" )  {

        Scene  scene( " tintcolor=\"#ff8000\"" );

        SunLight :: TileMap :: stLayer  layer {};

        layer.tint = SunLight :: Base :: stColor { 0, 0, 0, 0 };   // so a GetLayer that leaves it alone shows
        REQUIRE( scene.pRenderer -> GetLayer( "plain", layer ) == true );
        CHECK( layer.tint.nRed   == 0xFF );
        CHECK( layer.tint.nGreen == 0xFF );
        CHECK( layer.tint.nBlue  == 0xFF );
        CHECK( layer.tint.nAlpha == 0xFF );
    }

    TEST_CASE( "SetLayer writes the tint back as libtmx's ARGB, alpha included" )  {

        Scene  scene( "" );

        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( 1, layer ) == true );
        layer.tint = SunLight :: Base :: stColor { 0x12, 0x34, 0x56, 0x78 };
        REQUIRE( scene.pRenderer -> SetLayer( 1, layer ) == true );
        CHECK( scene.RawTint( 1 ) == 0x78123456u );

        SunLight :: TileMap :: stLayer  readBack {};

        REQUIRE( scene.pRenderer -> GetLayer( "tinted", readBack ) == true );
        CHECK( readBack.tint.nRed   == 0x12 );
        CHECK( readBack.tint.nGreen == 0x34 );
        CHECK( readBack.tint.nBlue  == 0x56 );
        CHECK( readBack.tint.nAlpha == 0x78 );
    }

    TEST_CASE( "A tint set with SetLayer shows from the next frame, its alpha still ignored" )  {

        Scene  scene( "" );

        scene.RunFrames( 1 );
        REQUIRE( scene.TileEvents().size() == 4 );
        CHECK( scene.TileEvents()[0].color.nGreen == 0xFF );   // untinted before

        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( "tinted", layer ) == true );
        layer.tint = SunLight :: Base :: stColor { 0x00, 0x80, 0xFF, 0x10 };
        REQUIRE( scene.pRenderer -> SetLayer( "tinted", layer ) == true );
        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.TileEvents();

        REQUIRE( tiles.size() == 4 );

        for( size_t i = 0; i < 2; i++ )  {
            INFO( "tinted tile " << i );
            CHECK( tiles[i].color.nRed   == 0x00 );
            CHECK( tiles[i].color.nGreen == 0x80 );
            CHECK( tiles[i].color.nBlue  == 0xFF );
            CHECK( tiles[i].color.nAlpha == 0xFF );   // the opacity's, not the tint's 0x10
        }

        CHECK( tiles[2].color.nRed == 0xFF );   // the other layer untouched
        CHECK( tiles[2].color.nBlue == 0xFF );
    }

    TEST_CASE( "Writing back an unchanged layer leaves its tint exactly as authored" )  {

        Scene  scene( " tintcolor=\"#80ff8000\"" );

        uint32_t  nAuthored = scene.RawTint( 1 );

        scene.RunFrames( 1 );

        std :: vector<Event>  before = scene.TileEvents();
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( 1, layer ) == true );
        REQUIRE( scene.pRenderer -> SetLayer( 1, layer ) == true );
        CHECK( scene.RawTint( 1 ) == nAuthored );
        scene.RunFrames( 1 );

        std :: vector<Event>  after = scene.TileEvents();

        REQUIRE( after.size() == before.size() );

        for( size_t i = 0; i < after.size(); i++ )  {
            INFO( "tile " << i );
            CHECK( after[i].color.nRed   == before[i].color.nRed );
            CHECK( after[i].color.nGreen == before[i].color.nGreen );
            CHECK( after[i].color.nBlue  == before[i].color.nBlue );
            CHECK( after[i].color.nAlpha == before[i].color.nAlpha );
        }
    }
}
