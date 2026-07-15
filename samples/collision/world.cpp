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
#define __HOUSE_IMAGE               "resources/map/images/house.png"
#define __HOUSE_LAYER_ID            5
#define __PLAYER_MOVE_STEP          2
#define __GAME_NAME                 "Collision test"


/**
 * @brief Moves Sunny up (direct control - unlike the camera-scroll samples,
 * there's no inversion here: Sunny is on screen, so "up" moves Sunny up).
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: MovePlayerUp( SunLight :: Input :: ControllerType type, int nId )  {
    m_LastStep = { 0, -__PLAYER_MOVE_STEP };
    m_pSpriteSunny -> Move( m_LastStep );
}

/**
 * @brief Moves Sunny down.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: MovePlayerDown( SunLight :: Input :: ControllerType type, int nId )  {
    m_LastStep = { 0, __PLAYER_MOVE_STEP };
    m_pSpriteSunny -> Move( m_LastStep );
}

/**
 * @brief Moves Sunny left.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId )  {
    m_LastStep = { -__PLAYER_MOVE_STEP, 0 };
    m_pSpriteSunny -> Move( m_LastStep );
}

/**
 * @brief Moves Sunny right.
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: MovePlayerRight( SunLight :: Input :: ControllerType type, int nId )  {
    m_LastStep = { __PLAYER_MOVE_STEP, 0 };
    m_pSpriteSunny -> Move( m_LastStep );
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

/**
 * @brief Called by the CollisionManager whenever two registered colliders
 * overlap - here, Sunny (collider layer __SUNNY_LAYER_ID) and the house
 * (collider layer __HOUSE_LAYER_ID), paired via AddColliderToColliderRule()
 * in LoadSprites(). A real game would trigger gameplay logic here (damage,
 * pickups, opening a door, ...); this sample simply blocks movement by
 * undoing the step that caused the overlap, giving the house solid,
 * wall-like collision.
 */
void World :: OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: Collision :: Collider *pSecond )  {

    SunLight :: TileMap :: stCoordinate2D  revert = { -m_LastStep.x, -m_LastStep.y };

    m_pSpriteSunny -> Move( revert );
}

/**
 * @brief Unused in this sample - only a collider-to-collider rule is
 * registered (see LoadSprites()), no collider-to-tile-layer rule.
 */
void World :: OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: TileMap :: stTile *pSecond )  {
}

bool World :: LoadSprites( void ) {

    if( m_pCanvasSunny -> Load( m_strBasePath + __SUNNY_SPRITE_IDLE ) &&
        m_pCanvasHouse -> Load( m_strBasePath + __HOUSE_IMAGE ) ) {
        SunLight :: TileMap :: stDimension2D    dimSunny;
        SunLight :: TileMap :: stDimension2D    dimHouse;

        dimSunny.pos.x = 100;
        dimSunny.pos.y = 100;
        dimSunny.size.nWidth  = 32;
        dimSunny.size.nHeight = 32;

        m_pCanvasSunny -> SetTileSize( 32 );
        m_pCanvasSunny -> SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
        m_pCanvasSunny -> SetDimension2D( dimSunny );
        m_pSpriteSunny -> AddTextureSequence( 0, m_pCanvasSunny.get(), __SUNNY_SPRITE_IDLE_DELAY );
        m_pSpriteSunny -> SetActiveTextureSequence( 0 );
        m_pSpriteSunny -> SetVisible( true );

        // tileset_house.tsx packs 10 house tiles into a 5x2 grid; pinning
        // the canvas to tile 0 (manual animation mode, the default) gives
        // a static, non-animated obstacle - a plain house.
        dimHouse.pos.x = 180;
        dimHouse.pos.y = 100;
        dimHouse.size.nWidth  = 32;
        dimHouse.size.nHeight = 32;

        m_pCanvasHouse -> SetTileSize( 32 );
        m_pCanvasHouse -> SetDimension2D( dimHouse );
        m_pSpriteHouse -> AddTextureSequence( 0, m_pCanvasHouse.get() );
        m_pSpriteHouse -> SetActiveTextureSequence( 0 );
        m_pSpriteHouse -> SetVisible( true );

        m_pRenderer -> AddSprite( __SUNNY_LAYER_ID, *m_pSpriteSunny );
        m_pRenderer -> AddSprite( __HOUSE_LAYER_ID, *m_pSpriteHouse );

        // Every sprite added via AddSprite() is automatically registered
        // with the renderer's CollisionManager, keyed by the same layer id
        // (see TileMapRenderer::AddSprite()) - pairing the two layers here
        // is all that's needed to start checking Sunny against the house.
        m_pRenderer -> GetCollisionManager().AddColliderToColliderRule( __SUNNY_LAYER_ID, __HOUSE_LAYER_ID );
        m_pRenderer -> GetCollisionManager().AddCollisionListener( this );

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
    m_LastStep     = { 0, 0 };
    m_pRenderer = std :: make_unique<SunLight :: Renderer :: TileMapRenderer>( __DISPLAY_W,
                                                                               __DISPLAY_H,
                                                                               __GAME_NAME,
                                                                               __FRAMES_PER_SECOND,
                                                                               false );
    m_pSpriteSunny = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasSunny = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
    m_pSpriteHouse = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasHouse = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
}

/**
 * @brief Run World configuration.
 */
bool World :: Run( void )  {

    std :: string                            strMapFile;
    SunLight :: TileMap :: stDimension2D     viewport;

    m_pRenderer -> SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Player movement - direct control of Sunny (AWSD + arrow keys)
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_W, std :: bind( &World :: MovePlayerUp, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &World :: MovePlayerUp, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_S, std :: bind( &World :: MovePlayerDown, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &World :: MovePlayerDown, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_A, std :: bind( &World :: MovePlayerLeft, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &World :: MovePlayerLeft, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_D, std :: bind( &World :: MovePlayerRight, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &World :: MovePlayerRight, this, std :: placeholders::_1, std :: placeholders :: _2 ) );

    //zoom configuration
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
