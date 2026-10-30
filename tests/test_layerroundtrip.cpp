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
 * I9 of the master plan: a layer's opacity is a float, 0..1, as in Tiled and libtmx, and a
 * GetLayer followed by an unchanged SetLayer leaves the map's own values exactly as authored -
 * opacity and parallax alike (a float cannot hold every double Tiled writes, so SetLayer writes
 * one only when it changed). With the old int 0..255 opacity, an authored 0.5 came back as
 * 128 / 255 = 0.502 and was drawn with alpha 128 instead of 127.
 */

#include <doctest/doctest.h>
#include <cmath>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    // Three 1 x 1 tile layers: "half" (opacity 0.5), "odd" (opacity 0.69, a value no float holds
    // exactly) and "far" (parallax 0.3, another).
    const char * const  __TMX =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"1\" height=\"1\""
        " tilewidth=\"32\" tileheight=\"32\" nextlayerid=\"4\">"
        "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
        "<image source=\"tiles.png\" width=\"32\" height=\"32\"/></tileset>"
        "<layer id=\"1\" name=\"half\" width=\"1\" height=\"1\" opacity=\"0.5\"><data encoding=\"csv\">1</data></layer>"
        "<layer id=\"2\" name=\"odd\" width=\"1\" height=\"1\" opacity=\"0.69\"><data encoding=\"csv\">1</data></layer>"
        "<layer id=\"3\" name=\"far\" width=\"1\" height=\"1\" parallaxx=\"0.3\" parallaxy=\"0.3\"><data encoding=\"csv\">1</data></layer>"
        "</map>";

    // The TILE event each layer draws, in draw order.
    enum LayerTile  {
        HALF_TILE,
        ODD_TILE,
        FAR_TILE,
        LAYER_TILE_COUNT
    };

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            std :: string  str( __TMX );

            fsFixture.fs.files["maps/roundtrip.tmx"] = Bytes( str.begin(), str.end() );

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
            REQUIRE( pRenderer -> LoadMap( "maps/roundtrip.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        // libtmx's own layer.
        tmx_layer* Tmx( const char* szName )  {
            SunLight :: TileMap :: stLayer  layer {};

            REQUIRE( pRenderer -> GetLayer( szName, layer ) == true );

            return layer.pLayer;
        }

        // GetLayer, then SetLayer with nothing changed.
        void WriteBackUnchanged( const char* szName )  {
            SunLight :: TileMap :: stLayer  layer {};

            REQUIRE( pRenderer -> GetLayer( szName, layer ) == true );
            REQUIRE( pRenderer -> SetLayer( szName, layer ) == true );
        }

        // The alpha each layer's tile is drawn with in one frame.
        std :: vector<int> Alphas( void )  {
            engineFixture.engine.events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = 1;
            pRenderer -> Run();

            std :: vector<int>  alphas;

            for( const Event& evt : engineFixture.engine.events )
                if( evt.kind == Event :: TILE )
                    alphas.push_back( evt.color.nAlpha );

            REQUIRE( alphas.size() == LAYER_TILE_COUNT );

            return alphas;
        }
    };
}

TEST_SUITE( "renderer/layer_roundtrip" )  {

    TEST_CASE( "An unchanged write-back keeps an authored opacity exactly, and its drawn alpha" )  {

        Scene  scene;

        CHECK( scene.Alphas()[HALF_TILE] == 127 );   // 255 x 0.5 = 127.5, truncated

        scene.WriteBackUnchanged( "half" );
        CHECK( scene.Tmx( "half" ) -> opacity == 0.5 );
        CHECK( scene.Alphas()[HALF_TILE] == 127 );   // 128 with the old 0..255 opacity
    }

    TEST_CASE( "An unchanged write-back keeps an opacity no float holds exactly" )  {

        Scene   scene;
        double  fAuthored = scene.Tmx( "odd" ) -> opacity;

        REQUIRE( fAuthored == 0.69 );
        scene.WriteBackUnchanged( "odd" );
        CHECK( scene.Tmx( "odd" ) -> opacity == fAuthored );
    }

    TEST_CASE( "An unchanged write-back keeps an authored parallax exactly" )  {

        Scene   scene;
        double  fAuthored = scene.Tmx( "far" ) -> parallaxx;

        REQUIRE( fAuthored == 0.3 );
        scene.WriteBackUnchanged( "far" );
        CHECK( scene.Tmx( "far" ) -> parallaxx == fAuthored );
        CHECK( scene.Tmx( "far" ) -> parallaxy == fAuthored );
    }

    TEST_CASE( "GetLayer reads the opacity as authored, 0..1" )  {

        Scene                           scene;
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( "half", layer ) == true );
        CHECK( layer.fOpacity == 0.5f );
        REQUIRE( scene.pRenderer -> GetLayer( "odd", layer ) == true );
        CHECK( layer.fOpacity == 0.69f );
        REQUIRE( scene.pRenderer -> GetLayer( "far", layer ) == true );
        CHECK( layer.fOpacity == 1.0f );   // none authored
    }

    TEST_CASE( "A changed opacity or parallax is written, and drawn from the next frame" )  {

        Scene                           scene;
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( "half", layer ) == true );
        layer.fOpacity = 0.25f;
        REQUIRE( scene.pRenderer -> SetLayer( "half", layer ) == true );
        CHECK( scene.Tmx( "half" ) -> opacity == 0.25 );
        CHECK( scene.Alphas()[HALF_TILE] == 63 );   // 255 x 0.25 = 63.75, truncated

        REQUIRE( scene.pRenderer -> GetLayer( "far", layer ) == true );
        layer.fParallaxX = 0.5f;
        REQUIRE( scene.pRenderer -> SetLayer( "far", layer ) == true );
        CHECK( scene.Tmx( "far" ) -> parallaxx == 0.5 );
        CHECK( scene.Tmx( "far" ) -> parallaxy == 0.3 );   // left alone
    }

    TEST_CASE( "A NaN opacity leaves the layer's own" )  {

        Scene                           scene;
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.pRenderer -> GetLayer( "half", layer ) == true );
        layer.fOpacity = std :: nanf( "" );
        REQUIRE( scene.pRenderer -> SetLayer( "half", layer ) == true );
        CHECK( scene.Tmx( "half" ) -> opacity == 0.5 );
    }
}
