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
// The viewport is the rectangle [pos, pos + size) - see samples/sprite/world.cpp's own comment.
#define __VIEWPORT_WIDTH            890
#define __VIEWPORT_HEIGHT           790
#define __DEFAULT_ZOOM_SCALE_POS    60
#define __ENABLE_FPS_SHOW_LABEL     true
#define __DEFAULT_MAP_ALIGNMENT     SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT
#define __TMX_MAP_FILE              "resources/map/parallax.tmx"
#define __SUNNY_SPRITE_IDLE         "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SPRITE_IDLE_DELAY   100
#define __SUNNY_TILE_SIZE           32
// "clouds", the LAST layer - not because she is logically part of it, but because a sprite
// draws right after its own container layer's tiles, so attaching her to the topmost layer is
// what keeps her drawn on top of everything, never covered by a drifting cloud landing on her
// fixed screen position (found live: clouds are translucent but still visibly obscured her).
#define __SUNNY_LAYER_ID            4
#define __GAME_NAME                 "Parallax test"
#define __PLAYER_STEP               4         // world pixels moved per frame the key is held


/**
 * @brief Moves Sunny's own logical world position upward, wraps it against the map's own
 * marker rectangle, and recentres the camera on her - every frame the key is held
 * (VIEW_CONTROL_MODE_ACTIVE, set in Run()).
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: MovePlayerUp( SunLight :: Input :: ControllerType type, int nId )  {
    m_fWorldY-=__PLAYER_STEP;
    WrapWorldPosition();
    UpdateCameraOnSunny();
}

/**
 * @brief Moves Sunny's own logical world position downward (see MovePlayerUp).
 */
void World :: MovePlayerDown( SunLight :: Input :: ControllerType type, int nId )  {
    m_fWorldY+=__PLAYER_STEP;
    WrapWorldPosition();
    UpdateCameraOnSunny();
}

/**
 * @brief Moves Sunny's own logical world position left (see MovePlayerUp).
 */
void World :: MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId )  {
    m_fWorldX-=__PLAYER_STEP;
    WrapWorldPosition();
    UpdateCameraOnSunny();
}

/**
 * @brief Moves Sunny's own logical world position right (see MovePlayerUp).
 */
void World :: MovePlayerRight( SunLight :: Input :: ControllerType type, int nId )  {
    m_fWorldX+=__PLAYER_STEP;
    WrapWorldPosition();
    UpdateCameraOnSunny();
}

/**
 * @brief Zoom camera in.
 */
void World :: ZoomIn( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomIn();
    UpdateCameraOnSunny();
    RecenterSunnyOnScreen();
}

/**
 * @brief Zoom camera out.
 */
void World :: ZoomOut( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomOut();
    UpdateCameraOnSunny();
    RecenterSunnyOnScreen();
}

/**
 * @brief Resets camera to default zoom.
 */
void World :: ResetZoom( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ResetZoom();
    UpdateCameraOnSunny();
    RecenterSunnyOnScreen();
}

/**
 * @brief Keeps Sunny's logical position inside the wrap rectangle the map's own marker objects
 * describe, carrying over the exact overflow on a crossing (not just clamping to the edge) so
 * the step she is mid-way through does not stutter - she simply keeps moving, now measured from
 * the opposite edge.
 */
void World :: WrapWorldPosition( void )  {

    if( m_fWorldX > m_fWrapRight )
        m_fWorldX = m_fWrapLeft + ( m_fWorldX - m_fWrapRight );
    else if( m_fWorldX < m_fWrapLeft )
        m_fWorldX = m_fWrapRight - ( m_fWrapLeft - m_fWorldX );

    if( m_fWorldY > m_fWrapBottom )
        m_fWorldY = m_fWrapTop + ( m_fWorldY - m_fWrapBottom );
    else if( m_fWorldY < m_fWrapTop )
        m_fWorldY = m_fWrapBottom - ( m_fWrapTop - m_fWorldY );
}

/**
 * @brief Recentres the camera on Sunny's own logical world position - called after every move,
 * and after every zoom change (the viewport's own half-size, in world pixels, depends on the
 * current zoom). Sunny's own sprite is screen-space and never moves; this is the only thing
 * that makes her look like she is walking.
 */
void World :: UpdateCameraOnSunny( void )  {

    float  fZoom   = m_pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;
    float  fHalfW  = ( __VIEWPORT_WIDTH  / fZoom ) / 2.0f;
    float  fHalfH  = ( __VIEWPORT_HEIGHT / fZoom ) / 2.0f;

    m_pRenderer -> SetCameraPosition( ( int ) ( m_fWorldX - fHalfW ), ( int ) ( m_fWorldY - fHalfH ) );
}

/**
 * @brief Reads the map's own wrap_left/wrap_right/wrap_top/wrap_bottom marker objects - the
 * same mechanism Caravellius already uses on its own side to cycle a map forever between two
 * authored marks; sunlight itself has no built-in map-wrap, so this sample reads the markers
 * through the existing GetObjectByName and does the wrap itself (WrapWorldPosition).
 * @return false if any of the four markers is missing from the map.
 */
bool World :: LoadWrapMarkers( void )  {

    SunLight :: TileMap :: stObject  object;

    if( !m_pRenderer -> GetObjectByName( "wrap_left", object ) )
        return false;
    m_fWrapLeft = ( float ) object.dimension.pos.x;

    if( !m_pRenderer -> GetObjectByName( "wrap_right", object ) )
        return false;
    m_fWrapRight = ( float ) object.dimension.pos.x;

    if( !m_pRenderer -> GetObjectByName( "wrap_top", object ) )
        return false;
    m_fWrapTop = ( float ) object.dimension.pos.y;

    if( !m_pRenderer -> GetObjectByName( "wrap_bottom", object ) )
        return false;
    m_fWrapBottom = ( float ) object.dimension.pos.y;

    return true;
}

/**
 * @brief Keeps Sunny centred on screen, at any zoom. A screen-space canvas's own stored
 * position is NOT a raw screen pixel - it is drawn at "position x zoom" with the camera
 * ignored (BaseCanvas's own documented convention; TextureCanvas::GetScreenRect never adds
 * a camera term for a screen-space canvas, but Viewport::GetClippedRect still multiplies its
 * position by the active zoom same as it would for a world coordinate). So the stored
 * position that lands a given SCREEN pixel is that pixel, minus the viewport's own origin,
 * divided by the current zoom - found live: an earlier, naive attempt stored the actual
 * desired screen pixel directly, which at this sample's own non-integer zoom (3.8125) scaled
 * far outside the viewport, and Viewport::GetClippedRect silently clipped her away entirely
 * (IsOnScreen() == false, confirmed directly, not guessed).
 */
void World :: RecenterSunnyOnScreen( void )  {

    float  fZoom = m_pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;
    float  fScreenCentreX = ( __VIEWPORT_WIDTH  / 2.0f ) - ( __SUNNY_TILE_SIZE / 2.0f );
    float  fScreenCentreY = ( __VIEWPORT_HEIGHT / 2.0f ) - ( __SUNNY_TILE_SIZE / 2.0f );

    SunLight :: Base :: stDimension2D  dim = m_pCanvasSunny -> GetDimension2D();

    dim.pos.x = ( int ) ( fScreenCentreX / fZoom );
    dim.pos.y = ( int ) ( fScreenCentreY / fZoom );

    m_pCanvasSunny -> SetDimension2D( dim );
}

bool World :: LoadSprites( void ) {

    if( m_pCanvasSunny -> Load( __SUNNY_SPRITE_IDLE ) ) {
        SunLight :: Base :: stDimension2D    dim;

        dim.pos.x = 0;      // set for real by RecenterSunnyOnScreen below
        dim.pos.y = 0;
        dim.size.nWidth  = __SUNNY_TILE_SIZE;
        dim.size.nHeight = __SUNNY_TILE_SIZE;

        m_pCanvasSunny -> SetTileSize( __SUNNY_TILE_SIZE );
        m_pCanvasSunny -> SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
        m_pCanvasSunny -> SetDimension2D( dim );
        m_pSpriteSunny -> AddTextureSequence( 0, m_pCanvasSunny.get(), __SUNNY_SPRITE_IDLE_DELAY );
        m_pSpriteSunny -> SetActiveTextureSequence( 0 );
        m_pSpriteSunny -> SetVisible( true );

        if( !m_pRenderer -> AddSprite( __SUNNY_LAYER_ID, *m_pSpriteSunny ) )  {
            fprintf( stderr, "Error adding Sunny's sprite\n" );
            return false;
        }

        // Screen space (SetWorldSpace is never called): fixed at the viewport's own centre for
        // the whole run - what makes her look like she is walking is the camera moving under
        // her, exactly as a classic side-scroller's player sprite stays put on screen.
        RecenterSunnyOnScreen();

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

    m_pRenderer = std :: make_unique<SunLight :: Renderer :: TileMapRenderer>( __DISPLAY_W,
                                                                               __DISPLAY_H,
                                                                               __GAME_NAME,
                                                                               __FRAMES_PER_SECOND );
    m_pSpriteSunny = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasSunny = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
}

/**
 * @brief Run World configuration.
 */
bool World :: Run( void )  {

    SunLight :: Base :: stDimension2D     viewport;

    m_pRenderer -> SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Player movement - direct control of Sunny (AWSD + arrow keys), both axes.
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

    if( !m_pRenderer -> LoadMap( __TMX_MAP_FILE, __DEFAULT_MAP_ALIGNMENT ) )  {
        fprintf( stderr, "Error loading map\n" );
        return false;
    }

    if( !LoadWrapMarkers() )  {
        fprintf( stderr, "Error loading wrap markers (wrap_left/right/top/bottom)\n" );
        return false;
    }

    // Start Sunny in the middle of the wrap rectangle.
    m_fWorldX = ( m_fWrapLeft + m_fWrapRight ) / 2.0f;
    m_fWorldY = ( m_fWrapTop + m_fWrapBottom ) / 2.0f;

    if( !LoadSprites() )  {
        fprintf( stderr, "Error loading sprites\n" );
        return false;
    }

    UpdateCameraOnSunny();

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
