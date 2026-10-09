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
#include "general/clock.h"
#include <cstdio>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __VIEWPORT_POS_X            10
#define __VIEWPORT_POS_Y            10
#define __VIEWPORT_WIDTH            890
#define __VIEWPORT_HEIGHT           790
#define __DEFAULT_ZOOM_SCALE_POS    60
#define __ENABLE_FPS_SHOW_LABEL     true
// TOP_LEFT, not CENTER: CENTER sets a non-zero initial camera offset (see LoadMap's alignment
// switch) to visually center the map in the viewport - every position in this file was verified
// against the real renderer at camera (0,0), which only TOP_LEFT leaves untouched.
#define __DEFAULT_MAP_ALIGNMENT     SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT
#define __TMX_MAP_FILE              "resources/map/isometric.tmx"
#define __SUNNY_SPRITE_IDLE         "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SPRITE_IDLE_DELAY   100
#define __SUNNY_TILE_SIZE           32
#define __SUNNY_LAYER_ID            3        // isometric.tmx's "sprites" layer (id="3") - AddSprite
                                              // requires a real tmx layer id, confirmed by reading
                                              // TileMapRenderer::AddSprite's own GetLayer() guard.
#define __OBSTACLE_TILE_LAYER_ID    2        // isometric.tmx's "obstacles" layer (id="2")
// isometric.tmx's own declared size: 8 columns x 6 rows, 64x32 tiles - TryMoveStep's screen
// deltas are derived from the tile size, the same (col - row) * tileWidth/2, (col + row) *
// tileHeight/2 lattice step E1a/E1b confirmed.
#define __MAP_GRID_WIDTH            8
#define __MAP_GRID_HEIGHT           6
#define __MAP_TILE_WIDTH            64
#define __MAP_TILE_HEIGHT           32
// Sunny starts at grid cell (row 4, col 2), walkable ground clear of the obstacle cluster -
// stored coordinate is that cell's own map-pixel position (CellTopLeft, confirmed empirically
// against the real renderer: a screen-space sprite's stored position IS the map-pixel position
// of the cell it stands on, independent of zoom): (col - row + mapHeight - 1) * tileWidth/2,
// (col + row) * tileHeight/2 = (2 - 4 + 5) * 32, (2 + 4) * 16 = (96, 96), plus the same centering
// offset every target position gets (see __SUNNY_CENTER_OFFSET_X below).
#define __SUNNY_START_ROW           4
#define __SUNNY_START_COL           2
// She is 32 wide but a tile is 64 wide (and both 32 tall) - CellTopLeft is a cell's own top-left
// corner, the same anchor convention tiles themselves use, so an unadjusted Sunny sits toward the
// upper-left of her cell rather than centered on its diamond. This offsets her stored X by half
// the leftover width (the full-height case needs no Y offset, since 32 == 32). This is safe to do
// purely cosmetically: nothing in this file derives her cell from her pixel position (TryMoveStep
// tracks m_nSunnyRow/Col explicitly and checks obstacles via a direct GetTile call, not
// TileMapToTileMatrix) - the one remaining consumer of her exact position, the reactive
// AddColliderToTileRule path, is an intentional no-op (see OnCollision).
#define __SUNNY_CENTER_OFFSET_X     ( ( __MAP_TILE_WIDTH - __SUNNY_TILE_SIZE ) / 2 )
#define __SUNNY_START_X             ( 96 + __SUNNY_CENTER_OFFSET_X )
#define __SUNNY_START_Y             96
#define __GAME_NAME                 "Isometric collision test"
// How long a single grid-cell step takes to animate, and also how long a held key's repeated
// firings are ignored for (m_bAnimating) - one coherent step per this many milliseconds.
#define __MOVE_ANIM_MS              180


namespace  {

    SunLight :: Base :: stCoordinate2D CellTopLeft( int nRow, int nCol )  {

        return SunLight :: Base :: stCoordinate2D {
            ( nCol - nRow + __MAP_GRID_HEIGHT - 1 ) * ( __MAP_TILE_WIDTH  / 2 ),
            ( nCol + nRow )                         * ( __MAP_TILE_HEIGHT / 2 ) };
    }
}

/**
 * @brief Moves Sunny by exactly one grid cell along the diamond lattice's own axes, converted to
 * the matching screen diagonal - never a screen-straight pixel nudge. Refused outright if the
 * target cell is past the grid's own edge, or carries an obstacle (checked directly against the
 * "obstacles" layer, not via the reactive AddColliderToTileRule path - see this class's own doc
 * comment for why). Otherwise the step always commits logically (m_nSunnyRow/Col always reaches
 * the target cell) - what differs is only how that is shown: a plain step animates her drawn
 * position there; a step that would carry her past the viewport's own edge instead scrolls the
 * camera (MoveCameraUp/Down/Left/Right, already clamped to the map's own limits) and leaves her
 * drawn position where it is, since the camera motion itself supplies the apparent movement.
 * Committing the logical step either way matters: if a scroll left the logical position
 * unchanged, the very next call would recompute the identical target and hit the identical edge
 * again - no amount of further input could ever make new progress, which is exactly the "stuck"
 * behavior an earlier version of this method had.
 * @param nDeltaRow -1, 0 or +1: the step along the matrix row axis;
 * @param nDeltaCol -1, 0 or +1: the step along the matrix column axis;
 */
void World :: TryMoveStep( int nDeltaRow, int nDeltaCol )  {

    if( m_bAnimating )
        return;

    int  nTargetRow = m_nSunnyRow + nDeltaRow;
    int  nTargetCol = m_nSunnyCol + nDeltaCol;

    // Refuse outright past the grid's own edge - GetTile() answers false for an out-of-bounds
    // cell (see test_tilelookup_bounds.cpp), which the obstacle check below would otherwise read
    // as "no obstacle there", letting Sunny walk off the map entirely.
    if( ( nTargetRow < 0 ) || ( nTargetRow >= __MAP_GRID_HEIGHT ) ||
        ( nTargetCol < 0 ) || ( nTargetCol >= __MAP_GRID_WIDTH ) )
        return;

    // Refuse outright if the target cell is an obstacle (GetTile against the "obstacles" layer; a
    // tile with its own authored collision shape - see tileset_obstacle.tsx - is what
    // AddColliderToTileRule's own Hit() checks for too). Checked before the edge test below, so an
    // obstacle right at the viewport's edge still blocks Sunny instead of being silently skipped.
    SunLight :: TileMap :: stLayer  obstacleLayer;

    if( m_pRenderer -> GetLayer( __OBSTACLE_TILE_LAYER_ID, obstacleLayer ) )  {
        SunLight :: TileMap :: stMatrixPosition  targetPos = { nTargetRow, nTargetCol };
        SunLight :: TileMap :: stTile            tile;

        if( m_pRenderer -> GetTile( targetPos, obstacleLayer, tile ) && ( tile.pTile -> collision != nullptr ) )
            return;
    }

    SunLight :: Base :: stCoordinate2D  target = CellTopLeft( nTargetRow, nTargetCol );

    target.x += __SUNNY_CENTER_OFFSET_X;

    SunLight :: Base :: stDimension2D&  dim    = m_pSpriteSunny -> GetDimension2D();
    SunLight :: Base :: stDimension2D&  vp     = m_pRenderer -> GetViewport().GetDimension2D();
    float                                fZoom  = m_pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;

    // A tile's own drawn screen position is (mapPixel + camera) * zoom + viewport.pos (DrawTile
    // adds camera before the clip/zoom step), but Sunny is a screen-space sprite
    // (TextureCanvas::GetScreenRect adds no camera term at all) - so for her stored position to
    // visually land on a given cell, it must be target + the CURRENT camera, not target alone.
    //
    // GetCameraPosition() reports -m_CameraPos (its own doc comment: "the negated world
    // coordinate... shown at the viewport's top-left"), but DrawTile adds the raw, un-negated
    // m_CameraPos - the reported value needs negating back before use here.
    int  nCamXBeforeReported, nCamYBeforeReported, nCamXAfterReported, nCamYAfterReported;

    m_pRenderer -> GetCameraPosition( nCamXBeforeReported, nCamYBeforeReported );

    int  nCamXBefore = -nCamXBeforeReported;
    int  nCamYBefore = -nCamYBeforeReported;

    // The edge check still needs to know whether to attempt a scroll FIRST, using the
    // not-yet-scrolled camera - same screen-space transform, now camera-correct.
    int  nScreenX    = ( int ) ( ( target.x + nCamXBefore ) * fZoom ) + vp.pos.x;
    int  nScreenY    = ( int ) ( ( target.y + nCamYBefore ) * fZoom ) + vp.pos.y;
    int  nScreenSize = ( int ) ( __SUNNY_TILE_SIZE * fZoom );

    // MoveCameraLeft/Right/Up/Down name which way m_CameraPos itself shifts, not which way the
    // view visually pans - the same "sprite-relative" inversion the orthogonal tilemaprenderer
    // sample already relies on (its own D key, visually "pan right", calls MoveCameraLeft()).
    // Revealing more map to the right needs camera.x more negative, i.e. MoveCameraLeft(); back
    // toward the origin (more map to the left) is MoveCameraRight().
    if( nScreenX < vp.pos.x )
        m_pRenderer -> MoveCameraRight();
    else if( ( nScreenX + nScreenSize ) > ( vp.pos.x + vp.size.nWidth ) )
        m_pRenderer -> MoveCameraLeft();

    if( nScreenY < vp.pos.y )
        m_pRenderer -> MoveCameraDown();
    else if( ( nScreenY + nScreenSize ) > ( vp.pos.y + vp.size.nHeight ) )
        m_pRenderer -> MoveCameraUp();

    m_pRenderer -> GetCameraPosition( nCamXAfterReported, nCamYAfterReported );

    int  nCamXAfter = -nCamXAfterReported;
    int  nCamYAfter = -nCamYAfterReported;

    // MoveCameraX above already applied its own full step in this one call - put the camera back
    // where it was and let OnUpdate replay the same before/after span smoothly instead, in step
    // with Sunny's own glide (see this field's own doc comment in world.h for why).
    m_pRenderer -> SetCameraPosition( nCamXBeforeReported, nCamYBeforeReported );
    m_CamAnimFrom = { nCamXBeforeReported, nCamYBeforeReported };
    m_CamAnimTo   = { nCamXAfterReported, nCamYAfterReported };

    m_nSunnyRow = nTargetRow;
    m_nSunnyCol = nTargetCol;
    m_nAnimStartMs = SunLight :: General :: Clock :: NowMilliseconds();
    m_bAnimating   = true;

    // Always target + the camera as it stands AFTER whatever scrolling just happened - correct
    // whether the camera moved on that axis, partly moved, or hit its own A6 clamp and did not
    // move at all (that last case naturally makes Sunny resume moving herself, since using
    // whatever camera value actually resulted is always right, with no special-casing needed).
    m_AnimFrom = dim.pos;
    m_AnimTo   = { target.x + nCamXAfter, target.y + nCamYAfter };
}

/**
 * @brief Row - 1: the up-right diagonal.
 */
void World :: MovePlayerUp( SunLight :: Input :: ControllerType type, int nId )  {
    TryMoveStep( -1, 0 );
}

/**
 * @brief Row + 1: the down-left diagonal.
 */
void World :: MovePlayerDown( SunLight :: Input :: ControllerType type, int nId )  {
    TryMoveStep( 1, 0 );
}

/**
 * @brief Column - 1: the up-left diagonal.
 */
void World :: MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId )  {
    TryMoveStep( 0, -1 );
}

/**
 * @brief Column + 1: the down-right diagonal.
 */
void World :: MovePlayerRight( SunLight :: Input :: ControllerType type, int nId )  {
    TryMoveStep( 0, 1 );
}

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
 * @brief Intentionally not acted on. AddColliderToTileRule (paired against the "obstacles" layer
 * in LoadSprites()) runs every frame regardless of this sample's own logic, checking whatever
 * position Sunny's collider currently has - including the in-between frames of a smooth animation,
 * where that position is off the lattice's own cell corners. E1b found that isometric cells'
 * bounding boxes overlap their neighbors by half their width/height, so a mid-flight position can
 * resolve to the wrong cell there; reacting to a hit from here (an earlier version of this method
 * did, by reverting the step) could then undo a perfectly valid move because of a spurious hit on
 * an unrelated obstacle a frame or two into its animation. TryMoveStep's own proactive GetTile check
 * (grid-exact, no animation involved) is the actual, reliable gate; this callback is kept, and
 * still fires, purely so AddColliderToTileRule stays demonstrated as genuinely wired up and
 * working (see tests/test_collisionmanager.cpp for the rigorous, non-animated proof of that).
 */
void World :: OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: TileMap :: stTile *pSecond )  {
}

/**
 * @brief Unused in this sample - only a collider-to-tile rule is registered (see LoadSprites()).
 */
void World :: OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: Collision :: Collider *pSecond )  {
}

/**
 * @brief Advances Sunny's own walk animation (TryMoveStep already decided it's clear to move;
 * this only interpolates her drawn position toward the target cell over __MOVE_ANIM_MS), and the
 * camera's own scroll in step with it - MoveCameraX applied its full step in one instant call, so
 * TryMoveStep put the camera back to where it started and left the before/after span here, to be
 * replayed smoothly over the same span instead of snapping the world in a single frame while
 * Sunny is still visibly gliding to her own target.
 */
void World :: OnUpdate( SunLight :: TileMap :: ITileMap& tileMap )  {

    if( !m_bAnimating )
        return;

    int64_t  nNowMs   = SunLight :: General :: Clock :: NowMilliseconds();
    float    fElapsed = ( float ) ( nNowMs - m_nAnimStartMs );
    float    fT        = fElapsed / ( float ) __MOVE_ANIM_MS;

    SunLight :: Base :: stDimension2D&  dim = m_pSpriteSunny -> GetDimension2D();

    if( fT >= 1.0f )  {
        dim.pos      = m_AnimTo;
        m_bAnimating = false;
        m_pRenderer -> SetCameraPosition( m_CamAnimTo.x, m_CamAnimTo.y );
    }
    else  {
        dim.pos.x = m_AnimFrom.x + ( int ) ( ( m_AnimTo.x - m_AnimFrom.x ) * fT );
        dim.pos.y = m_AnimFrom.y + ( int ) ( ( m_AnimTo.y - m_AnimFrom.y ) * fT );

        int  nCamX = m_CamAnimFrom.x + ( int ) ( ( m_CamAnimTo.x - m_CamAnimFrom.x ) * fT );
        int  nCamY = m_CamAnimFrom.y + ( int ) ( ( m_CamAnimTo.y - m_CamAnimFrom.y ) * fT );

        m_pRenderer -> SetCameraPosition( nCamX, nCamY );
    }
}

/**
 * @brief Nothing to clean up on Stop().
 */
void World :: OnStop( void )  {
}

bool World :: LoadSprites( void ) {

    if( m_pCanvasSunny -> Load( __SUNNY_SPRITE_IDLE ) )  {
        SunLight :: Base :: stDimension2D  dimSunny;

        dimSunny.pos.x = __SUNNY_START_X;
        dimSunny.pos.y = __SUNNY_START_Y;
        dimSunny.size.nWidth  = __SUNNY_TILE_SIZE;
        dimSunny.size.nHeight = __SUNNY_TILE_SIZE;

        m_pCanvasSunny -> SetTileSize( __SUNNY_TILE_SIZE );
        m_pCanvasSunny -> SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
        m_pCanvasSunny -> SetDimension2D( dimSunny );
        m_pSpriteSunny -> AddTextureSequence( 0, m_pCanvasSunny.get(), __SUNNY_SPRITE_IDLE_DELAY );
        m_pSpriteSunny -> SetActiveTextureSequence( 0 );
        m_pSpriteSunny -> SetVisible( true );

        m_pRenderer -> AddSprite( __SUNNY_LAYER_ID, *m_pSpriteSunny );

        // Every sprite added via AddSprite() is automatically registered with the renderer's own
        // CollisionManager, keyed by the same layer id - pairing Sunny's layer against the
        // "obstacles" TILE layer (not a sprite layer) is all AddColliderToTileRule needs; Update()
        // then checks Sunny's collider against whatever tile her position resolves to on that
        // layer (see this class's own doc comment for why TryMoveStep also checks proactively).
        m_pRenderer -> GetCollisionManager().AddColliderToTileRule( __SUNNY_LAYER_ID, __OBSTACLE_TILE_LAYER_ID );
        m_pRenderer -> GetCollisionManager().AddCollisionListener( this );
        m_pRenderer -> AddTileMapListener( this );

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
                                                                               __FRAMES_PER_SECOND,
                                                                               false );
    m_pSpriteSunny = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasSunny = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
    m_nSunnyRow    = __SUNNY_START_ROW;
    m_nSunnyCol    = __SUNNY_START_COL;
}

/**
 * @brief Run World configuration.
 */
bool World :: Run( void )  {

    std :: string                            strMapFile;
    SunLight :: Base :: stDimension2D     viewport;

    // Scroll step left at its default (IsometricProjection::DefaultScrollStep, one full tile) -
    // MoveCameraUp/Down/Left/Right below need no override for this sample.
    m_pRenderer -> SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );

    // Player movement - direct control of Sunny (AWSD + arrow keys), one grid cell per key along
    // the diamond lattice's own axes; the camera only moves on its own, once a step would carry
    // her past the viewport's edge (see TryMoveStep).
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_W, std :: bind( &World :: MovePlayerUp, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &World :: MovePlayerUp, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_S, std :: bind( &World :: MovePlayerDown, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &World :: MovePlayerDown, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_A, std :: bind( &World :: MovePlayerLeft, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &World :: MovePlayerLeft, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_D, std :: bind( &World :: MovePlayerRight, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &World :: MovePlayerRight, this, std :: placeholders::_1, std :: placeholders :: _2 ) );

    //zoom configuration - "-" zooms out, "=" zooms in (the same key as "+" on most layouts, no
    //shift needed), Home resets.
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_MINUS, std :: bind( &World :: ZoomOut, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_EQUAL, std :: bind( &World :: ZoomIn, this, std :: placeholders::_1, std :: placeholders :: _2 ) );
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

    strMapFile = __TMX_MAP_FILE;

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
