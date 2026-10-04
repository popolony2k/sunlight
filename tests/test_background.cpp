/*
 * Background colours: the window area and the viewport can take different colours.
 * Until SetWindowBackgroundColor is called, both use the map's own backgroundcolor (the
 * behaviour every existing game has). After it is called, the window area takes that
 * colour and the default viewport is filled with the map's colour.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    // The map colour is ARGB: 0xFF102030 is R 0x10, G 0x20, B 0x30.
    const uint32_t  g_nMapColor    = 0xFF102030;
    const uint32_t  g_nWindowColor = 0xFF301010;

    Bytes MakeMap( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
            << " tilewidth=\"16\" tileheight=\"16\" backgroundcolor=\"#ff102030\"></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/bg.tmx"] = MakeMap();

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport.emplace();
            config.viewport -> pos.x        = 40;
            config.viewport -> pos.y        = 40;
            config.viewport -> size.nWidth  = 1000;
            config.viewport -> size.nHeight = 800;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/bg.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
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

        // The first CLEAR of the frame - the window clear, before anything is drawn.
        const Event *FirstClear( void )  {
            for( const Event &evt : engine().events )
                if( evt.kind == Event :: CLEAR )
                    return &evt;

            return nullptr;
        }

        const Event *FirstFill( void )  {
            for( const Event &evt : engine().events )
                if( evt.kind == Event :: FILL )
                    return &evt;

            return nullptr;
        }

        static bool SameColor( SunLight :: Base :: stColor c, uint32_t nArgb )  {
            return c.nAlpha == ( ( nArgb >> 24 ) & 0xFF ) && c.nRed == ( ( nArgb >> 16 ) & 0xFF ) &&
                   c.nGreen == ( ( nArgb >> 8 ) & 0xFF ) && c.nBlue == ( nArgb & 0xFF );
        }
    };
}

TEST_SUITE( "renderer/background colours" )  {

    TEST_CASE( "Without SetWindowBackgroundColor, the window and the viewport both take the map colour (unchanged)" )  {

        Scene  scene;

        scene.RunFrames( 1 );

        REQUIRE( scene.FirstClear() != nullptr );
        CHECK( Scene :: SameColor( scene.FirstClear() -> color, g_nMapColor ) );
        CHECK( scene.FirstFill() == nullptr );
    }

    TEST_CASE( "With SetWindowBackgroundColor, the window takes that colour and the viewport the map's" )  {

        Scene  scene;

        scene.pRenderer -> SetWindowBackgroundColor( g_nWindowColor );
        scene.RunFrames( 1 );

        REQUIRE( scene.FirstClear() != nullptr );
        CHECK( Scene :: SameColor( scene.FirstClear() -> color, g_nWindowColor ) );

        // The viewport is filled after the window clear, with the map's colour, over the viewport's rectangle.
        REQUIRE( scene.FirstFill() != nullptr );
        CHECK( Scene :: SameColor( scene.FirstFill() -> color, g_nMapColor ) );
        CHECK( scene.FirstFill() -> x == 40.0f );
        CHECK( scene.FirstFill() -> y == 40.0f );
        CHECK( scene.FirstFill() -> w == 1000.0f );
        CHECK( scene.FirstFill() -> h == 800.0f );
    }

    TEST_CASE( "With clearing turned off, nothing is cleared or filled" )  {

        Scene  scene;

        scene.pRenderer -> SetWindowBackgroundColor( g_nWindowColor );
        scene.pRenderer -> SetClearBackground( false );
        scene.RunFrames( 1 );

        CHECK( scene.FirstFill() == nullptr );
    }
}
