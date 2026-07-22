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
#include <cmath>

using namespace SunLight :: Scripting;

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
#define __SUNNY_SPRITE_IMAGE        "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SPRITE_IDLE_DELAY   100
#define __SUNNY_LAYER_ID            4
#define __MONKEY_IMAGE              "resources/map/images/monke_variants.png"
#define __MONKEY_LAYER_ID           7
#define __SONG_FILE                 "resources/sounds/stage_theme.wav"
#define __GAME_NAME                 "ScriptProcessor test"

// Ids used by the script - these are the "each id means a sprite/stage
// action" hooks: LOAD_STAGE_CMD/MOVE_SPRITES_TO_SCREEN_CMD/PLAY_SONG_CMD
// carry one of these as their nParm, and World::OnCommand() switches on it.
#define __STAGE_ID                  1
#define __SONG_ID                   1
#define __SUNNY_SPRITE_ID           1
#define __MONKEY_SPRITE_ID          2

// Script labels - arbitrary ids, only meaningful as LOOP_CMD/LABEL_CMD
// targets for their matching END_LOOP_CMD/GOTO_LABEL_CMD.
#define __IDLE_LABEL_ID             1
#define __IDLE_LOOP_LABEL_ID        2
#define __IDLE_LOOP_COUNT           2

#define __SUNNY_Y                   90
#define __SUNNY_TARGET_X            60
#define __MONKEY_Y                  140
#define __MONKEY_TARGET_X           160
#define __OFFSCREEN_LEFT_X          -60
#define __OFFSCREEN_RIGHT_X         300
#define __SLIDE_STEP                3

// Sunny's idle "walk in a circle" animation, centered on her own entrance
// target position. 2*pi hardcoded rather than using M_PI, which isn't
// defined by <cmath> on MSVC without _USE_MATH_DEFINES.
#define __SUNNY_CIRCLE_RADIUS       35
#define __SUNNY_CIRCLE_STEP         0.02f
#define __TWO_PI                    6.28318530718f

// The monkey's idle "patrol a rectangle" animation - her entrance target
// (__MONKEY_TARGET_X, __MONKEY_Y) is the rectangle's top-left corner.
#define __MONKEY_RECT_LEFT          __MONKEY_TARGET_X
#define __MONKEY_RECT_TOP           __MONKEY_Y
#define __MONKEY_RECT_RIGHT         ( __MONKEY_RECT_LEFT + 40 )
#define __MONKEY_RECT_BOTTOM        ( __MONKEY_RECT_TOP + 30 )
#define __MONKEY_PATROL_STEP        2

#define __INTRO_WAIT_MS             1000
#define __ENTRANCE_GAP_MS           400
#define __PAUSE_WAIT_MS             500
#define __IDLE_LOOP_WAIT_MS         2000


/**
 * @brief Zoom camera in.
 */
void World :: ZoomIn( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomIn();
}

/**
 * @brief Zoom camera out.
 */
void World :: ZoomOut( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ZoomOut();
}

/**
 * @brief Resets camera to default zoom.
 */
void World :: ResetZoom( SunLight :: Input :: ControllerType type, int nId )  {
    m_pRenderer -> ResetZoom();
}

/**
 * @brief Stops the stage theme. Wired directly to a key instead of the
 * script: the demo script settles into an infinite idle loop via
 * GOTO_LABEL_CMD (see BuildScript()) and so never reaches a scripted
 * STOP_SONG_CMD. Dispatches through the exact same OnCommand() a script
 * would use, so STOP_SONG_CMD's handling is still genuinely exercised.
 */
void World :: StopSongKey( SunLight :: Input :: ControllerType type, int nId )  {
    OnCommand( STOP_SONG_CMD, __SONG_ID );
}

/**
 * @brief IScriptListener - handles every command ScriptProcessor dispatches
 * to a listener. WAIT_CMD, WAIT_SPRITES_QUEUE_EMPTY, LOOP_CMD, END_LOOP_CMD,
 * LABEL_CMD, and GOTO_LABEL_CMD are all control-flow commands handled
 * internally by ScriptProcessor itself and never reach here.
 */
void World :: OnCommand( SunLight :: Scripting :: Commands cmd, uint16_t nEventId )  {

    switch( cmd )  {
        case LOAD_STAGE_CMD :
            LoadStage( nEventId );
        break;

        case MOVE_SPRITES_TO_SCREEN_CMD :
            StartSpriteEntrance( nEventId );
        break;

        case PLAY_SONG_CMD :
            m_SoundManager.Play( nEventId );
        break;

        case PAUSE_SONG_CMD :
            m_SoundManager.Pause( nEventId );
        break;

        case RESUME_SONG_CMD :
            m_SoundManager.Resume( nEventId );
        break;

        case STOP_SONG_CMD :
            m_SoundManager.Stop( nEventId );
        break;

        default :
            // PLAY_SONG_DIRECT_CMD/PAUSE_SONG_DIRECT_CMD/RESUME_SONG_DIRECT_CMD/
            // STOP_SONG_DIRECT_CMD are intentionally not used here - they're
            // a no-op case inside ScriptProcessor::Run() itself ("Not handled
            // by scripts"), meant to be called directly rather than queued.
        break;
    }
}

/**
 * @brief IScriptListener - reports a script runtime error (e.g. a
 * GOTO_LABEL_CMD/END_LOOP_CMD referencing a label that was never reached).
 */
void World :: OnError( std :: string strError )  {
    fprintf( stderr, "[script] %s\n", strError.c_str() );
}

/**
 * @brief ITileMapListener - called once per frame. Advances any in-flight
 * sprite entrance animation, releases WAIT_SPRITES_QUEUE_EMPTY once none
 * are left in flight, then pumps the script one step.
 */
void World :: OnUpdate( SunLight :: TileMap :: ITileMap& tileMap )  {

    if( m_bSunnySliding )  {
        if( m_pSpriteSunny -> GetDimension2D().pos.x >= m_nSunnyTargetX )  {
            m_bSunnySliding  = false;
            m_bSunnyCircling = true;
        }
        else  {
            SunLight :: TileMap :: stCoordinate2D  step = { __SLIDE_STEP, 0 };
            m_pSpriteSunny -> Move( step );
        }
    }
    else  {
        if( m_bSunnyCircling )
            UpdateSunnyCircle();
    }

    if( m_bMonkeySliding )  {
        if( m_pSpriteMonkey -> GetDimension2D().pos.x <= m_nMonkeyTargetX )  {
            m_bMonkeySliding    = false;
            m_bMonkeyPatrolling = true;
            m_nMonkeyPatrolEdge = 0;
        }
        else  {
            SunLight :: TileMap :: stCoordinate2D  step = { -__SLIDE_STEP, 0 };
            m_pSpriteMonkey -> Move( step );
        }
    }
    else  {
        if( m_bMonkeyPatrolling )
            UpdateMonkeyPatrol();
    }

    // Harmless to call even when the script isn't currently paused on
    // WAIT_SPRITES_QUEUE_EMPTY - it just clears a flag that may already
    // be false.
    if( !m_bSunnySliding && !m_bMonkeySliding )
        m_Script.ResetWaitSpritesQueueEmptyCmd();

    m_Script.Run();
}

/**
 * @brief ITileMapListener - nothing to clean up here beyond what
 * TileMapRenderer::Stop() already does.
 */
void World :: OnStop( void )  {
}

/**
 * @brief LOAD_STAGE_CMD handler. Resets both sprites just off-screen
 * (left/right respectively) and hidden - Viewport::GetClippedRect()
 * naturally excludes anything outside the visible world-coordinate range
 * from drawing, so this alone keeps them out of view until
 * MOVE_SPRITES_TO_SCREEN_CMD slides them in.
 */
void World :: LoadStage( uint16_t nStageId )  {

    switch( nStageId )  {
        case __STAGE_ID :
            m_pSpriteSunny -> SetDimension2D( SunLight :: TileMap :: stDimension2D { { __OFFSCREEN_LEFT_X, __SUNNY_Y }, { 32, 32 } } );
            m_pSpriteMonkey -> SetDimension2D( SunLight :: TileMap :: stDimension2D { { __OFFSCREEN_RIGHT_X, __MONKEY_Y }, { 32, 32 } } );
            m_pSpriteSunny -> SetVisible( false );
            m_pSpriteMonkey -> SetVisible( false );
            m_bSunnySliding     = false;
            m_bMonkeySliding    = false;
            m_bSunnyCircling    = false;
            m_fSunnyCircleAngle = 0.0f;
            m_bMonkeyPatrolling = false;
            m_nMonkeyPatrolEdge = 0;
            printf( "[stage] Stage %d loaded\n", nStageId );
            fflush( stdout );
        break;

        default :
            fprintf( stderr, "[stage] Unknown stage id %d\n", nStageId );
        break;
    }
}

/**
 * @brief MOVE_SPRITES_TO_SCREEN_CMD handler. Resets the requested sprite to
 * its off-screen start position and starts its slide-in animation.
 */
void World :: StartSpriteEntrance( uint16_t nSpriteId )  {

    switch( nSpriteId )  {
        case __SUNNY_SPRITE_ID :
            m_pSpriteSunny -> SetDimension2D( SunLight :: TileMap :: stDimension2D { { __OFFSCREEN_LEFT_X, __SUNNY_Y }, { 32, 32 } } );
            m_pSpriteSunny -> SetVisible( true );
            m_bSunnySliding  = true;
            m_bSunnyCircling = false;
            printf( "[sprite] Sunny entrance triggered\n" );
            fflush( stdout );
        break;

        case __MONKEY_SPRITE_ID :
            m_pSpriteMonkey -> SetDimension2D( SunLight :: TileMap :: stDimension2D { { __OFFSCREEN_RIGHT_X, __MONKEY_Y }, { 32, 32 } } );
            m_pSpriteMonkey -> SetVisible( true );
            m_bMonkeySliding    = true;
            m_bMonkeyPatrolling = false;
            printf( "[sprite] Monkey entrance triggered\n" );
            fflush( stdout );
        break;

        default :
            fprintf( stderr, "[sprite] Unknown sprite id %d\n", nSpriteId );
        break;
    }
}

/**
 * @brief Advances Sunny one step along a circular path centered on her own
 * entrance target position - called once per frame from OnUpdate() once
 * her slide-in finishes, and runs forever after that. Not driven by the
 * script at all; it's a plain per-frame animation, independent of
 * BuildScript()'s own idle LOOP_CMD/GOTO_LABEL_CMD cycle.
 */
void World :: UpdateSunnyCircle( void )  {

    SunLight :: TileMap :: stDimension2D  dim = m_pSpriteSunny -> GetDimension2D();

    m_fSunnyCircleAngle += __SUNNY_CIRCLE_STEP;

    if( m_fSunnyCircleAngle > __TWO_PI )
        m_fSunnyCircleAngle -= __TWO_PI;

    dim.pos.x = __SUNNY_TARGET_X + ( int ) ( __SUNNY_CIRCLE_RADIUS * cosf( m_fSunnyCircleAngle ) );
    dim.pos.y = __SUNNY_Y        + ( int ) ( __SUNNY_CIRCLE_RADIUS * sinf( m_fSunnyCircleAngle ) );

    m_pSpriteSunny -> SetDimension2D( dim );
}

/**
 * @brief Advances the monkey one step around a rectangular patrol path
 * (her entrance target is the rectangle's top-left corner) - called once
 * per frame from OnUpdate() once her slide-in finishes, and runs forever
 * after that, same as UpdateSunnyCircle().
 */
void World :: UpdateMonkeyPatrol( void )  {

    SunLight :: TileMap :: stDimension2D  dim = m_pSpriteMonkey -> GetDimension2D();

    switch( m_nMonkeyPatrolEdge )  {
        case 0 :  // top edge, walking right
            dim.pos.x += __MONKEY_PATROL_STEP;
            if( dim.pos.x >= __MONKEY_RECT_RIGHT )  {
                dim.pos.x = __MONKEY_RECT_RIGHT;
                m_nMonkeyPatrolEdge = 1;
            }
        break;

        case 1 :  // right edge, walking down
            dim.pos.y += __MONKEY_PATROL_STEP;
            if( dim.pos.y >= __MONKEY_RECT_BOTTOM )  {
                dim.pos.y = __MONKEY_RECT_BOTTOM;
                m_nMonkeyPatrolEdge = 2;
            }
        break;

        case 2 :  // bottom edge, walking left
            dim.pos.x -= __MONKEY_PATROL_STEP;
            if( dim.pos.x <= __MONKEY_RECT_LEFT )  {
                dim.pos.x = __MONKEY_RECT_LEFT;
                m_nMonkeyPatrolEdge = 3;
            }
        break;

        case 3 :  // left edge, walking up
            dim.pos.y -= __MONKEY_PATROL_STEP;
            if( dim.pos.y <= __MONKEY_RECT_TOP )  {
                dim.pos.y = __MONKEY_RECT_TOP;
                m_nMonkeyPatrolEdge = 0;
            }
        break;
    }

    m_pSpriteMonkey -> SetDimension2D( dim );
}

/**
 * @brief Builds and compiles the demo script - see world.h's class comment
 * for the full outline. Exercises every ScriptProcessor command except the
 * *_DIRECT_CMD family (not meant to be queued - see OnCommand()).
 */
void World :: BuildScript( void )  {

    m_Script.AddOneParmCmd( LOAD_STAGE_CMD, __STAGE_ID );
    m_Script.AddOneParmCmd( WAIT_CMD, __INTRO_WAIT_MS );
    m_Script.AddOneParmCmd( PLAY_SONG_CMD, __SONG_ID );
    m_Script.AddOneParmCmd( MOVE_SPRITES_TO_SCREEN_CMD, __SUNNY_SPRITE_ID );
    m_Script.AddOneParmCmd( WAIT_CMD, __ENTRANCE_GAP_MS );
    m_Script.AddOneParmCmd( MOVE_SPRITES_TO_SCREEN_CMD, __MONKEY_SPRITE_ID );
    m_Script.AddNoParmCmd( WAIT_SPRITES_QUEUE_EMPTY );
    m_Script.AddOneParmCmd( PAUSE_SONG_CMD, __SONG_ID );
    m_Script.AddOneParmCmd( WAIT_CMD, __PAUSE_WAIT_MS );
    m_Script.AddOneParmCmd( RESUME_SONG_CMD, __SONG_ID );

    // Idle segment: Sunny is walking her own continuous circle by now (see
    // UpdateSunnyCircle(), started once her entrance slide finishes) - this
    // part of the script is just a periodic "heartbeat" running alongside
    // her, still exercising LOOP_CMD/END_LOOP_CMD/LABEL_CMD/GOTO_LABEL_CMD.
    // __IDLE_LOOP_COUNT+1 waits happen per pass (one initial pass plus
    // __IDLE_LOOP_COUNT jumps back), then GOTO_LABEL_CMD repeats the whole
    // segment forever - a script has no conditional branch other than
    // LOOP_CMD's counter, so an unconditional backward jump like this is
    // inherently an infinite "attract mode" loop, not a bounded one.
    m_Script.AddOneParmCmd( LABEL_CMD, __IDLE_LABEL_ID );
    m_Script.AddTwoParmsCmd( LOOP_CMD, __IDLE_LOOP_LABEL_ID, __IDLE_LOOP_COUNT );
    m_Script.AddOneParmCmd( WAIT_CMD, __IDLE_LOOP_WAIT_MS );
    m_Script.AddOneParmCmd( END_LOOP_CMD, __IDLE_LOOP_LABEL_ID );
    m_Script.AddOneParmCmd( GOTO_LABEL_CMD, __IDLE_LABEL_ID );

    m_Script.Compile();
}

bool World :: LoadSprites( void ) {

    if( m_pCanvasSunny -> Load( m_strBasePath + __SUNNY_SPRITE_IMAGE ) &&
        m_pCanvasMonkey -> Load( m_strBasePath + __MONKEY_IMAGE ) ) {

        m_pCanvasSunny -> SetTileSize( 32 );
        m_pCanvasSunny -> SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
        m_pSpriteSunny -> AddTextureSequence( 0, m_pCanvasSunny.get(), __SUNNY_SPRITE_IDLE_DELAY );
        m_pSpriteSunny -> SetActiveTextureSequence( 0 );
        m_pSpriteSunny -> SetVisible( false );

        // monke_variants.png is a 4x4 Tiled tileset grid - tile 0 (manual
        // animation mode, the default) works directly as a static, properly
        // square sprite via TextureCanvas :: SetTileSize(). See the
        // collision sample for the full explanation.
        m_pCanvasMonkey -> SetTileSize( 32 );
        m_pSpriteMonkey -> AddTextureSequence( 0, m_pCanvasMonkey.get() );
        m_pSpriteMonkey -> SetActiveTextureSequence( 0 );
        m_pSpriteMonkey -> SetVisible( false );

        m_pRenderer -> AddSprite( __SUNNY_LAYER_ID, *m_pSpriteSunny );
        m_pRenderer -> AddSprite( __MONKEY_LAYER_ID, *m_pSpriteMonkey );

        if( !m_SoundManager.Load( __SONG_ID, m_strBasePath + __SONG_FILE ) )  {
            fprintf( stderr, "Warning: could not load stage theme sound\n" );
        }

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
    m_pSpriteSunny  = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasSunny  = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
    m_pSpriteMonkey = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasMonkey = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();

    m_bSunnySliding     = false;
    m_nSunnyTargetX     = __SUNNY_TARGET_X;
    m_bMonkeySliding    = false;
    m_nMonkeyTargetX    = __MONKEY_TARGET_X;
    m_bSunnyCircling    = false;
    m_fSunnyCircleAngle = 0.0f;
    m_bMonkeyPatrolling = false;
    m_nMonkeyPatrolEdge = 0;
}

/**
 * @brief Run World configuration.
 */
bool World :: Run( void )  {

    std :: string                            strMapFile;
    SunLight :: TileMap :: stDimension2D     viewport;

    m_pRenderer -> SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Zoom controls, plus a manual stop for the looping demo's music -
    // see StopSongKey()'s comment for why STOP_SONG_CMD isn't scripted.
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_UP, std :: bind( &World :: ZoomOut, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_DOWN, std :: bind( &World :: ZoomIn, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_HOME, std :: bind( &World :: ResetZoom, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_M, std :: bind( &World :: StopSongKey, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    viewport.pos.x = __VIEWPORT_POS_X;
    viewport.pos.y = __VIEWPORT_POS_Y;
    viewport.size.nWidth  = __VIEWPORT_WIDTH;
    viewport.size.nHeight = __VIEWPORT_HEIGHT;

    m_pRenderer -> GetViewport().SetPreferredZoom( __DEFAULT_ZOOM_SCALE_POS );
    m_pRenderer -> GetViewport().SetZoom( __DEFAULT_ZOOM_SCALE_POS );
    m_pRenderer -> GetViewport().SetDimension2D( viewport );
    m_pRenderer -> SetDrawFPS( __ENABLE_FPS_SHOW_LABEL );
    m_pRenderer -> AddTileMapListener( this );
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

    m_Script.AddScriptListener( this );
    BuildScript();

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
