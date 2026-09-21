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
 * Multi-view sample: the same map shown in three places at once.
 *
 *   - the MAIN view, the renderer's own (default) view: scrolled and zoomed with the keyboard;
 *   - a MINIMAP (top right): the whole map (FitToMap), with the sprite layer masked out;
 *   - a CLOSE-UP (right, below it): a second camera on the map at a higher zoom, sprite included.
 *
 * Like the other samples it takes its own directory as argv[1] and reads its map and sprite
 * (a copy of the sprite sample's) relative to it:
 *
 *     ./build/samples/multiview/multiview_test samples/multiview/
 */

#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <cstdio>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __MAIN_ZOOM_POS             30
#define __TMX_MAP_FILE              "resources/map/test.tmx"
#define __SUNNY_SPRITE_IDLE         "resources/sprites/sunny_idle_down.png"
#define __SUNNY_LAYER_ID            4
#define __CAMERA_STEP               4

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
    renderer.GetViewport().SetPreferredZoom( __MAIN_ZOOM_POS );
    renderer.GetViewport().SetZoom( __MAIN_ZOOM_POS );
    renderer.GetViewport().SetDimension2D( Rect( 10, 10, 900, 900 ) );

    if( !renderer.Start() || !renderer.LoadMap( __TMX_MAP_FILE, SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot start / load the map [%s] - is [%s] this sample's own directory (samples/multiview/)?\n", __TMX_MAP_FILE, argv[1] );
        return EXIT_FAILURE;
    }

    // A sprite on layer 4: it belongs to that layer, so a view shows it if and only if it shows the layer.
    SunLight :: Sprite :: Sprite         sprite;
    SunLight :: Canvas :: TextureCanvas  canvas;

    if( !canvas.Load( __SUNNY_SPRITE_IDLE ) )  {
        fprintf( stderr, "Cannot load the sprite\n" );
        return EXIT_FAILURE;
    }

    canvas.SetTileSize( 32 );
    canvas.SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
    canvas.SetDimension2D( SunLight :: TileMap :: stDimension2D { { 100, 100 }, { 32, 32 } } );
    sprite.AddTextureSequence( 0, &canvas, 100 );
    sprite.SetActiveTextureSequence( 0 );
    sprite.SetVisible( true );
    renderer.AddSprite( __SUNNY_LAYER_ID, sprite );

    // The minimap: the whole map, without the sprite's layer.
    // (CreateView returns a shared handle: the renderer keeps its own reference, so the view is
    // drawn for as long as it exists, and holding the handle here is always safe.)
    std :: shared_ptr<SunLight :: TileMap :: IView>  pMinimap = renderer.CreateView( Rect( 930, 10, 300, 300 ) );

    pMinimap -> FitToMap();
    pMinimap -> ShowLayer( __SUNNY_LAYER_ID, false );
    pMinimap -> SetBackgroundColor( SunLight :: Base :: stColor { 0, 0, 0, 200 } );

    // The close-up: its own camera and a higher zoom, everything shown.
    std :: shared_ptr<SunLight :: TileMap :: IView>  pCloseUp = renderer.CreateView( Rect( 930, 330, 300, 300 ) );

    pCloseUp -> GetViewport().SetZoom( 45 );
    pCloseUp -> SetBackgroundColor( SunLight :: Base :: stColor { 30, 30, 60, 255 } );

    typedef SunLight :: Input :: ControllerType  Controller;

    auto  bind = [&renderer]( SunLight :: Input :: KeyboardKey key, std :: function<void( void )> action )  {
        renderer.SetUserKeyEventHandler( key, [action]( Controller, int ) { action(); } );
    };

    // Main view: arrows / WASD scroll (sprite-relative inversion, like the other samples), PageUp/PageDown zoom.
    bind( SunLight :: Input :: KEY_UP,        [&]() { renderer.MoveCameraDown(); } );
    bind( SunLight :: Input :: KEY_DOWN,      [&]() { renderer.MoveCameraUp(); } );
    bind( SunLight :: Input :: KEY_LEFT,      [&]() { renderer.MoveCameraRight(); } );
    bind( SunLight :: Input :: KEY_RIGHT,     [&]() { renderer.MoveCameraLeft(); } );
    bind( SunLight :: Input :: KEY_PAGE_UP,   [&]() { renderer.ZoomOut(); } );
    bind( SunLight :: Input :: KEY_PAGE_DOWN, [&]() { renderer.ZoomIn(); } );

    // Close-up camera: W A S D.
    bind( SunLight :: Input :: KEY_W, [&]() { pCloseUp -> MoveCameraDown(); } );
    bind( SunLight :: Input :: KEY_S, [&]() { pCloseUp -> MoveCameraUp(); } );
    bind( SunLight :: Input :: KEY_A, [&]() { pCloseUp -> MoveCameraRight(); } );
    bind( SunLight :: Input :: KEY_D, [&]() { pCloseUp -> MoveCameraLeft(); } );

    // 1 / 2: show or hide the minimap / the close-up. 3: mask the "sky" layer (id 1) out of the MAIN view.
    bind( SunLight :: Input :: KEY_ONE, [&]() { pMinimap -> SetVisible( !pMinimap -> GetVisible() ); } );
    bind( SunLight :: Input :: KEY_TWO, [&]() { pCloseUp -> SetVisible( !pCloseUp -> GetVisible() ); } );
    bind( SunLight :: Input :: KEY_THREE, [&]() { SunLight :: TileMap :: IView &view = renderer.GetDefaultView();  view.ShowLayer( 1, !view.IsLayerShown( 1 ) ); } );

    // 4: bring the minimap in front of / behind the main view.
    bind( SunLight :: Input :: KEY_FOUR, [&]() { pMinimap -> SetDrawOrder( pMinimap -> GetDrawOrder() >= 0 ? -1 : pMinimap -> GetId() ); } );

    renderer.SetScrollStepSize( __CAMERA_STEP, __CAMERA_STEP );
    pCloseUp -> SetScrollStepSize( __CAMERA_STEP, __CAMERA_STEP );

    renderer.Run();
    renderer.Stop();

    return EXIT_SUCCESS;
}
