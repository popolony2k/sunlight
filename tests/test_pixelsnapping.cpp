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
 * A real, user-reported bug (found live in samples/sprite, not assumed): at a non-integer
 * zoom, two adjacent tiles' screen edges are mathematically contiguous (their difference is an
 * exact multiple of the tile's own zoomed width), but that shared edge commonly lands on a
 * fractional screen pixel - and two SEPARATELY issued draw calls sharing a fractional-pixel
 * edge can leave the rasterizer's coverage of that boundary pixel ambiguous, a visible seam of
 * the clear colour between every pair of tiles at most camera positions. The fix (LayerRenderer's
 * DrawTile and DrawImageLayer, TextureCanvas::Draw) rounds each destination rect to the nearest whole screen
 * pixel. Zoom position 60 (factor 3.8125 = 61/16) is the exact setting samples/sprite/world.cpp
 * uses by default (__DEFAULT_ZOOM_SCALE_POS) - the same setting the bug was first seen in.
 */

#include <doctest/doctest.h>
#include <cmath>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
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

    // 4x1 tiles, 32px each, one "ground" layer filling every cell - enough tiles in a single
    // row to check every adjacent pair, nothing more.
    Bytes MakePixelSnappingTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"1\""
            << " tilewidth=\"32\" tileheight=\"32\" nextlayerid=\"2\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"32\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"1\"><data encoding=\"csv\">1,1,1,1</data></layer>"
            << "<imagelayer id=\"2\" name=\"sky\"><image source=\"images/sky.png\" width=\"64\" height=\"32\"/></imagelayer>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/snap.tmx"] = MakePixelSnappingTmx();

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 0, 0, 2000, 2000 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/snap.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

            // The exact zoom position (and factor, 61/16 = 3.8125) samples/sprite/world.cpp
            // defaults to - where the real seam was first seen.
            pRenderer -> GetViewport().SetZoom( 60 );
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

        std :: vector<Event> TileEvents( void )  {
            std :: vector<Event>  tiles;

            for( const Event& evt : engine().events )
                if( evt.kind == Event :: TILE )
                    tiles.push_back( evt );

            return tiles;
        }
    };
}

TEST_SUITE( "renderer/pixel_snapping" )  {

    TEST_CASE( "At a non-integer zoom, every tile lands on a whole screen pixel and exactly abuts its neighbor" )  {

        Scene  scene;

        // A camera position that is NOT a multiple of 16 - at this zoom (61/16), that is
        // exactly the condition that puts the unrounded destination on a fractional pixel
        // (see this file's own header comment).
        scene.pRenderer -> SetCameraPosition( 1, 0 );
        scene.RunFrames( 1 );

        std :: vector<Event>  allTiles = scene.TileEvents();

        // "ground" (4 tiles) draws first, declared first; the "sky" image layer's own single
        // draw (a different, larger size) follows it - see this file's own map.
        REQUIRE( allTiles.size() == 5 );

        std :: vector<Event>  tiles( allTiles.begin(), allTiles.begin() + 4 );
        const Event           &image = allTiles[4];

        for( const Event& tile : tiles )  {
            CHECK( std :: floor( tile.x ) == tile.x );            // whole pixel, not fractional
            CHECK( std :: floor( tile.w ) == tile.w );
        }

        for( size_t i = 0; i + 1 < tiles.size(); i++ )  {
            INFO( "tile pair " << i << "/" << ( i + 1 ) );
            // No gap and no overlap: the next tile's left edge is exactly this one's right edge.
            CHECK( ( tiles[i].x + tiles[i].w ) == tiles[i + 1].x );
        }

        // The image layer's own draw (a third, separate call site with the identical fix)
        // lands on a whole pixel too.
        CHECK( std :: floor( image.x ) == image.x );
        CHECK( std :: floor( image.y ) == image.y );
    }

    TEST_CASE( "Tiles straddling screen x = 0 stay contiguous: round-half-up, never std::round's away-from-zero tie" )  {

        // A second, real bug found live (std::round still in place from the first fix): an
        // integer map position times zoom 3.8125 can land EXACTLY on a half-pixel (verified:
        // tile 0 at camera (8, 0) -> -30.5, tile 1 -> 91.5 - both exact .5 values, one
        // negative, one positive). std::round ties away from zero: round(-30.5) == -31 (more
        // negative) but round(91.5) == 92 (more positive) - the two tiles, otherwise exactly
        // contiguous (their difference is exactly the tile's own zoomed width, 122), round
        // AWAY from each other and open a real 1px gap. Found by sweeping every camera
        // position from -300 to 300 against this exact map/zoom and checking for a missing
        // column - not assumed from the formula alone.
        Scene  scene;

        scene.pRenderer -> SetCameraPosition( 8, 0 );
        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.TileEvents();

        REQUIRE( tiles.size() >= 2 );
        CHECK( tiles[0].x == -30.0f );             // RoundHalfUp(-30.5) == -30, not std::round's -31
        CHECK( tiles[1].x == 92.0f );               // RoundHalfUp(91.5) == 92
        CHECK( ( tiles[0].x + tiles[0].w ) == tiles[1].x );      // exactly contiguous, no gap
    }

    TEST_CASE( "A world-space sprite at a non-integer zoom also lands on a whole screen pixel" )  {

        Scene                                 scene;
        SunLight :: Sprite :: Sprite          sprite;
        SunLight :: Canvas :: TextureCanvas   canvas;

        scene.engine().hLoadTextureResult = ( SunLight :: Base :: TextureHandle ) 0x77;

        REQUIRE( canvas.Load( "sprite.png" ) == true );
        canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 1, 0 }, { 16, 16 } } );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetWorldSpace( true );

        REQUIRE( scene.pRenderer -> AddSprite( 1, sprite ) == true );

        scene.pRenderer -> SetCameraPosition( 1, 0 );
        scene.RunFrames( 1 );

        const Event  *pSprite = nullptr;

        for( const Event& evt : scene.engine().events )
            if( ( evt.kind == Event :: TILE ) && ( evt.handle == ( void * ) 0x77 ) )
                pSprite = &evt;

        REQUIRE( pSprite != nullptr );
        CHECK( std :: floor( pSprite -> x ) == pSprite -> x );
        CHECK( std :: floor( pSprite -> y ) == pSprite -> y );

        scene.pRenderer -> RemoveSprite( 1, sprite );
    }
}
