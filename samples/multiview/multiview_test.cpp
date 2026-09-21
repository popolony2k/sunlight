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
 * Multi-view sample: the same map, and the same character, shown in three places at once.
 *
 *   - the MAIN view, the renderer's own (default) view: the whole map;
 *   - a MINIMAP (top right): the whole map, small;
 *   - a CLOSE-UP (right, below it): a zoomed view whose camera FOLLOWS Sunny - it stands still while
 *     Sunny walks inside it and scrolls when Sunny reaches its border.
 *
 * Sunny is ONE sprite, in WORLD space (Sprite::SetWorldSpace): its position is a map position, in pixels,
 * and every view draws it where it draws the map at that position, whatever the view's zoom and camera.
 * That is why nothing here places or duplicates the sprite per view: the sample only moves Sunny (arrow keys /
 * WASD) and, for the close-up, moves that view's camera.
 *
 * Like the other samples it takes its own directory as argv[1] and reads its map and sprite
 * (a copy of the sprite sample's) relative to it:
 *
 *     ./build/samples/multiview/multiview_test samples/multiview/
 */

#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <cstdio>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __TMX_MAP_FILE              "resources/map/test.tmx"
#define __SUNNY_SPRITE_IDLE         "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SIZE                32
#define __SUNNY_STEP                4
#define __SUNNY_ANIMATION_DELAY     100
#define __SUNNY_LAYER_ID            8       // "birb": the last layer, so Sunny is drawn on top of the map
#define __CLOSEUP_ZOOM_POS          46      // factor 2.9375
#define __FOLLOW_MARGIN             28      // map pixels between Sunny and the close-up's border before it scrolls

namespace  {

    SunLight :: TileMap :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: TileMap :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }
}

int main( int argc, char **argv )  {

    if( argc < 2 )  {
        fprintf( stderr, "usage: %s <this sample's directory> (e.g. samples/multiview/)\n", argv[0] );
        return EXIT_FAILURE;
    }

    // Resources are read by name relative to the working directory (see the other samples).
    std :: error_code  errorCode;

    std :: filesystem :: current_path( argv[1], errorCode );

    if( errorCode )  {
        fprintf( stderr, "Cannot enter the sample directory [%s]: %s\n", argv[1], errorCode.message().c_str() );
        return EXIT_FAILURE;
    }

    SunLight :: Renderer :: TileMapRenderer  renderer( __DISPLAY_W, __DISPLAY_H, "Multiview test", __FRAMES_PER_SECOND, false );

    renderer.SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );
    renderer.SetDrawFPS( true );
    renderer.GetViewport().SetDimension2D( Rect( 10, 10, 900, 900 ) );

    if( !renderer.Start() || !renderer.LoadMap( __TMX_MAP_FILE, SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot start / load the map [%s] - is [%s] this sample's own directory (samples/multiview/)?\n", __TMX_MAP_FILE, argv[1] );
        return EXIT_FAILURE;
    }

    SunLight :: TileMap :: stMapInfo  mapInfo {};

    renderer.GetMapInfo( mapInfo );

    const int  nMapWidth  = mapInfo.mapSize.nWidth  * mapInfo.tileSize.nWidth;      // map pixels
    const int  nMapHeight = mapInfo.mapSize.nHeight * mapInfo.tileSize.nHeight;

    // Main view: the whole map (FitToMap zooms to the largest zoom that fits and puts the camera at the map's origin).
    SunLight :: TileMap :: IView  &mainView = renderer.GetDefaultView();

    mainView.FitToMap();

    // (CreateView returns a shared handle: the renderer keeps its own reference, so the view is drawn for as
    // long as it exists, and holding the handle here is always safe.)
    std :: shared_ptr<SunLight :: TileMap :: IView>  pMinimap = renderer.CreateView( Rect( 930, 10, 300, 300 ) );

    pMinimap -> FitToMap();
    pMinimap -> SetBackgroundColor( SunLight :: Base :: stColor { 0, 0, 0, 200 } );

    std :: shared_ptr<SunLight :: TileMap :: IView>  pCloseUp = renderer.CreateView( Rect( 930, 330, 300, 300 ) );

    pCloseUp -> GetViewport().SetPreferredZoom( __CLOSEUP_ZOOM_POS );
    pCloseUp -> GetViewport().SetZoom( __CLOSEUP_ZOOM_POS );
    pCloseUp -> SetBackgroundColor( SunLight :: Base :: stColor { 30, 30, 60, 255 } );

    // Sunny: ONE sprite, in world space. (The canvas is declared first so it is destroyed after the sprite that uses it.)
    SunLight :: Canvas :: TextureCanvas  canvas;
    SunLight :: Sprite :: Sprite         sunny;

    if( !canvas.Load( __SUNNY_SPRITE_IDLE ) )  {
        fprintf( stderr, "Cannot load the sprite [%s]\n", __SUNNY_SPRITE_IDLE );
        return EXIT_FAILURE;
    }

    canvas.SetTileSize( __SUNNY_SIZE );
    canvas.SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
    canvas.SetDimension2D( SunLight :: TileMap :: stDimension2D { { 0, 0 }, { __SUNNY_SIZE, __SUNNY_SIZE } } );
    sunny.AddTextureSequence( 0, &canvas, __SUNNY_ANIMATION_DELAY );
    sunny.SetActiveTextureSequence( 0 );
    sunny.SetVisible( true );
    sunny.SetWorldSpace( true );

    if( !renderer.AddSprite( __SUNNY_LAYER_ID, sunny ) )  {
        fprintf( stderr, "Cannot add the sprite to layer %d\n", __SUNNY_LAYER_ID );
        return EXIT_FAILURE;
    }

    /*
     * Sunny's position is a map position: the sprite's own dimension. All that has to follow it is the close-up's
     * camera: it stays where it is until Sunny comes within __FOLLOW_MARGIN map pixels of the view's border, then
     * scrolls just enough to keep that margin, never past the map (SetCameraPosition does not clamp). The camera is
     * the map point shown at the view's top-left.
     */
    auto  followSunny = [&]()  {
        SunLight :: TileMap :: stDimension2D  &closeUpRect = pCloseUp -> GetViewport().GetDimension2D();
        float  fZoom  = pCloseUp -> GetViewport().GetZoomProperties().fZoomFactor;
        int    nVisW  = ( int ) ( closeUpRect.size.nWidth  / fZoom );      // map pixels visible in the close-up
        int    nVisH  = ( int ) ( closeUpRect.size.nHeight / fZoom );
        int    nSunnyX = sunny.GetDimension2D().pos.x;
        int    nSunnyY = sunny.GetDimension2D().pos.y;
        int    nCamX  = 0, nCamY = 0;

        pCloseUp -> GetCameraPosition( nCamX, nCamY );

        if( nSunnyX - nCamX < __FOLLOW_MARGIN )
            nCamX = nSunnyX - __FOLLOW_MARGIN;
        else if( nSunnyX + __SUNNY_SIZE - nCamX > nVisW - __FOLLOW_MARGIN )
            nCamX = nSunnyX + __SUNNY_SIZE + __FOLLOW_MARGIN - nVisW;

        if( nSunnyY - nCamY < __FOLLOW_MARGIN )
            nCamY = nSunnyY - __FOLLOW_MARGIN;
        else if( nSunnyY + __SUNNY_SIZE - nCamY > nVisH - __FOLLOW_MARGIN )
            nCamY = nSunnyY + __SUNNY_SIZE + __FOLLOW_MARGIN - nVisH;

        nCamX = std :: max( 0, std :: min( nCamX, nMapWidth  - nVisW ) );
        nCamY = std :: max( 0, std :: min( nCamY, nMapHeight - nVisH ) );

        pCloseUp -> SetCameraPosition( nCamX, nCamY );
    };

    auto  moveSunny = [&]( int nDX, int nDY )  {
        SunLight :: TileMap :: stDimension2D  &dim = sunny.GetDimension2D();

        dim.pos.x = std :: max( 0, std :: min( dim.pos.x + nDX, nMapWidth  - __SUNNY_SIZE ) );
        dim.pos.y = std :: max( 0, std :: min( dim.pos.y + nDY, nMapHeight - __SUNNY_SIZE ) );
        followSunny();
    };

    typedef SunLight :: Input :: ControllerType  Controller;

    auto  bind = [&renderer]( SunLight :: Input :: KeyboardKey key, std :: function<void( void )> action )  {
        renderer.SetUserKeyEventHandler( key, [action]( Controller, int ) { action(); } );
    };

    // Arrow keys / WASD: walk Sunny (the main view and the minimap never move; the close-up follows).
    bind( SunLight :: Input :: KEY_UP,    [&]() { moveSunny( 0, -__SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_DOWN,  [&]() { moveSunny( 0,  __SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_LEFT,  [&]() { moveSunny( -__SUNNY_STEP, 0 ); } );
    bind( SunLight :: Input :: KEY_RIGHT, [&]() { moveSunny(  __SUNNY_STEP, 0 ); } );
    bind( SunLight :: Input :: KEY_W,     [&]() { moveSunny( 0, -__SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_S,     [&]() { moveSunny( 0,  __SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_A,     [&]() { moveSunny( -__SUNNY_STEP, 0 ); } );
    bind( SunLight :: Input :: KEY_D,     [&]() { moveSunny(  __SUNNY_STEP, 0 ); } );

    // Page Up / Page Down: zoom the close-up out / in (its camera then re-follows Sunny).
    bind( SunLight :: Input :: KEY_PAGE_UP,   [&]() { pCloseUp -> ZoomOut();  followSunny(); } );
    bind( SunLight :: Input :: KEY_PAGE_DOWN, [&]() { pCloseUp -> ZoomIn();   followSunny(); } );

    // 1 / 2: show or hide the minimap / the close-up. 3: mask the "sky" layer (id 1) out of the MAIN view.
    bind( SunLight :: Input :: KEY_ONE,   [&]() { pMinimap -> SetVisible( !pMinimap -> GetVisible() ); } );
    bind( SunLight :: Input :: KEY_TWO,   [&]() { pCloseUp -> SetVisible( !pCloseUp -> GetVisible() ); } );
    bind( SunLight :: Input :: KEY_THREE, [&]() { mainView.ShowLayer( 1, !mainView.IsLayerShown( 1 ) ); } );

    // 4: bring the minimap in front of / behind the main view.
    bind( SunLight :: Input :: KEY_FOUR,  [&]() { pMinimap -> SetDrawOrder( pMinimap -> GetDrawOrder() >= 0 ? -1 : pMinimap -> GetId() ); } );

    // Sunny starts somewhere in the map; the close-up starts at the map's origin and then follows.
    sunny.GetDimension2D().pos.x = 128;
    sunny.GetDimension2D().pos.y = 128;
    pCloseUp -> SetCameraPosition( 0, 0 );
    followSunny();

    renderer.Run();
    renderer.Stop();

    return EXIT_SUCCESS;
}
