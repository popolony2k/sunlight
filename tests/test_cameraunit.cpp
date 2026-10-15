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
 * S1e of the master plan: Camera on its own - position, scroll step, the four moves with their
 * limits, the map alignments and the parallax camera - with no renderer, no view and no map. Every
 * expected value is worked out by hand. test_camera, test_views and test_viewport_semantics pin the
 * same behaviour through the renderer and its views.
 */

#include <doctest/doctest.h>
#include "renderer/camera/camera.h"
#include "tilemap/itilemap.h"

using SunLight :: Renderer :: Camera;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {
        return SunLight :: Base :: stDimension2D { { nX, nY }, { nW, nH } };
    }

    int PosX( const Camera &camera )  { int nX = 0, nY = 0; camera.GetPosition( nX, nY ); return nX; }
    int PosY( const Camera &camera )  { int nX = 0, nY = 0; camera.GetPosition( nX, nY ); return nY; }
}

TEST_SUITE( "renderer/Camera" )  {

    TEST_CASE( "The position is stored negated: SetPosition/GetPosition take and give the map point; Reset goes to the origin" )  {

        Camera  camera;

        CHECK( camera.Position().x == 0.0f );
        CHECK( camera.Position().y == 0.0f );

        camera.SetPosition( 30, -7 );

        CHECK( camera.Position().x == -30.0f );     // the offset every map position gets
        CHECK( camera.Position().y == 7.0f );
        CHECK( PosX( camera ) == 30 );
        CHECK( PosY( camera ) == -7 );

        camera.Reset();
        CHECK( PosX( camera ) == 0 );
        CHECK( PosY( camera ) == 0 );
    }

    TEST_CASE( "The scroll step starts unresolved (-1); ResolveScrollStep fills each unresolved axis only" )  {

        Camera  camera;
        int     nW = 0, nH = 0;

        camera.GetScrollStep( nW, nH );
        CHECK( nW == -1 );
        CHECK( nH == -1 );

        camera.SetScrollStep( 5, -1 );
        camera.ResolveScrollStep( SunLight :: Base :: stSize2D { 16, 8 } );
        camera.GetScrollStep( nW, nH );
        CHECK( nW == 5 );                            // set explicitly: kept
        CHECK( nH == 8 );                            // was -1: the map's

        camera.ResolveScrollStep( SunLight :: Base :: stSize2D { 99, 99 } );
        camera.GetScrollStep( nW, nH );
        CHECK( nW == 5 );
        CHECK( nH == 8 );
    }

    TEST_CASE( "MoveUp/MoveDown: whole steps, a partial last one to the map's bottom edge, and back to the origin" )  {

        // A 100 px tall viewport over a 640 px map at zoom 1: the limit is 100 - 640 = -540, which is
        // 33 steps of 16 and 12 more.
        Camera  camera;

        camera.SetScrollStep( 16, 16 );

        for( int i = 0; i < 33; i++ )
            camera.MoveUp( 100, 1.0f, 640 );

        CHECK( PosY( camera ) == 528 );
        camera.MoveUp( 100, 1.0f, 640 );
        CHECK( PosY( camera ) == 540 );
        camera.MoveUp( 100, 1.0f, 640 );
        CHECK( PosY( camera ) == 540 );

        for( int i = 0; i < 33; i++ )
            camera.MoveDown();

        CHECK( PosY( camera ) == 12 );
        camera.MoveDown();
        CHECK( PosY( camera ) == 0 );
        camera.MoveDown();
        CHECK( PosY( camera ) == 0 );
        CHECK( PosX( camera ) == 0 );
    }

    TEST_CASE( "MoveUp at a fractional zoom stops at the whole map pixel that leaves no gap; a map shorter than the viewport does not scroll" )  {

        // The hexagonal sample: a 790 px viewport at zoom 3.8125 shows 207.2 map px of a 225 px map. The
        // limit, 207.2 - 225 = -17.8, rounds up to -17: the map's edge lands at or past the viewport's.
        Camera  camera;

        camera.SetScrollStep( 60, 50 );
        camera.MoveUp( 790, 3.8125f, 225 );
        CHECK( PosY( camera ) == 17 );
        CHECK( ( ( -17 + 225 ) * 3.8125f ) >= 790.0f );
        CHECK( ( ( -18 + 225 ) * 3.8125f ) < 790.0f );

        Camera  shortMap;

        shortMap.SetScrollStep( 16, 16 );
        shortMap.MoveUp( 1000, 1.0f, 640 );
        CHECK( PosY( shortMap ) == 0 );
    }

    TEST_CASE( "MoveLeft/MoveRight: the horizontal pair, on the staggered sample's numbers" )  {

        // 890 px at zoom 3.8125 show 233.4 map px of 544: the limit -310.6 rounds up to -310, reached
        // after four whole steps of 64 (-256) and a partial one of 54.
        Camera  camera;

        camera.SetScrollStep( 64, 48 );

        for( int i = 0; i < 4; i++ )
            camera.MoveLeft( 890, 3.8125f, 544 );

        CHECK( PosX( camera ) == 256 );
        camera.MoveLeft( 890, 3.8125f, 544 );
        CHECK( PosX( camera ) == 310 );
        camera.MoveLeft( 890, 3.8125f, 544 );
        CHECK( PosX( camera ) == 310 );

        for( int i = 0; i < 4; i++ )
            camera.MoveRight();

        CHECK( PosX( camera ) == 54 );
        camera.MoveRight();
        CHECK( PosX( camera ) == 0 );
        CHECK( PosY( camera ) == 0 );
    }

    TEST_CASE( "Align: each alignment's position; only the axes it names are set" )  {

        // A 640 x 480 map, the viewport at (10, 20), 300 x 200, zoom 2. Its right edge is 310, its
        // bottom 220; the bottom alignments' "visible height" is |(480 / 220) x 220 - 480| = 40.
        SunLight :: Base :: stDimension2D  vp = Rect( 10, 20, 300, 200 );

        auto  aligned = [&vp]( int nAlignment )  {
            Camera  camera;

            camera.SetPosition( 1, 2 );                                      // a position to keep or replace
            camera.Align( nAlignment, 640, 480, vp, 2.0f );

            return camera.Position();
        };

        SunLight :: Base :: stVector2D  pos = aligned( ITM :: MAP_ALIGNMENT_TOP_LEFT );
        CHECK( pos.x == -1.0f );                                             // unchanged
        CHECK( pos.y == -2.0f );

        pos = aligned( ITM :: MAP_ALIGNMENT_TOP_RIGHT );
        CHECK( pos.x == -( 640 - 310 + 10 ) );                               // -340
        CHECK( pos.y == -2.0f );

        pos = aligned( ITM :: MAP_ALIGNMENT_BOTTOM_LEFT );
        CHECK( pos.x == -1.0f );
        CHECK( pos.y == -( 480 - 220 + 20 ) );                               // -280

        pos = aligned( ITM :: MAP_ALIGNMENT_BOTTOM_RIGHT );
        CHECK( pos.x == -340.0f );
        CHECK( pos.y == -( 480 - 40 + 20 ) );                                // -460

        pos = aligned( ITM :: MAP_ALIGNMENT_CENTER );
        CHECK( pos.x == -245.0f );                                           // -round(((1280 - 310 + 10) / 2) / 2)
        CHECK( pos.y == -190.0f );                                           // -round(((960 - 220 + 20) / 2) / 2)

        pos = aligned( ITM :: MAP_ALIGNMENT_CENTER_WIDTH_TOP );
        CHECK( pos.x == -245.0f );
        CHECK( pos.y == -2.0f );

        pos = aligned( ITM :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM );
        CHECK( pos.x == -245.0f );
        CHECK( pos.y == -460.0f );

        pos = aligned( ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT );
        CHECK( pos.x == -1.0f );
        CHECK( pos.y == -190.0f );

        pos = aligned( ITM :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT );
        CHECK( pos.x == -340.0f );
        CHECK( pos.y == -190.0f );
    }

    TEST_CASE( "ForParallax: exactly the camera at factor 1; anchored at the origin and scaled otherwise" )  {

        Camera  camera;

        camera.SetPosition( 100, 60 );                                       // stored as (-100, -60)

        SunLight :: Base :: stVector2D  same = camera.ForParallax( 1.0, 1.0, 10.0, 20.0 );

        CHECK( same.x == -100.0f );
        CHECK( same.y == -60.0f );

        SunLight :: Base :: stVector2D  half = camera.ForParallax( 0.5, 2.0, 10.0, 20.0 );

        CHECK( half.x == -55.0f );                                           // (-100 + 10) x 0.5 - 10
        CHECK( half.y == -100.0f );                                          // (-60 + 20) x 2 - 20
    }
}
