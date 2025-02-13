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

#include "tilemaprenderer_test.h"
#include "renderer/tilemaprenderer.h"
#include <string>

// FIXME: temporario
SunLight :: Renderer :: TileMapRenderer  *pRenderer = NULL;

void MoveCameraUp( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> MoveCameraUp();
}

void MoveCameraDown( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> MoveCameraDown();
}

void MoveCameraLeft( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> MoveCameraLeft();
}

void MoveCameraRight( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> MoveCameraRight();
}

void ZoomIn( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> ZoomIn();
}

void ZoomOut( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> ZoomOut();
}

void ResetZoom( SunLight :: Input :: ControllerType type, int nId )  {
    pRenderer -> ResetZoom();
}

int main( int argc, char **argv ) {
    std :: string                            strBasePath;
    std :: string                            strMapFile;
    SunLight :: TileMap :: stDimension2D     viewport;
    SunLight :: Renderer :: TileMapRenderer  renderer( DISPLAY_W,
                                                       DISPLAY_H,
                                                       GAME_NAME,
                                                       FRAMES_PER_SECOND,
                                                       false );

    // Check command line arguments
    if( argc < 2 )  {
        perror( "Invalid command line arguments" );
        return EXIT_FAILURE;
    }

    strBasePath = argv[1];  
    pRenderer = &renderer;
    renderer.SetScrollStepSize( W_SCROLL_STEP_SIZE, H_SCROLL_STEP_SIZE );
    renderer.SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Setup keyboard controller
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_S, std :: bind( &MoveCameraUp, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_W, std :: bind( &MoveCameraDown, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_D, std :: bind( &MoveCameraLeft, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_A, std :: bind( &MoveCameraRight, std :: placeholders::_1, std :: placeholders :: _2 ) );

    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &MoveCameraUp, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &MoveCameraDown, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &MoveCameraLeft, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &MoveCameraRight, std :: placeholders::_1, std :: placeholders :: _2 ) );

    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_UP, std :: bind( &ZoomOut, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_DOWN, std :: bind( &ZoomIn, std :: placeholders::_1, std :: placeholders :: _2 ) );
    renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_HOME, std :: bind( &ResetZoom, std :: placeholders::_1, std :: placeholders :: _2 ) );

    viewport.pos.x = VIEWPORT_POS_X;
    viewport.pos.y = VIEWPORT_POS_Y;
    viewport.size.nWidth  = VIEWPORT_WIDTH;
    viewport.size.nHeight = VIEWPORT_HEIGHT;

    renderer.GetViewport().SetPreferredZoom( DEFAULT_ZOOM_SCALE_POS );
    renderer.GetViewport().SetZoom( DEFAULT_ZOOM_SCALE_POS );
    renderer.GetViewport().SetDimension2D( viewport );
    renderer.SetDrawFPS( ENABLE_FPS_SHOW_LABEL );
    renderer.Start();

    strMapFile = strBasePath + TMX_MAP_FILE;

    if( !renderer.LoadMap( strMapFile.c_str(), DEFAULT_MAP_ALIGNMENT ) )  {
        perror("Error loading map\n" );
        return EXIT_FAILURE;
    } 

    renderer.Run();
    renderer.Stop();

    return EXIT_SUCCESS;
}
