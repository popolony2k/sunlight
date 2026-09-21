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
 * Views: the default view that every renderer has, additional views created over the same world,
 * and - the point - that operating on any view runs the renderer's OWN camera/zoom logic against
 * that view's state and leaves every other view untouched. Nothing here needs a window: a null-
 * backend renderer over a tiny in-memory TMX.
 */

#include <doctest/doctest.h>
#include <memory>
#include "renderer/tilemaprenderer.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    SunLight :: TileMap :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: TileMap :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // Null-backend renderer with the default view's viewport set; optionally with a 640 x 640 map (40 x 40 tiles of 16) loaded.
    std :: unique_ptr<TileMapRenderer> MakeRenderer( const SunLight :: TileMap :: stDimension2D &viewport, unsigned nZoomPos, bool bLoadMap )  {

        RendererConfig  config;

        config.backend     = RENDERER_BACKEND_NULL;
        config.framePacing = FRAME_PACING_UNLIMITED;
        config.fWidth      = 1260.0f;
        config.fHeight     = 920.0f;
        config.nZoomPos    = nZoomPos;
        config.viewport    = viewport;

        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        if( bLoadMap )
            REQUIRE( pRenderer -> LoadMap( "maps/square.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        return pRenderer;
    }
}

TEST_SUITE( "renderer/views" )  {

    TEST_CASE( "The default view is view 0, always there, and shares the renderer's own viewport" )  {

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 10, 10, 1240, 900 ), 60, false );

        CHECK( pRenderer -> GetViewCount() == 1 );
        CHECK( pRenderer -> GetDefaultView().GetId() == 0 );
        CHECK( pRenderer -> GetView( 0 ) == &pRenderer -> GetDefaultView() );

        // One object, not a copy: what the renderer's GetViewport() shows is what the view shows.
        CHECK( &pRenderer -> GetDefaultView().GetViewport() == &pRenderer -> GetViewport() );
        CHECK( pRenderer -> GetDefaultView().GetViewport().GetDimension2D().size.nWidth == 1240 );
        CHECK( pRenderer -> GetDefaultView().GetViewport().GetZoomProperties().nCurrentZoomPos == 60 );

        // Removing it is refused.
        CHECK( pRenderer -> RemoveView( 0 ) == false );
        CHECK( pRenderer -> GetViewCount() == 1 );

        pRenderer -> Stop();
    }

    TEST_CASE( "Operating on the default view IS operating on the renderer" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 10, 10, 300, 300 ), 15, true );
        SunLight :: TileMap :: IView        &view     = pRenderer -> GetDefaultView();
        int                                 nX = 0, nY = 0;

        pRenderer -> SetCameraPosition( 30, 40 );
        view.GetCameraPosition( nX, nY );
        CHECK( nX == 30 );
        CHECK( nY == 40 );

        view.SetCameraPosition( 7, 9 );
        pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == 7 );
        CHECK( nY == 9 );

        // Same clamped scrolling either way.
        view.MoveCameraLeft();
        pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == 7 + 16 );

        pRenderer -> MoveCameraRight();
        view.GetCameraPosition( nX, nY );
        CHECK( nX == 7 );

        // Zoom too.
        float  fBefore = pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;

        view.ZoomIn();
        CHECK( pRenderer -> GetViewport().GetZoomProperties().fZoomFactor == fBefore + SunLight :: Base :: ZOOM_STEP );

        pRenderer -> Stop();
    }

    TEST_CASE( "CreateView / GetView / RemoveView: ids, lookup, counting" )  {

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 0, 0, 800, 600 ), 15, false );

        int  nFirst  = pRenderer -> CreateView( Rect( 900, 20, 200, 200 ) );
        int  nSecond = pRenderer -> CreateView( Rect( 10, 300, 100, 100 ) );

        CHECK( nFirst > 0 );
        CHECK( nSecond > nFirst );
        CHECK( pRenderer -> GetViewCount() == 3 );

        REQUIRE( pRenderer -> GetView( nFirst ) != nullptr );
        CHECK( pRenderer -> GetView( nFirst ) -> GetId() == nFirst );
        CHECK( pRenderer -> GetView( nSecond ) -> GetId() == nSecond );
        CHECK( pRenderer -> GetView( 999 ) == nullptr );

        // The rectangle it was created with.
        SunLight :: TileMap :: stDimension2D  &dim = pRenderer -> GetView( nFirst ) -> GetViewport().GetDimension2D();

        CHECK( dim.pos.x == 900 );
        CHECK( dim.pos.y == 20 );
        CHECK( dim.size.nWidth == 200 );
        CHECK( dim.size.nHeight == 200 );

        // It is its own viewport, not the renderer's.
        CHECK( &pRenderer -> GetView( nFirst ) -> GetViewport() != &pRenderer -> GetViewport() );

        CHECK( pRenderer -> RemoveView( nFirst ) == true );
        CHECK( pRenderer -> GetView( nFirst ) == nullptr );
        CHECK( pRenderer -> GetView( nSecond ) != nullptr );
        CHECK( pRenderer -> GetViewCount() == 2 );
        CHECK( pRenderer -> RemoveView( nFirst ) == false );        // already gone

        // Ids are never reused.
        int  nThird = pRenderer -> CreateView( Rect( 0, 0, 10, 10 ) );

        CHECK( nThird > nSecond );

        pRenderer -> Stop();
    }

    TEST_CASE( "A view's camera, zoom and viewport are its own: operating on one leaves the others exactly as they were" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 10, 10, 300, 300 ), 15, true );
        SunLight :: TileMap :: IView        &defaultView = pRenderer -> GetDefaultView();

        int  nId = pRenderer -> CreateView( Rect( 500, 20, 100, 100 ) );
        SunLight :: TileMap :: IView  &other = *pRenderer -> GetView( nId );

        defaultView.SetCameraPosition( 5, 6 );
        other.SetCameraPosition( 100, 200 );
        other.ZoomIn();
        other.ZoomIn();

        int  nX = 0, nY = 0;

        // The default view kept ITS state through everything done to the other one...
        pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == 5 );
        CHECK( nY == 6 );
        CHECK( defaultView.GetViewport().GetZoomProperties().nCurrentZoomPos == 15 );
        CHECK( defaultView.GetViewport().GetDimension2D().pos.x == 10 );

        // ...and the other kept its own.
        other.GetCameraPosition( nX, nY );
        CHECK( nX == 100 );
        CHECK( nY == 200 );
        CHECK( other.GetViewport().GetZoomProperties().nCurrentZoomPos == 17 );
        CHECK( other.GetViewport().GetDimension2D().pos.x == 500 );

        // The renderer is back on its own root viewport, not left pointing at the other view's.
        CHECK( &pRenderer -> GetViewport() == &defaultView.GetViewport() );

        pRenderer -> Stop();
    }

    TEST_CASE( "An additional view runs the renderer's own camera logic: same viewport and zoom, identical scrolling step by step" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        // Both views get the SAME rectangle and zoom (position 30 = 1.9375), on a map bigger than either.
        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 10, 10, 300, 200 ), 30, true );
        SunLight :: TileMap :: IView        &defaultView = pRenderer -> GetDefaultView();
        int                                 nId = pRenderer -> CreateView( Rect( 10, 10, 300, 200 ) );
        SunLight :: TileMap :: IView        &other = *pRenderer -> GetView( nId );

        other.GetViewport().SetZoom( 30 );

        long  nCompared   = 0;
        long  nMismatches = 0;
        long  nMoved      = 0;

        for( int nIter = 0; nIter < 200; nIter++ )  {
            int  nDirection = nIter % 4;
            int  nX1 = 0, nY1 = 0, nX2 = 0, nY2 = 0;

            switch( nDirection )  {
                case 0 :  defaultView.MoveCameraLeft();   other.MoveCameraLeft();   break;
                case 1 :  defaultView.MoveCameraUp();     other.MoveCameraUp();     break;
                case 2 :  defaultView.MoveCameraRight();  other.MoveCameraRight();  break;
                case 3 :  defaultView.MoveCameraDown();   other.MoveCameraDown();   break;
            }

            defaultView.GetCameraPosition( nX1, nY1 );
            other.GetCameraPosition( nX2, nY2 );

            nCompared++;

            if( ( nX1 != nX2 ) || ( nY1 != nY2 ) )
                nMismatches++;

            if( ( nX1 != 0 ) || ( nY1 != 0 ) )
                nMoved++;
        }

        // Then the harder, one-directional run into the clamps.
        for( int nIter = 0; nIter < 300; nIter++ )  {
            int  nX1 = 0, nY1 = 0, nX2 = 0, nY2 = 0;

            defaultView.MoveCameraLeft();
            defaultView.MoveCameraUp();
            other.MoveCameraLeft();
            other.MoveCameraUp();

            defaultView.GetCameraPosition( nX1, nY1 );
            other.GetCameraPosition( nX2, nY2 );

            nCompared++;

            if( ( nX1 != nX2 ) || ( nY1 != nY2 ) )
                nMismatches++;
        }

        CHECK( nCompared == 500 );
        CHECK( nMoved > 20 );               // the cameras really moved (not vacuous)
        CHECK( nMismatches == 0 );

        pRenderer -> Stop();
    }

    TEST_CASE( "A view's scroll step is the map's tile size: for a view made after the map, and for one made before it" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 0, 0, 200, 200 ), 15, false );

        // Made BEFORE any map is loaded: its step is unresolved until LoadMap.
        int  nBefore = pRenderer -> CreateView( Rect( 0, 0, 200, 200 ) );

        REQUIRE( pRenderer -> LoadMap( "maps/square.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        // Made AFTER: resolved at once.
        int  nAfter = pRenderer -> CreateView( Rect( 0, 0, 200, 200 ) );

        for( int nId : { nBefore, nAfter } )  {
            SunLight :: TileMap :: IView  &view = *pRenderer -> GetView( nId );
            int                           nX = 0, nY = 0;

            view.MoveCameraLeft();
            view.GetCameraPosition( nX, nY );

            INFO( "view " << nId );
            CHECK( nX == 16 );                  // one 16-pixel tile
            CHECK( nY == 0 );
        }

        pRenderer -> Stop();
    }

    TEST_CASE( "TileMapToTileMatrix through a view uses THAT view's viewport position and camera" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 10, 40, 500, 500 ), 15, true );
        int                                 nId = pRenderer -> CreateView( Rect( 64, 32, 200, 200 ) );
        SunLight :: TileMap :: IView        &other = *pRenderer -> GetView( nId );
        SunLight :: TileMap :: stCoordinate2D    coord { 100, 100 };
        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

        // Default view (pos (10, 40), camera 0): col (100 + 10) / 16, row (100 + 40) / 16.
        REQUIRE( pRenderer -> GetDefaultView().TileMapToTileMatrix( coord, pos ) == true );
        CHECK( pos.nTileCol == ( 100 + 10 ) / 16 );
        CHECK( pos.nTileRow == ( 100 + 40 ) / 16 );

        // The other view (pos (64, 32)) with its own camera at (-16, -32): the point 100,100 is a different tile.
        other.SetCameraPosition( 16, 32 );
        pos = SunLight :: TileMap :: stMatrixPosition { -1, -1 };

        REQUIRE( other.TileMapToTileMatrix( coord, pos ) == true );
        CHECK( pos.nTileCol == ( 100 + 64 + 16 ) / 16 );      // camera x is -16 -> minus it
        CHECK( pos.nTileRow == ( 100 + 32 + 32 ) / 16 );

        // And the renderer's (= the default view's) answer is unchanged by having asked the other.
        pos = SunLight :: TileMap :: stMatrixPosition { -1, -1 };
        REQUIRE( pRenderer -> TileMapToTileMatrix( coord, pos ) == true );
        CHECK( pos.nTileCol == ( 100 + 10 ) / 16 );
        CHECK( pos.nTileRow == ( 100 + 40 ) / 16 );

        pRenderer -> Stop();
    }

    TEST_CASE( "GetScrollStepSize reads back each view's own step: unresolved as -1, then the map's tile size, or what was set" )  {

        MemoryFileSystemFixture  fixture;
        fixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 40, 16 );

        std :: unique_ptr<TileMapRenderer>  pRenderer = MakeRenderer( Rect( 0, 0, 200, 200 ), 15, false );
        SunLight :: TileMap :: IView        &defaultView = pRenderer -> GetDefaultView();
        int                                 nId = pRenderer -> CreateView( Rect( 0, 0, 200, 200 ) );
        SunLight :: TileMap :: IView        &other = *pRenderer -> GetView( nId );
        int                                 nW = 0, nH = 0;

        // Nothing loaded yet: both are still "the map's tile size, not known yet".
        defaultView.GetScrollStepSize( nW, nH );
        CHECK( nW == -1 );
        CHECK( nH == -1 );
        other.GetScrollStepSize( nW, nH );
        CHECK( nW == -1 );
        CHECK( nH == -1 );

        // A set value reads back, per view, without touching the other one.
        other.SetScrollStepSize( 3, 5 );
        other.GetScrollStepSize( nW, nH );
        CHECK( nW == 3 );
        CHECK( nH == 5 );
        defaultView.GetScrollStepSize( nW, nH );
        CHECK( nW == -1 );
        CHECK( nH == -1 );

        // Loading the map resolves the -1 ones (both views) and leaves the explicit one alone.
        REQUIRE( pRenderer -> LoadMap( "maps/square.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        defaultView.GetScrollStepSize( nW, nH );
        CHECK( nW == 16 );
        CHECK( nH == 16 );
        other.GetScrollStepSize( nW, nH );
        CHECK( nW == 3 );
        CHECK( nH == 5 );

        // The renderer's own getter is the default view's.
        pRenderer -> SetScrollStepSize( 7, 9 );
        defaultView.GetScrollStepSize( nW, nH );
        CHECK( nW == 7 );
        CHECK( nH == 9 );
        pRenderer -> GetScrollStepSize( nW, nH );
        CHECK( nW == 7 );
        CHECK( nH == 9 );

        // And what it reads back is what MoveCamera* actually moves by.
        other.SetCameraPosition( 100, 100 );
        other.MoveCameraLeft();

        int  nX = 0, nY = 0;

        other.GetCameraPosition( nX, nY );
        CHECK( nX == 100 + 3 );

        pRenderer -> Stop();
    }
}
