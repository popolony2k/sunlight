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
#include <cstdio>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __VIEWPORT_POS_X            10
#define __VIEWPORT_POS_Y            10
#define __VIEWPORT_WIDTH            900
#define __VIEWPORT_HEIGHT           800
#define __DEFAULT_ZOOM_SCALE_POS    60
#define __ENABLE_FPS_SHOW_LABEL     true
#define __DEFAULT_MAP_ALIGNMENT     SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_CENTER
#define __TMX_MAP_FILE              "resources/map/test.tmx"
#define __SUNNY_SPRITE_IDLE         "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SPRITE_IDLE_DELAY   100
#define __SUNNY_LAYER_ID            4
#define __MONKEY_IMAGE              "resources/map/images/monke_variants.png"
#define __MONKEY_LAYER_ID           7
#define __PLAYER_MOVE_STEP          2
#define __GAMEPAD_ID                0
#define __GAME_NAME                 "Gamepad test"


/**
 * @brief Moves Sunny up. Triggered by the WASD/arrow-key fallback (KEY_W)
 * and by the gamepad's left stick / DPad up (GAMEPAD_BUTTON_LEFT_FACE_UP).
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveSunnyUp( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { 0, -__PLAYER_MOVE_STEP };
    m_pSpriteSunny -> Move( step );
}

/**
 * @brief Moves Sunny down.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveSunnyDown( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { 0, __PLAYER_MOVE_STEP };
    m_pSpriteSunny -> Move( step );
}

/**
 * @brief Moves Sunny left.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveSunnyLeft( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { -__PLAYER_MOVE_STEP, 0 };
    m_pSpriteSunny -> Move( step );
}

/**
 * @brief Moves Sunny right.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveSunnyRight( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { __PLAYER_MOVE_STEP, 0 };
    m_pSpriteSunny -> Move( step );
}

/**
 * @brief Moves the monkey up. Triggered by the arrow-key fallback (KEY_UP)
 * and by the gamepad's right stick / face buttons up (GAMEPAD_BUTTON_RIGHT_FACE_UP,
 * i.e. Y on Xbox / Triangle on PS3).
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveMonkeyUp( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { 0, -__PLAYER_MOVE_STEP };
    m_pSpriteMonkey -> Move( step );
}

/**
 * @brief Moves the monkey down.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveMonkeyDown( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { 0, __PLAYER_MOVE_STEP };
    m_pSpriteMonkey -> Move( step );
}

/**
 * @brief Moves the monkey left.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveMonkeyLeft( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { -__PLAYER_MOVE_STEP, 0 };
    m_pSpriteMonkey -> Move( step );
}

/**
 * @brief Moves the monkey right.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler/SetUserGamePadEventHandler.
 */
void World :: MoveMonkeyRight( SunLight :: Input :: ControllerType type, int nId )  {
    SunLight :: TileMap :: stCoordinate2D  step = { __PLAYER_MOVE_STEP, 0 };
    m_pSpriteMonkey -> Move( step );
}

/**
 * @brief Zoom camera in.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: ZoomIn( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomIn();
}

/**
 * @brief Zoom camera out.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: ZoomOut( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomOut();
}

/**
 * @brief Resets camera to default zoom.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: ResetZoom( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ResetZoom();
}

bool World :: LoadSprites( void ) {

    if( m_pCanvasSunny -> Load( m_strBasePath + __SUNNY_SPRITE_IDLE ) &&
        m_pCanvasMonkey -> Load( m_strBasePath + __MONKEY_IMAGE ) ) {
        SunLight :: TileMap :: stDimension2D    dimSunny;
        SunLight :: TileMap :: stDimension2D    dimMonkey;

        // Sprites don't scroll with the camera in this library - see the
        // collision sample's LoadSprites() for the full explanation. At this
        // sample's zoom (3.8125x) and 900x800 viewport at (10,10), only world
        // coordinates roughly within x:[0,236] y:[0,210] land on screen, so
        // both characters start well inside that range with room to move
        // independently in either direction.
        dimSunny.pos.x = 60;
        dimSunny.pos.y = 60;
        dimSunny.size.nWidth  = 32;
        dimSunny.size.nHeight = 32;

        m_pCanvasSunny -> SetTileSize( 32 );
        m_pCanvasSunny -> SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
        m_pCanvasSunny -> SetDimension2D( dimSunny );
        m_pSpriteSunny -> AddTextureSequence( 0, m_pCanvasSunny.get(), __SUNNY_SPRITE_IDLE_DELAY );
        m_pSpriteSunny -> SetActiveTextureSequence( 0 );
        m_pSpriteSunny -> SetVisible( true );

        // monke_variants.png is a 4x4 Tiled tileset grid (tileset_monke_variants.tsx)
        // of self-contained character icons, so tile 0 (manual animation mode,
        // the default) works directly as a static, properly square sprite via
        // TextureCanvas :: SetTileSize() - see the collision sample for why.
        dimMonkey.pos.x = 160;
        dimMonkey.pos.y = 140;
        dimMonkey.size.nWidth  = 32;
        dimMonkey.size.nHeight = 32;

        m_pCanvasMonkey -> SetTileSize( 32 );
        m_pCanvasMonkey -> SetDimension2D( dimMonkey );
        m_pSpriteMonkey -> AddTextureSequence( 0, m_pCanvasMonkey.get() );
        m_pSpriteMonkey -> SetActiveTextureSequence( 0 );
        m_pSpriteMonkey -> SetVisible( true );

        m_pRenderer -> AddSprite( __SUNNY_LAYER_ID, *m_pSpriteSunny );
        m_pRenderer -> AddSprite( __MONKEY_LAYER_ID, *m_pSpriteMonkey );

        return true;
    }
    return false;
}

/**
 * @brief Constructor. Initializes class data by reading base path.
 *
 * @param strBasePath base path needed.
 */
World :: World( std :: string strBasePath )  {

    m_strBasePath = strBasePath;
    m_pRenderer = std :: make_unique<SunLight :: Renderer :: TileMapRenderer>( __DISPLAY_W,
                                                                               __DISPLAY_H,
                                                                               __GAME_NAME,
                                                                               __FRAMES_PER_SECOND,
                                                                               false );
    m_pSpriteSunny = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasSunny = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
    m_pSpriteMonkey = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasMonkey = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
}

/**
 * @brief Run World configuration.
 */
bool World :: Run( void )  {

    std :: string                            strMapFile;
    SunLight :: TileMap :: stDimension2D     viewport;

    m_pRenderer -> SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Gamepad id 0 must be registered before its events are dispatched -
    // see TileMapRenderer::HandleUserInput(), which only checks ids present
    // in m_GamePadList.
    m_pRenderer -> AddGamePad( __GAMEPAD_ID );

    // Sunny - left stick / DPad (GAMEPAD_BUTTON_LEFT_FACE_*), WASD as a
    // keyboard fallback for testing without a controller connected.
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_W, std :: bind( &World :: MoveSunnyUp, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_S, std :: bind( &World :: MoveSunnyDown, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_A, std :: bind( &World :: MoveSunnyLeft, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_D, std :: bind( &World :: MoveSunnyRight, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_UP, std :: bind( &World :: MoveSunnyUp, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_DOWN, std :: bind( &World :: MoveSunnyDown, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_LEFT, std :: bind( &World :: MoveSunnyLeft, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT, std :: bind( &World :: MoveSunnyRight, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    // Monkey - right stick / face buttons (GAMEPAD_BUTTON_RIGHT_FACE_*), arrow
    // keys as a keyboard fallback.
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &World :: MoveMonkeyUp, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &World :: MoveMonkeyDown, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &World :: MoveMonkeyLeft, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &World :: MoveMonkeyRight, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_UP, std :: bind( &World :: MoveMonkeyUp, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN, std :: bind( &World :: MoveMonkeyDown, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT, std :: bind( &World :: MoveMonkeyLeft, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, std :: bind( &World :: MoveMonkeyRight, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    // Zoom configuration
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_UP, std :: bind( &World :: ZoomOut, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_DOWN, std :: bind( &World :: ZoomIn, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_HOME, std :: bind( &World :: ResetZoom, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

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
        fprintf( stderr, "Error loading map\n" );
        return false;
    }

    if( !LoadSprites() )  {
        fprintf( stderr, "Error loading sprites\n" );
        return false;
    }

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
