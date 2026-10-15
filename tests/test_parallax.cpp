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
 * L1 of the master plan: per-layer camera and parallax. Every expected screen position below
 * is hand-derived from Tiled's own placement, not re-derived from the code under test: Tiled
 * shifts a parallax layer by (1 - factor) x (view centre + parallax origin), in map pixels
 * (MapScene::parallaxOffset - see Camera::ForParallax). With this file's camera (the negated
 * map point at the view's top-left), half view H and origin O, a layer's effective camera is
 * camera x factor + (1 - factor) x (H + O). Every view here is 2000 x 2000 at zoom 1: H = 1000.
 * (Until 2026-10-15 these values followed an older formula anchored at the view's top-left,
 * (camera + origin) x factor - origin, which did not match Tiled.)
 *
 * Map: 4x4 tiles, 16px each, parallaxoriginx=-100, parallaxoriginy=-50 (negative, so that the
 * camera positions this file exercises keep every drawn position at a non-negative, un-clipped
 * local coordinate - Viewport::GetClippedRect trims anything before the viewport's own origin,
 * a pre-existing, documented limitation (see the scroll-boundary FIXME, master plan item A6),
 * not something this test should trip over). "ground" (id 1, factor 1,1 - the reference) fills
 * every cell; "bg" (id 2, factor 0.5,0.5) has exactly one tile, at map cell (0,0) - the same
 * map position as ground's own first tile, so the two are directly comparable and "bg" is
 * trivially isolated (the only TILE event after ground's 16).
 */

#include <doctest/doctest.h>
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

    Bytes MakeParallaxTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\" parallaxoriginx=\"-100\" parallaxoriginy=\"-50\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"2\" columns=\"2\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"16\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
            << "<layer id=\"2\" name=\"bg\" width=\"4\" height=\"4\" parallaxx=\"0.5\" parallaxy=\"0.5\"><data encoding=\"csv\">2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // A viewport at the screen origin, big enough that none of this file's map tiles or
    // sprites (map coordinates 0-64, camera moves and layer factors that keep every drawn
    // LOCAL position - before the viewport's own offset - non-negative and under a few
    // hundred pixels) ever reach its far edge either.
    SunLight :: Base :: stDimension2D  GenerousViewport( void )  { return Rect( 0, 0, 2000, 2000 ); }

    // Everything one test needs: mocks installed, a renderer over the map above, default view
    // at GenerousViewport(), zoom at its own default (fZoomFactor == 1.0, same convention
    // test_viewpasses.cpp's own Scene relies on).
    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/parallax.tmx"] = MakeParallaxTmx();

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = GenerousViewport();

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/parallax.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
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

        // "ground" draws all 16 cells first (layer id 1, declared first); "bg" (layer id 2)
        // draws its own single tile right after - the 17th TILE event in a single-view pass.
        const Event& GroundTile0( void )  {
            for( const Event& evt : engine().events )
                if( evt.kind == Event :: TILE )
                    return evt;
            FAIL( "no TILE event" );
            return engine().events[0];
        }

        const Event& BgTile( void )  {
            int  nTileIndex = 0;

            for( const Event& evt : engine().events )  {
                if( evt.kind == Event :: TILE )  {
                    nTileIndex++;
                    if( nTileIndex == 17 )
                        return evt;
                }
            }
            FAIL( "bg's own tile (the 17th TILE event) was not found" );
            return engine().events[0];
        }
    };
}

TEST_SUITE( "renderer/parallax" )  {

    TEST_CASE( "A factor of 1 is byte-identical to today, regardless of the map's own parallax origin" )  {

        Scene  scene;

        scene.RunFrames( 1 );
        CHECK( scene.GroundTile0().x == 0.0f );        // (0 + 0) * 1 + viewport.pos.x
        CHECK( scene.GroundTile0().y == 0.0f );

        // SetCameraPosition(-nX,-nY) -> Camera::Position() == (nX, nY): kept positive throughout this
        // file so every drawn local position stays non-negative (see the file's own comment).
        scene.pRenderer -> SetCameraPosition( -20, -30 );
        scene.RunFrames( 1 );
        CHECK( scene.GroundTile0().x == 20.0f );        // (0 + 20) * 1 + 0
        CHECK( scene.GroundTile0().y == 30.0f );        // (0 + 30) * 1 + 0
    }

    TEST_CASE( "A factor of 0.5 moves the layer half as far as the camera, against the factor-1 reference" )  {

        Scene  scene;

        scene.RunFrames( 1 );
        float  fGroundBaseX = scene.GroundTile0().x;
        float  fGroundBaseY = scene.GroundTile0().y;
        float  fBgBaseX     = scene.BgTile().x;
        float  fBgBaseY     = scene.BgTile().y;

        // Asserted as absolute values too (not just the shift below): with origin (-100,-50),
        // factor 0.5, camera (0,0), H 1000 -> effective = 0 x 0.5 + 0.5 x (1000 + -100) = 450 (and,
        // on y, 0.5 x (1000 + -50) = 475). An implementation that ignored the origin or the view's
        // centre would still pass the ratio check below, but not this one.
        CHECK( fBgBaseX == 450.0f );
        CHECK( fBgBaseY == 475.0f );

        scene.pRenderer -> SetCameraPosition( -20, -30 );
        scene.RunFrames( 1 );

        float  fGroundShiftX = scene.GroundTile0().x - fGroundBaseX;
        float  fGroundShiftY = scene.GroundTile0().y - fGroundBaseY;
        float  fBgShiftX     = scene.BgTile().x - fBgBaseX;
        float  fBgShiftY     = scene.BgTile().y - fBgBaseY;

        CHECK( fGroundShiftX == 20.0f );
        CHECK( fGroundShiftY == 30.0f );
        CHECK( fBgShiftX == ( fGroundShiftX * 0.5f ) );
        CHECK( fBgShiftY == ( fGroundShiftY * 0.5f ) );

        // effective = 20 x 0.5 + 450 = 460; 30 x 0.5 + 475 = 490.
        CHECK( scene.BgTile().x == 460.0f );
        CHECK( scene.BgTile().y == 490.0f );
    }

    TEST_CASE( "Each view applies the factor to its own camera" )  {

        Scene  scene;

        scene.pRenderer -> SetCameraPosition( -20, -30 );

        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> CreateView( GenerousViewport() );

        view.SetCameraPosition( -40, -60 );

        scene.RunFrames( 1 );

        // The default view's own pass: bg's effective camera uses the default view's camera
        // (20, 30) - same derivation as the "moves half as far" case above (460, 490).
        // The extra view's own pass: bg's effective camera uses THAT view's own camera
        // (40, 60), and its own size (also 2000 x 2000 at zoom 1, H 1000):
        // x = 40 x 0.5 + 0.5 x (1000 + -100) = 470; y = 60 x 0.5 + 0.5 x (1000 + -50) = 505.
        int  nTileIndex = 0;
        int  nBgEventsFound = 0;

        for( const Event& evt : scene.engine().events )  {
            if( evt.kind != Event :: TILE )
                continue;

            nTileIndex++;

            // bg is the 17th TILE event in EACH pass (ground's own 16 tiles precede it).
            if( ( nTileIndex == 17 ) || ( nTileIndex == 34 ) )  {
                nBgEventsFound++;

                if( nTileIndex == 17 )  {
                    CHECK( evt.x == 460.0f );
                    CHECK( evt.y == 490.0f );
                }
                else  {
                    CHECK( evt.x == 470.0f );
                    CHECK( evt.y == 505.0f );
                }
            }
        }

        REQUIRE( nBgEventsFound == 2 );
    }

    TEST_CASE( "Zooming about a fixed view centre does not move a parallax layer against that centre" )  {

        // The regression seen in samples/parallax: the layer was anchored at the view's top-left, so
        // zooming about a fixed centre slid it away from the rest of the map. Here the ground's map
        // point (32, 32) is kept at the view's centre (screen (1000, 1000)) at zoom 1, 2 and 4 (exact
        // powers of two, so every value below is exact): camera = 1000 / zoom - 32. bg's tile sits at
        // map (0, 0), so it is drawn at its layer camera x zoom, and the bg point under the centre is
        // 1000 / zoom - drawn / zoom. By Tiled's placement it is factor x 32 - (1 - factor) x origin:
        // 16 + 50 = 66 on x, 16 + 25 = 41 on y, at every zoom.
        const unsigned  aZoomPositions[] = { 15u, 31u, 63u };                // factors 1, 2 and 4
        const float     fCentre = 1000.0f;
        const float     fGroundPoint = 32.0f;

        Scene  scene;

        for( unsigned nZoomPos : aZoomPositions )  {
            scene.pRenderer -> GetViewport().SetZoom( nZoomPos );

            float  fZoom   = scene.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;
            int    nCamera = ( int ) ( ( fCentre / fZoom ) - fGroundPoint );

            scene.pRenderer -> SetCameraPosition( -nCamera, -nCamera );
            scene.RunFrames( 1 );

            INFO( "zoom " << fZoom );
            // The ground's map point (32, 32) is at the centre: (32 + camera) x zoom == 1000.
            CHECK( ( fGroundPoint + nCamera ) * fZoom == fCentre );
            CHECK( ( fCentre / fZoom ) - ( scene.BgTile().x / fZoom ) == 66.0f );
            CHECK( ( fCentre / fZoom ) - ( scene.BgTile().y / fZoom ) == 41.0f );
        }
    }

    TEST_CASE( "SetLayerParallax overrides the TMX-declared factor" )  {

        Scene  scene;

        REQUIRE( scene.pRenderer -> SetLayerParallax( 2, 1.0f, 1.0f ) == true );

        scene.pRenderer -> SetCameraPosition( -20, -30 );
        scene.RunFrames( 1 );

        // bg now behaves exactly like ground (factor 1,1) - no parallax at all.
        CHECK( scene.BgTile().x == scene.GroundTile0().x );
        CHECK( scene.BgTile().y == scene.GroundTile0().y );
    }

    TEST_CASE( "SetLayerParallax by name works the same as by id" )  {

        Scene  scene;

        REQUIRE( scene.pRenderer -> SetLayerParallax( "bg", 1.0f, 1.0f ) == true );
        CHECK( scene.pRenderer -> SetLayerParallax( "nope", 1.0f, 1.0f ) == false );

        scene.pRenderer -> SetCameraPosition( -20, -30 );
        scene.RunFrames( 1 );

        CHECK( scene.BgTile().x == scene.GroundTile0().x );
    }

    // A sprite's own draw is recognised by its distinctive texture handle, 0x77 - never shared
    // with any map tile/image draw (those use the mock's own default handle, 0x1).
    const Event* FindSpriteDraw( Scene& scene )  {
        for( const Event& evt : scene.engine().events )
            if( ( evt.kind == Event :: TILE ) && ( evt.handle == ( void * ) 0x77 ) )
                return &evt;
        return nullptr;
    }

    TEST_CASE( "A world-space sprite on a parallax layer follows its own layer's factor" )  {

        Scene                                 scene;
        SunLight :: Sprite :: Sprite          sprite;
        SunLight :: Canvas :: TextureCanvas   canvas;

        scene.engine().hLoadTextureResult = ( SunLight :: Base :: TextureHandle ) 0x77;

        REQUIRE( canvas.Load( "sprite.png" ) == true );

        // Map position (0, 0) - the same map position as "bg"'s own tile, so the two are
        // directly comparable: a world-space sprite must land on EXACTLY the same screen
        // position as a tile on the same layer, at the same map position.
        canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 16, 16 } } );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetWorldSpace( true );

        REQUIRE( scene.pRenderer -> AddSprite( 2, sprite ) == true );   // layer id 2 = "bg"

        scene.pRenderer -> SetCameraPosition( -20, -30 );
        scene.RunFrames( 1 );

        const Event  *pSprite = FindSpriteDraw( scene );

        REQUIRE( pSprite != nullptr );
        // Same effective camera "bg"'s own tile gets (see the "moves half as far" test).
        CHECK( pSprite -> x == scene.BgTile().x );
        CHECK( pSprite -> y == scene.BgTile().y );
        CHECK( pSprite -> x == 460.0f );
        CHECK( pSprite -> y == 490.0f );

        scene.pRenderer -> RemoveSprite( 2, sprite );
    }

    TEST_CASE( "A screen-space sprite on the same parallax layer is entirely unaffected by it" )  {

        Scene                                 scene;
        SunLight :: Sprite :: Sprite          sprite;
        SunLight :: Canvas :: TextureCanvas   canvas;

        scene.engine().hLoadTextureResult = ( SunLight :: Base :: TextureHandle ) 0x77;

        REQUIRE( canvas.Load( "sprite.png" ) == true );

        canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 16, 16 } } );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        // Screen-space is the default - SetWorldSpace is deliberately not called.

        REQUIRE( scene.pRenderer -> AddSprite( 2, sprite ) == true );   // layer id 2 = "bg"

        scene.RunFrames( 1 );

        const Event  *pBefore = FindSpriteDraw( scene );

        REQUIRE( pBefore != nullptr );
        CHECK( pBefore -> x == 0.0f );        // viewport.pos.x + its own stored position (0)
        CHECK( pBefore -> y == 0.0f );

        scene.pRenderer -> SetCameraPosition( -20, -30 );
        scene.RunFrames( 1 );

        const Event  *pAfter = FindSpriteDraw( scene );

        REQUIRE( pAfter != nullptr );

        // Unlike "bg"'s own tile (which moved), the screen-space sprite on the very same
        // layer is entirely unaffected by both the camera move and the layer's own factor.
        CHECK( pAfter -> x == pBefore -> x );
        CHECK( pAfter -> y == pBefore -> y );
        CHECK( scene.BgTile().x != pAfter -> x );

        scene.pRenderer -> RemoveSprite( 2, sprite );
    }
}
