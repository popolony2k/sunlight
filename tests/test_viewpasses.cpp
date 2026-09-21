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
 * Drawing the views: the multi-pass frame. Run against MockEngine's ordered event log, so each
 * test can say WHICH view's pass drew what and in what order - a view's tiles are recognised by
 * where they land (the views used here never overlap unless a test is about overlap).
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
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

    SunLight :: TileMap :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: TileMap :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // A 4 x 4 map of 16 px tiles, one embedded 2-tile tileset, four layers:
    //   id 1 "ground"  - all 16 tiles          (16 tile draws in a view that shows it whole)
    //   id 2 "props"   - one tile               ( 1 draw)
    //   id 3 "fx"      - a GROUP, holding
    //   id 4 "glow"    - one tile               ( 1 draw)
    //   id 5 "sky"     - an IMAGE layer         (drawn by the default view only)
    Bytes MakeTiledTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"6\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"2\" columns=\"2\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"16\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
            << "<layer id=\"2\" name=\"props\" width=\"4\" height=\"4\"><data encoding=\"csv\">2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
            << "<group id=\"3\" name=\"fx\">"
            << "<layer id=\"4\" name=\"glow\" width=\"4\" height=\"4\"><data encoding=\"csv\">0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0</data></layer>"
            << "</group>"
            << "<imagelayer id=\"5\" name=\"sky\"><image source=\"sky.png\" width=\"32\" height=\"32\"/></imagelayer>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // Everything one test needs: mocks installed, a renderer over the map above, default view at (10, 10, 100, 100).
    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/tiled.tmx"] = MakeTiledTmx();

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 10, 10, 100, 100 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/tiled.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        // Run exactly nFrames frames, starting the event log fresh.
        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // Number of tile draws that landed inside the rectangle (their top-left is inside it).
        int TilesIn( const SunLight :: TileMap :: stDimension2D &rect )  {
            int  nCount = 0;

            for( const Event &evt : engine().events )  {
                if( ( evt.kind == Event :: TILE ) &&
                    ( evt.x >= rect.pos.x ) && ( evt.x < rect.pos.x + rect.size.nWidth ) &&
                    ( evt.y >= rect.pos.y ) && ( evt.y < rect.pos.y + rect.size.nHeight ) )
                    nCount++;
            }

            return nCount;
        }

        int Count( Event :: Kind kind )  {
            int  nCount = 0;

            for( const Event &evt : engine().events )
                if( evt.kind == kind )
                    nCount++;

            return nCount;
        }

        // The frame's own clear is the FIRST clear, before anything is drawn (Run() also clears
        // once at the very end, around the letterboxed blit, which is not what is under test).
        // Returns its index or -1 when the frame did not clear before its first tile/fill.
        int FrameClear( void )  {
            for( int nIdx = 0; nIdx < ( int ) engine().events.size(); nIdx++ )  {
                Event :: Kind  kind = engine().events[nIdx].kind;

                if( kind == Event :: CLEAR )
                    return nIdx;

                if( ( kind == Event :: TILE ) || ( kind == Event :: FILL ) )
                    return -1;
            }

            return -1;
        }

        // Index of the first event of that kind at/after nFrom, or -1.
        int IndexOf( Event :: Kind kind, int nFrom = 0 )  {
            for( int nIdx = nFrom; nIdx < ( int ) engine().events.size(); nIdx++ )
                if( engine().events[nIdx].kind == kind )
                    return nIdx;

            return -1;
        }
    };

    const SunLight :: TileMap :: stDimension2D  g_DefaultRect = Rect( 10, 10, 100, 100 );
    const SunLight :: TileMap :: stDimension2D  g_SideRect    = Rect( 300, 10, 100, 100 );
}

TEST_SUITE( "renderer/viewpasses" )  {

    TEST_CASE( "With no extra view the frame is what it always was: one clear, the map once, nothing else" )  {

        Scene  scene;

        scene.RunFrames( 1 );

        CHECK( scene.FrameClear() == 0 );
        CHECK( scene.Count( Event :: FILL ) == 0 );
        CHECK( scene.Count( Event :: TILE ) == 18 );        // ground 16 + props 1 + glow 1
    }

    TEST_CASE( "An extra view draws the same map again, in its own rectangle, after clearing that rectangle" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();

        scene.RunFrames( 1 );

        // Once-per-frame: a single whole-frame clear, first - and none between the passes.
        CHECK( scene.FrameClear() == 0 );

        int  nClearsBeforeLastTile = 0;
        int  nLastTile             = -1;

        for( int nIdx = 0; nIdx < ( int ) scene.engine().events.size(); nIdx++ )
            if( scene.engine().events[nIdx].kind == Event :: TILE )
                nLastTile = nIdx;

        for( int nIdx = 0; nIdx < nLastTile; nIdx++ )
            if( scene.engine().events[nIdx].kind == Event :: CLEAR )
                nClearsBeforeLastTile++;

        CHECK( nClearsBeforeLastTile == 1 );

        // One rectangle fill: the extra view's own rectangle.
        REQUIRE( scene.Count( Event :: FILL ) == 1 );

        const Event  &fill = scene.engine().events[scene.IndexOf( Event :: FILL )];

        CHECK( fill.x == 300.0f );
        CHECK( fill.y == 10.0f );
        CHECK( fill.w == 100.0f );
        CHECK( fill.h == 100.0f );

        // Each view drew the whole map, inside its own rectangle only.
        CHECK( scene.TilesIn( g_DefaultRect ) == 18 );
        CHECK( scene.TilesIn( g_SideRect ) == 18 );
        CHECK( scene.Count( Event :: TILE ) == 36 );

        // Default view first, then the fill, then the extra view's tiles (painter's order).
        int  nFill = scene.IndexOf( Event :: FILL );

        for( int nIdx = 0; nIdx < ( int ) scene.engine().events.size(); nIdx++ )  {
            const Event  &evt = scene.engine().events[nIdx];

            if( evt.kind == Event :: TILE )
                CHECK( ( evt.x < 200.0f ) == ( nIdx < nFill ) );
        }

        // The renderer is back on its default view afterwards.
        CHECK( &scene.pRenderer -> GetViewport() == &scene.pRenderer -> GetDefaultView().GetViewport() );
        (void) nId;
    }

    TEST_CASE( "An extra view draws with its OWN zoom and camera" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();
        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> GetView( nId );

        view.GetViewport().SetZoom( 31 );                   // factor 2.0
        view.SetCameraPosition( 0, 0 );

        scene.RunFrames( 1 );

        // 16 px tiles at 2x, from the view's own top-left: 32 wide, the first at (300, 10).
        int  nScaled = 0;
        int  nPlain  = 0;

        for( const Event &evt : scene.engine().events )  {
            if( evt.kind != Event :: TILE )
                continue;

            if( evt.x >= 200.0f )  {
                CHECK( evt.scale == 2.0f );
                nScaled++;
            }
            else  {
                CHECK( evt.scale == 1.0f );
                nPlain++;
            }
        }

        CHECK( nPlain == 18 );
        CHECK( nScaled > 0 );
    }

    TEST_CASE( "An image layer is drawn once - by the default view - because it is drawn at the screen origin, camera- and viewport-blind" )  {

        Scene  scene;

        scene.RunFrames( 1 );
        CHECK( scene.engine().nDrawTextureCalls == 1 );

        int  nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();

        scene.engine().nDrawTextureCalls = 0;
        scene.RunFrames( 1 );
        CHECK( scene.engine().nDrawTextureCalls == 1 );          // still one, not one per view

        // Hidden default view: nothing draws it (no other view does).
        scene.pRenderer -> GetDefaultView().SetVisible( false );
        scene.engine().nDrawTextureCalls = 0;
        scene.RunFrames( 1 );
        CHECK( scene.engine().nDrawTextureCalls == 0 );
        (void) nId;
    }

    TEST_CASE( "The layer mask: ShowOnlyLayers, ShowLayer by id and by name, ShowAllLayers - per view, never touching another view" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();
        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> GetView( nId );

        // Only "ground" (id 1).
        view.ShowOnlyLayers( { 1 } );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 16 );
        CHECK( scene.TilesIn( g_DefaultRect ) == 18 );           // the default view is untouched

        // Ground plus props: props is a plain add-back by id...
        view.ShowLayer( 2, true );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 17 );

        // ...and hiding one by NAME (props again) takes it away.
        CHECK( view.ShowLayer( "props", false ) == true );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 16 );

        // An unknown name changes nothing and says so.
        CHECK( view.ShowLayer( "nope", false ) == false );
        CHECK( view.ShowLayer( nullptr, false ) == false );

        // Back to everything.
        view.ShowAllLayers();
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 18 );

        // The other direction: everything except ground.
        view.ShowLayer( 1, false );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 2 );
        CHECK( view.IsLayerShown( 1 ) == false );
        CHECK( view.IsLayerShown( 2 ) == true );
        CHECK( scene.pRenderer -> GetDefaultView().IsLayerShown( 1 ) == true );
    }

    TEST_CASE( "A group layer that is masked out hides its children; a shown group applies the mask to each child" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();
        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> GetView( nId );

        // Hide the group (id 3): its child "glow" (id 4) goes with it, whatever the child's own setting.
        view.ShowLayer( 3, false );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 17 );

        // Hide only the child: the group is shown, the mask applies to the child.
        view.ShowAllLayers();
        view.ShowLayer( 4, false );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 17 );

        // A whitelist naming only the child does NOT show it - its group is not on the list.
        view.ShowOnlyLayers( { 4 } );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 0 );

        // Naming the group as well does.
        view.ShowOnlyLayers( { 3, 4 } );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 1 );
    }

    TEST_CASE( "The mask may be set before the map is loaded, by id; by name needs the map" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;

        fsFixture.fs.files["maps/tiled.tmx"] = MakeTiledTmx();

        RendererConfig  config;

        config.viewport = g_DefaultRect;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        int  nId = pRenderer -> CreateView( g_SideRect ) -> GetId();
        SunLight :: TileMap :: IView  &view = *pRenderer -> GetView( nId );

        CHECK( view.ShowLayer( "ground", false ) == false );        // no map yet: nothing to resolve the name against
        view.ShowLayer( 1, false );                                  // by id: fine

        REQUIRE( pRenderer -> LoadMap( "maps/tiled.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        engineFixture.engine.events.clear();
        windowFixture.window.nFramesUntilShouldClose = 1;
        pRenderer -> Run();

        int  nSide = 0;

        for( const Event &evt : engineFixture.engine.events )
            if( ( evt.kind == Event :: TILE ) && ( evt.x >= 200.0f ) )
                nSide++;

        CHECK( nSide == 2 );                                         // props + glow, no ground
        pRenderer -> Stop();
    }

    TEST_CASE( "A hidden view is skipped and keeps its state; hiding the default view leaves the extra ones drawn" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();
        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> GetView( nId );

        CHECK( view.GetVisible() == true );
        view.SetVisible( false );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 0 );
        CHECK( scene.Count( Event :: FILL ) == 0 );                 // not even its backdrop
        CHECK( scene.TilesIn( g_DefaultRect ) == 18 );

        view.SetVisible( true );
        view.SetVisible( true );
        scene.pRenderer -> GetDefaultView().SetVisible( false );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_DefaultRect ) == 0 );
        CHECK( scene.TilesIn( g_SideRect ) == 18 );
        CHECK( scene.FrameClear() == 0 );                           // the frame clear still happens

        // And with no extra views at all, a hidden default view draws nothing either.
        scene.pRenderer -> RemoveView( nId );
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 0 );
    }

    TEST_CASE( "Draw order: ascending order, ties by id; the default view sits at 0 and extras at their own id" )  {

        Scene  scene;
        int    nA = scene.pRenderer -> CreateView( Rect( 300, 10, 100, 100 ) ) -> GetId();
        int    nB = scene.pRenderer -> CreateView( Rect( 500, 10, 100, 100 ) ) -> GetId();

        CHECK( scene.pRenderer -> GetDefaultView().GetDrawOrder() == 0 );
        CHECK( scene.pRenderer -> GetView( nA ) -> GetDrawOrder() == nA );
        CHECK( scene.pRenderer -> GetView( nB ) -> GetDrawOrder() == nB );

        // Order of the passes = order of the backdrop fills (default has none): A (300) before B (500).
        scene.RunFrames( 1 );

        REQUIRE( scene.Count( Event :: FILL ) == 2 );
        CHECK( scene.engine().events[scene.IndexOf( Event :: FILL )].x == 300.0f );

        // Put B first.
        scene.pRenderer -> GetView( nB ) -> SetDrawOrder( -5 );
        scene.RunFrames( 1 );
        CHECK( scene.engine().events[scene.IndexOf( Event :: FILL )].x == 500.0f );

        // Default view drawn LAST: its tiles come after every fill.
        scene.pRenderer -> GetDefaultView().SetDrawOrder( 100 );
        scene.RunFrames( 1 );

        int  nLastFill = -1;
        int  nFirstDefaultTile = -1;

        for( int nIdx = 0; nIdx < ( int ) scene.engine().events.size(); nIdx++ )  {
            const Event  &evt = scene.engine().events[nIdx];

            if( evt.kind == Event :: FILL )
                nLastFill = nIdx;

            if( ( evt.kind == Event :: TILE ) && ( evt.x < 200.0f ) && ( nFirstDefaultTile < 0 ) )
                nFirstDefaultTile = nIdx;
        }

        CHECK( nFirstDefaultTile > nLastFill );
        CHECK( scene.pRenderer -> GetView( nB ) -> GetDrawOrder() == -5 );
    }

    TEST_CASE( "Per-view background: filled by default with the frame's color, off on request, or an explicit color" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();
        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> GetView( nId );

        CHECK( view.GetClearBackground() == true );

        // Default color: the very one the frame clear used (alpha included).
        scene.RunFrames( 1 );

        REQUIRE( scene.FrameClear() == 0 );

        SunLight :: Base :: stColor  frame = scene.engine().events[0].color;
        SunLight :: Base :: stColor  fill  = scene.engine().lastFilledRectangleColor;

        CHECK( fill.nAlpha == frame.nAlpha );

        CHECK( fill.nRed   == frame.nRed );
        CHECK( fill.nGreen == frame.nGreen );
        CHECK( fill.nBlue  == frame.nBlue );

        // Explicit color, translucent.
        view.SetBackgroundColor( SunLight :: Base :: stColor { 1, 2, 3, 4 } );
        scene.RunFrames( 1 );
        fill = scene.engine().lastFilledRectangleColor;
        CHECK( fill.nRed == 1 );
        CHECK( fill.nGreen == 2 );
        CHECK( fill.nBlue == 3 );
        CHECK( fill.nAlpha == 4 );

        // Back to the map's own.
        view.UseMapBackgroundColor();
        scene.RunFrames( 1 );
        fill = scene.engine().lastFilledRectangleColor;
        CHECK( fill.nRed == frame.nRed );
        CHECK( fill.nAlpha == frame.nAlpha );

        // Off: no fill, but the tiles are still drawn.
        view.SetClearBackground( false );
        CHECK( view.GetClearBackground() == false );
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: FILL ) == 0 );
        CHECK( scene.TilesIn( g_SideRect ) == 18 );
    }

    TEST_CASE( "The default view's background IS the renderer's frame clear: same flag, and an explicit color overrides the map's" )  {

        Scene  scene;
        SunLight :: TileMap :: IView  &view = scene.pRenderer -> GetDefaultView();

        CHECK( view.GetClearBackground() == scene.pRenderer -> GetClearBackground() );

        view.SetClearBackground( false );
        CHECK( scene.pRenderer -> GetClearBackground() == false );
        scene.RunFrames( 1 );
        CHECK( scene.FrameClear() == -1 );

        scene.pRenderer -> SetClearBackground( true );
        CHECK( view.GetClearBackground() == true );

        view.SetBackgroundColor( SunLight :: Base :: stColor { 9, 8, 7, 255 } );
        scene.RunFrames( 1 );
        REQUIRE( scene.FrameClear() == 0 );
        CHECK( scene.engine().events[0].color.nRed == 9 );
        CHECK( scene.engine().events[0].color.nGreen == 8 );
        CHECK( scene.engine().events[0].color.nBlue == 7 );

        // Not a per-view rectangle fill: the default view never fills.
        CHECK( scene.Count( Event :: FILL ) == 0 );
    }

    TEST_CASE( "FPS is drawn once per frame, after every view" )  {

        Scene  scene;

        scene.pRenderer -> SetDrawFPS( true );
        scene.pRenderer -> CreateView( g_SideRect );
        scene.RunFrames( 1 );

        CHECK( scene.Count( Event :: FPS ) == 1 );

        int  nFps = scene.IndexOf( Event :: FPS );

        for( int nIdx = 0; nIdx < ( int ) scene.engine().events.size(); nIdx++ )  {
            Event :: Kind  kind = scene.engine().events[nIdx].kind;

            if( ( kind == Event :: TILE ) || ( kind == Event :: FILL ) )
                CHECK( nIdx < nFps );
        }
    }

    TEST_CASE( "Removing the last extra view returns to the plain single-view frame" )  {

        Scene  scene;
        int    nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();

        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 36 );

        REQUIRE( scene.pRenderer -> RemoveView( nId ) == true );
        scene.RunFrames( 1 );
        CHECK( scene.Count( Event :: TILE ) == 18 );
        CHECK( scene.Count( Event :: FILL ) == 0 );
    }

    TEST_CASE( "FitToMap zooms to the largest zoom at which the whole map fits the view, camera at the map's top-left" )  {

        Scene  scene;

        // 64 x 64 map. A 40 x 40 view: 40/64 = 0.625 = exactly 10 steps of 1/16.
        int  nId = scene.pRenderer -> CreateView( Rect( 300, 10, 40, 40 ) ) -> GetId();
        SunLight :: TileMap :: IView  &view = *scene.pRenderer -> GetView( nId );

        view.SetCameraPosition( 5, 5 );
        REQUIRE( view.FitToMap() == true );
        CHECK( view.GetViewport().GetZoomProperties().nCurrentZoomPos == 9 );
        CHECK( view.GetViewport().GetZoomProperties().fZoomFactor == 0.625f );

        int  nX = -1, nY = -1;

        view.GetCameraPosition( nX, nY );
        CHECK( nX == 0 );
        CHECK( nY == 0 );

        // Everything is on screen: all 18 tiles land inside the view, none clipped away.
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( Rect( 300, 10, 40, 40 ) ) == 18 );

        // A view exactly the map's size fits at zoom 1.0 (position 15); a non-square view fits the tighter axis.
        int  nBig = scene.pRenderer -> CreateView( Rect( 0, 0, 64, 64 ) ) -> GetId();

        REQUIRE( scene.pRenderer -> GetView( nBig ) -> FitToMap() == true );
        CHECK( scene.pRenderer -> GetView( nBig ) -> GetViewport().GetZoomProperties().nCurrentZoomPos == 15 );

        int  nWide = scene.pRenderer -> CreateView( Rect( 0, 0, 400, 32 ) ) -> GetId();

        REQUIRE( scene.pRenderer -> GetView( nWide ) -> FitToMap() == true );
        CHECK( scene.pRenderer -> GetView( nWide ) -> GetViewport().GetZoomProperties().fZoomFactor == 0.5f );

        // Too small even for the lowest zoom: clamped to it.
        int  nTiny = scene.pRenderer -> CreateView( Rect( 0, 0, 2, 2 ) ) -> GetId();

        REQUIRE( scene.pRenderer -> GetView( nTiny ) -> FitToMap() == true );
        CHECK( scene.pRenderer -> GetView( nTiny ) -> GetViewport().GetZoomProperties().nCurrentZoomPos == 0 );
    }

    TEST_CASE( "FitToMap with no map loaded changes nothing and says so" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;

        RendererConfig  config;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        int  nId = pRenderer -> CreateView( Rect( 300, 10, 40, 40 ) ) -> GetId();
        SunLight :: TileMap :: IView  &view = *pRenderer -> GetView( nId );
        unsigned  nBefore = view.GetViewport().GetZoomProperties().nCurrentZoomPos;

        CHECK( view.FitToMap() == false );
        CHECK( view.GetViewport().GetZoomProperties().nCurrentZoomPos == nBefore );
        pRenderer -> Stop();
    }

    // A one-sequence sprite on layer 1: a 64 px wide texture of 16 px frames animating in circle, at (20, 20)
    // of whichever viewport draws it. Its draws are recognised by the texture handle 0x77.
    struct SpriteRig  {
        Scene                              &m_Scene;
        int                                m_nLayerId;
        SunLight :: Sprite :: Sprite       sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;

        SpriteRig( Scene &scene, int nLayerId ) : m_Scene( scene ), m_nLayerId( nLayerId )  {
            scene.engine().hLoadTextureResult = ( SunLight :: Base :: TextureHandle ) 0x77;
            scene.engine().nLoadTextureWidth  = 64;
            scene.engine().nLoadTextureHeight = 16;

            REQUIRE( canvas.Load( "sprite.png" ) == true );

            canvas.SetTileSize( 16 );
            canvas.SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
            canvas.SetDimension2D( SunLight :: TileMap :: stDimension2D { { 20, 20 }, { 16, 16 } } );
            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );

            REQUIRE( scene.pRenderer -> AddSprite( nLayerId, sprite ) == true );
        }

        // The renderer only holds a raw pointer to the sprite and unloads it again at Stop(): take it
        // back out before the sprite (and its canvas) go away.
        ~SpriteRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( m_nLayerId, sprite );
        }
    };

    // The sprite's per-frame draws: for each of nFrames frames, the source x of every sprite draw in it.
    std :: vector<std :: vector<float>> SpriteFrames( Scene &scene, int nFrames )  {
        std :: vector<std :: vector<float>>  frames;

        for( int nFrame = 0; nFrame < nFrames; nFrame++ )  {
            scene.RunFrames( 1 );

            std :: vector<float>  draws;

            for( const Event &evt : scene.engine().events )
                if( ( evt.kind == Event :: TILE ) && ( evt.handle == ( void * ) 0x77 ) )
                    draws.push_back( evt.srcX );

            frames.push_back( draws );
        }

        return frames;
    }

    TEST_CASE( "A sprite advances ONCE per frame however many views draw it: every view shows the same frame, the sequence is the single-view one" )  {

        std :: vector<std :: vector<float>>  single;
        std :: vector<std :: vector<float>>  multi;

        {
            Scene      scene;
            SpriteRig  rig( scene, 1 );

            single = SpriteFrames( scene, 8 );
        }

        {
            Scene      scene;
            SpriteRig  rig( scene, 1 );

            scene.pRenderer -> CreateView( g_SideRect );
            scene.pRenderer -> CreateView( Rect( 500, 10, 100, 100 ) );
            multi = SpriteFrames( scene, 8 );
        }

        REQUIRE( single.size() == 8 );
        REQUIRE( multi.size() == 8 );

        // The animation really moves (not vacuous)...
        CHECK( single[0][0] != single[1][0] );

        for( int nFrame = 0; nFrame < 8; nFrame++ )  {
            INFO( "frame " << nFrame );
            REQUIRE( single[nFrame].size() == 1 );
            REQUIRE( multi[nFrame].size() == 3 );                  // one draw per view

            // ...every view shows the frame the single-view run shows...
            CHECK( multi[nFrame][0] == single[nFrame][0] );
            CHECK( multi[nFrame][1] == single[nFrame][0] );
            CHECK( multi[nFrame][2] == single[nFrame][0] );
        }
    }

    TEST_CASE( "A sprite on a layer no view shows stands still; a layer masked from one view still animates for the others" )  {

        Scene      scene;
        SpriteRig  rig( scene, 1 );
        int        nId = scene.pRenderer -> CreateView( g_SideRect ) -> GetId();

        // Ground (the sprite's layer) masked out of the extra view: only the default view draws the sprite.
        scene.pRenderer -> GetView( nId ) -> ShowLayer( 1, false );

        std :: vector<std :: vector<float>>  frames = SpriteFrames( scene, 4 );

        for( int nFrame = 0; nFrame < 4; nFrame++ )
            CHECK( frames[nFrame].size() == 1 );

        CHECK( frames[0][0] != frames[1][0] );                   // and it animated

        // Masked out of every view: not drawn and not advanced (a hidden layer's sprites are frozen).
        scene.pRenderer -> GetDefaultView().ShowLayer( 1, false );
        frames = SpriteFrames( scene, 3 );

        for( int nFrame = 0; nFrame < 3; nFrame++ )
            CHECK( frames[nFrame].size() == 0 );
    }
}

TEST_SUITE( "renderer/viewhandles" )  {

    typedef std :: shared_ptr<SunLight :: TileMap :: IView>  ViewHandle;

    TEST_CASE( "CreateView returns the shared handle GetView(id) returns; the renderer keeps its own reference, so dropping it changes nothing" )  {

        Scene  scene;

        {
            ViewHandle  pHandle = scene.pRenderer -> CreateView( g_SideRect );

            REQUIRE( pHandle != nullptr );
            CHECK( pHandle -> GetId() > 0 );
            CHECK( scene.pRenderer -> GetView( pHandle -> GetId() ) == pHandle );
            CHECK( pHandle -> IsRemoved() == false );
        }

        // The handle is gone; the view is not: still counted, still reachable, still drawn.
        CHECK( scene.pRenderer -> GetViewCount() == 2 );
        REQUIRE( scene.pRenderer -> GetView( 1 ) != nullptr );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 18 );

        // A discarded return value works the same way.
        scene.pRenderer -> CreateView( Rect( 500, 10, 100, 100 ) );
        CHECK( scene.pRenderer -> GetViewCount() == 3 );
    }

    TEST_CASE( "The default view can never be removed - by id or by handle - and is the same object everywhere" )  {

        Scene  scene;

        ViewHandle  pDefault = scene.pRenderer -> GetView( 0 );

        REQUIRE( pDefault != nullptr );
        CHECK( pDefault.get() == &scene.pRenderer -> GetDefaultView() );
        CHECK( pDefault -> GetId() == 0 );

        CHECK( scene.pRenderer -> RemoveView( 0 ) == false );
        CHECK( scene.pRenderer -> RemoveView( pDefault ) == false );
        CHECK( scene.pRenderer -> GetViewCount() == 1 );
        CHECK( pDefault -> IsRemoved() == false );
        CHECK( scene.pRenderer -> GetView( 0 ) == pDefault );

        // Still drawn, still the renderer's own view.
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_DefaultRect ) == 18 );
    }

    TEST_CASE( "A held handle stays SAFE after its view is removed: inert, not gone" )  {

        Scene       scene;
        ViewHandle  pHandle = scene.pRenderer -> CreateView( g_SideRect );
        int         nId     = pHandle -> GetId();
        int         nX = -1, nY = -1;

        pHandle -> SetCameraPosition( 7, 9 );
        pHandle -> SetScrollStepSize( 3, 5 );
        scene.pRenderer -> SetCameraPosition( 20, 30 );            // the DEFAULT view's own camera

        REQUIRE( scene.pRenderer -> RemoveView( nId ) == true );

        // No longer a view of the renderer.
        CHECK( pHandle -> IsRemoved() == true );
        CHECK( scene.pRenderer -> GetView( nId ) == nullptr );
        CHECK( scene.pRenderer -> GetViewCount() == 1 );
        CHECK( scene.pRenderer -> RemoveView( nId ) == false );          // already gone
        CHECK( scene.pRenderer -> RemoveView( pHandle ) == false );

        // Not drawn any more, even when told to be visible.
        pHandle -> SetVisible( true );
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect ) == 0 );
        CHECK( scene.Count( Event :: FILL ) == 0 );
        CHECK( scene.TilesIn( g_DefaultRect ) == 18 );

        // What acts on the renderer does nothing / answers "no" - and changes nothing of the renderer's.
        pHandle -> MoveCameraLeft();
        pHandle -> MoveCameraUp();
        pHandle -> ZoomIn();
        pHandle -> ZoomOut();
        pHandle -> ResetZoom();
        pHandle -> ResetCamera();
        pHandle -> SetCameraPosition( 500, 500 );
        pHandle -> SetScrollStepSize( 99, 99 );

        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

        CHECK( pHandle -> TileMapToTileMatrix( SunLight :: TileMap :: stCoordinate2D { 100, 100 }, pos ) == false );
        CHECK( pHandle -> FitToMap() == false );
        CHECK( pHandle -> ShowLayer( "ground", false ) == false );

        // It keeps the last state it had...
        pHandle -> GetCameraPosition( nX, nY );
        CHECK( nX == 7 );
        CHECK( nY == 9 );
        pHandle -> GetScrollStepSize( nX, nY );
        CHECK( nX == 3 );
        CHECK( nY == 5 );

        // ...its Viewport is its own and still valid, untouched by the calls above...
        CHECK( pHandle -> GetViewport().GetDimension2D().pos.x == 300 );
        CHECK( pHandle -> GetViewport().GetZoomProperties().nCurrentZoomPos == 15 );
        CHECK( &pHandle -> GetViewport() != &scene.pRenderer -> GetViewport() );

        // ...and what is just the view's own data still works.
        pHandle -> ShowLayer( 1, false );
        CHECK( pHandle -> IsLayerShown( 1 ) == false );
        pHandle -> SetDrawOrder( 42 );
        CHECK( pHandle -> GetDrawOrder() == 42 );
        pHandle -> SetClearBackground( false );
        CHECK( pHandle -> GetClearBackground() == false );

        // The renderer itself is exactly as it was.
        scene.pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == 20 );
        CHECK( nY == 30 );
        CHECK( &scene.pRenderer -> GetViewport() == &scene.pRenderer -> GetDefaultView().GetViewport() );
    }

    TEST_CASE( "RemoveView by handle removes only THIS renderer's view: not another renderer's, not an empty pointer" )  {

        Scene  scene;

        // A second renderer (never started) whose first extra view has the same id as ours.
        RendererConfig                      config;
        std :: unique_ptr<TileMapRenderer>  pOther = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pOther != nullptr );

        ViewHandle  pOurs   = scene.pRenderer -> CreateView( g_SideRect );
        ViewHandle  pTheirs = pOther -> CreateView( g_SideRect );

        REQUIRE( pOurs -> GetId() == pTheirs -> GetId() );

        CHECK( scene.pRenderer -> RemoveView( pTheirs ) == false );
        CHECK( scene.pRenderer -> GetViewCount() == 2 );
        CHECK( pTheirs -> IsRemoved() == false );
        CHECK( pOurs -> IsRemoved() == false );

        CHECK( scene.pRenderer -> RemoveView( ViewHandle() ) == false );
        CHECK( scene.pRenderer -> GetViewCount() == 2 );

        CHECK( scene.pRenderer -> RemoveView( pOurs ) == true );
        CHECK( pOurs -> IsRemoved() == true );
        CHECK( scene.pRenderer -> GetViewCount() == 1 );
        CHECK( scene.pRenderer -> RemoveView( pOurs ) == false );
    }

    TEST_CASE( "Handles outlive the renderer: inert, with the last state, and every call safe (default view included)" )  {

        Scene  scene;

        // State worth keeping, on the default view and on an extra one.
        SunLight :: TileMap :: IView  &defaultView = scene.pRenderer -> GetDefaultView();

        defaultView.SetCameraPosition( 20, 30 );
        defaultView.GetViewport().SetMinZoom( 5 );
        defaultView.GetViewport().SetMaxZoom( 200 );
        defaultView.GetViewport().SetPreferredZoom( 20 );
        defaultView.GetViewport().SetZoom( 30 );
        defaultView.SetScrollStepSize( 6, 8 );
        defaultView.SetBackgroundColor( SunLight :: Base :: stColor { 9, 8, 7, 255 } );

        bool  bUserZoom = defaultView.GetViewport().GetZoomProperties().bEnabledUserZoom;

        ViewHandle  pDefault = scene.pRenderer -> GetView( 0 );
        ViewHandle  pExtra   = scene.pRenderer -> CreateView( g_SideRect );

        pExtra -> SetCameraPosition( 40, 50 );
        pExtra -> GetViewport().SetZoom( 33 );
        pExtra -> ShowLayer( 2, false );

        scene.pRenderer.reset();                       // the renderer is gone; the handles are not

        int  nX = -1, nY = -1;

        CHECK( pDefault -> IsRemoved() == true );
        CHECK( pExtra -> IsRemoved() == true );

        // The default view's Viewport was copied out of the renderer before it died.
        SunLight :: Base :: Viewport  &vp = pDefault -> GetViewport();
        unsigned                      nMin = 0, nMax = 0;

        CHECK( vp.GetDimension2D().pos.x == 10 );
        CHECK( vp.GetDimension2D().size.nWidth == 100 );
        CHECK( vp.GetZoomProperties().nCurrentZoomPos == 30 );
        CHECK( vp.GetZoomProperties().nPreferredZoomPos == 20 );
        CHECK( vp.GetZoomProperties().bEnabledUserZoom == bUserZoom );
        vp.GetZoomLimits( nMin, nMax );
        CHECK( nMin == 5 );
        CHECK( nMax == 200 );

        pDefault -> GetCameraPosition( nX, nY );
        CHECK( nX == 20 );
        CHECK( nY == 30 );
        pDefault -> GetScrollStepSize( nX, nY );
        CHECK( nX == 6 );
        CHECK( nY == 8 );
        CHECK( pDefault -> GetClearBackground() == true );          // the renderer's flag, as it was

        pExtra -> GetCameraPosition( nX, nY );
        CHECK( nX == 40 );
        CHECK( nY == 50 );
        CHECK( pExtra -> GetViewport().GetZoomProperties().nCurrentZoomPos == 33 );
        CHECK( pExtra -> IsLayerShown( 2 ) == false );

        // Every call is safe.
        for( ViewHandle pView : { pDefault, pExtra } )  {
            SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

            pView -> MoveCameraUp();
            pView -> MoveCameraDown();
            pView -> MoveCameraLeft();
            pView -> MoveCameraRight();
            pView -> ZoomIn();
            pView -> ZoomOut();
            pView -> ResetZoom();
            pView -> ResetCamera();
            pView -> SetCameraPosition( 1, 2 );
            pView -> SetScrollStepSize( 1, 2 );
            pView -> SetClearBackground( false );
            pView -> SetVisible( false );
            pView -> UseMapBackgroundColor();
            CHECK( pView -> TileMapToTileMatrix( SunLight :: TileMap :: stCoordinate2D { 1, 1 }, pos ) == false );
            CHECK( pView -> FitToMap() == false );
            CHECK( pView -> ShowLayer( "ground", true ) == false );
            CHECK( pView -> GetClearBackground() == false );
        }
    }
}
