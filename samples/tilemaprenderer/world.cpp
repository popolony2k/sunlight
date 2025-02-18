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

#include "world.h"

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __H_SCROLL_STEP_SIZE        1
#define __W_SCROLL_STEP_SIZE        1
#define __VIEWPORT_POS_X            10
#define __VIEWPORT_POS_Y            10
#define __VIEWPORT_WIDTH            900
#define __VIEWPORT_HEIGHT           800
#define __DEFAULT_ZOOM_SCALE_POS    60
#define __ENABLE_FPS_SHOW_LABEL     true
#define __DEFAULT_MAP_ALIGNMENT     SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_CENTER
#define __TMX_MAP_FILE              "/map/cloud_map.tmx"
#define __GAME_NAME                 "TileMap renderer test"


 
void World :: MoveCameraUp( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> MoveCameraUp();
}

void World :: MoveCameraDown( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> MoveCameraDown();
}

void World :: MoveCameraLeft( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> MoveCameraLeft();
}

void World :: MoveCameraRight( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> MoveCameraRight();
}

void World :: ZoomIn( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomIn();
}

void World :: ZoomOut( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomOut();
}

void World :: ResetZoom( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ResetZoom();
}

World :: World( std :: string strBasePath )  {

    m_strBasePath = strBasePath;
    m_pRenderer = new SunLight :: Renderer :: TileMapRenderer( __DISPLAY_W,
                                                               __DISPLAY_H,
                                                               __GAME_NAME,
                                                               __FRAMES_PER_SECOND,
                                                               false );
}

World :: ~World( void )  {
    delete m_pRenderer;
}

bool World :: Run( void )  {

    std :: string                            strMapFile;
    SunLight :: TileMap :: stDimension2D     viewport;
    
    m_pRenderer -> SetScrollStepSize( __W_SCROLL_STEP_SIZE, __H_SCROLL_STEP_SIZE );
    m_pRenderer -> SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Setup keyboard controller
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_S, std :: bind( &World :: MoveCameraUp, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_W, std :: bind( &World :: MoveCameraDown, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_D, std :: bind( &World :: MoveCameraLeft, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_A, std :: bind( &World :: MoveCameraRight, this, std :: placeholders::_1, std :: placeholders :: _2 ) );

    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &World :: MoveCameraUp, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &World :: MoveCameraDown, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &World :: MoveCameraLeft, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &World :: MoveCameraRight, this, std :: placeholders::_1, std :: placeholders :: _2 ) );

    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_UP, std :: bind( &World :: ZoomOut, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_DOWN, std :: bind( &World :: ZoomIn, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_HOME, std :: bind( &World :: ResetZoom, this, std :: placeholders::_1, std :: placeholders :: _2 ) );

    viewport.pos.x = __VIEWPORT_POS_X;
    viewport.pos.y = __VIEWPORT_POS_Y;
    viewport.size.nWidth  = __VIEWPORT_WIDTH;
    viewport.size.nHeight = __VIEWPORT_HEIGHT;

    m_pRenderer -> GetViewport().SetPreferredZoom( __DEFAULT_ZOOM_SCALE_POS );
    m_pRenderer -> GetViewport().SetZoom( __DEFAULT_ZOOM_SCALE_POS );
    m_pRenderer -> GetViewport().SetDimension2D( viewport );
    m_pRenderer -> SetDrawFPS( __ENABLE_FPS_SHOW_LABEL );
    m_pRenderer -> Start();

    strMapFile = m_strBasePath + __TMX_MAP_FILE;

    if( !m_pRenderer -> LoadMap( strMapFile.c_str(), __DEFAULT_MAP_ALIGNMENT ) )  {
        perror("Error loading map\n" );
        return false;
    } 

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
