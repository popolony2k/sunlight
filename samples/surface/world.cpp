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
#include <cmath>
#include <cstdio>
#include <vector>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __VIEWPORT_POS_X            40
#define __VIEWPORT_POS_Y            40
#define __VIEWPORT_WIDTH            1000
#define __VIEWPORT_HEIGHT           800
#define __ENABLE_FPS_SHOW_LABEL     true
#define __SUNNY_SPRITE_IDLE         "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SPRITE_IDLE_DELAY   100
#define __SUNNY_TILE_SIZE           32
#define __SUNNY_LAYER_ID            1       // no map: just a grouping id for AddSprite (M2)
#define __SUNNY_START_X             484
#define __SUNNY_START_Y             640
#define __PLAYER_MOVE_STEP          4
#define __GAME_NAME                 "Drawing surface test (no map)"

namespace  {

    const SunLight :: Base :: stColor  __CYAN    { 0x40, 0xD0, 0xE0, 0xFF };
    const SunLight :: Base :: stColor  __ORANGE  { 0xF0, 0x90, 0x30, 0xFF };
    const SunLight :: Base :: stColor  __GREEN   { 0x70, 0xD0, 0x60, 0xFF };
    const SunLight :: Base :: stColor  __MAGENTA { 0xE0, 0x50, 0xC0, 0xFF };
    const SunLight :: Base :: stColor  __YELLOW  { 0xF0, 0xE0, 0x50, 0xFF };

    // Shape layout, in map units (at this sample's zoom 1.0, map units are viewport pixels).
    const double  __SQUARE_CX          = 160.0;   // the rotating square turns about its own centre
    const double  __SQUARE_CY          = 160.0;
    const double  __SQUARE_HALF        = 70.0;
    const double  __SQUARE_THICKNESS   = 4.0;
    const double  __SQUARE_SPIN        = 1.0;     // degrees per frame

    const double  __RING_X             = 330.0;
    const double  __RING_Y             = 100.0;
    const double  __RING_W             = 220.0;
    const double  __RING_H             = 120.0;
    const double  __RING_THICKNESS     = 6.0;
    const double  __RING_SPIN          = 0.5;     // degrees per frame

    const double  __RECT_X             = 640.0;
    const double  __RECT_Y             = 90.0;
    const double  __RECT_W             = 260.0;
    const double  __RECT_H             = 140.0;
    const double  __RECT_THICKNESS     = 3.0;

    const double  __WAVE_X0            = 60.0;
    const double  __WAVE_Y             = 400.0;
    const double  __WAVE_LENGTH        = 880.0;
    const double  __WAVE_STEP          = 16.0;    // distance between consecutive wave points
    const double  __WAVE_AMPLITUDE     = 50.0;
    const double  __WAVE_PERIOD        = 220.0;   // map units per full sine cycle
    const double  __WAVE_SPEED         = 0.05;    // radians per frame
    const double  __WAVE_THICKNESS     = 3.0;

    const double  __POINTS_X0          = 120.0;
    const double  __POINTS_Y           = 530.0;
    const double  __POINTS_GAP         = 90.0;
    const int     __POINTS_COUNT       = 9;
    const double  __POINT_SIZE_STEP    = 2.0;     // the n-th point is (n + 1) x this, in map units

    const double  __TWO_PI             = 6.283185307179586;

    // HUD, in raw screen pixels (IDrawSurface draws with no viewport/camera transform).
    const int     __HUD_TEXT_X         = 40;
    const int     __HUD_TEXT_Y         = 10;
    const int     __HUD_FONT_SIZE      = 20;
    const int     __HUD_BAR_X          = 40;
    const int     __HUD_BAR_Y          = 860;
    const int     __HUD_BAR_W          = 1000;
    const int     __HUD_BAR_H          = 20;
    const int     __HUD_BAR_PERIOD     = 300;     // frames for the bar to fill once
    const unsigned char  __OPAQUE      = 0xFF;
}

void World :: MoveSunny( int nDx, int nDy )  {

    SunLight :: Base :: stCoordinate2D  step = { nDx, nDy };

    m_pSpriteSunny -> Move( step );
}

void World :: MovePlayerUp( SunLight :: Input :: ControllerType type, int nId )     { MoveSunny( 0, -__PLAYER_MOVE_STEP ); }
void World :: MovePlayerDown( SunLight :: Input :: ControllerType type, int nId )   { MoveSunny( 0, __PLAYER_MOVE_STEP ); }
void World :: MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId )   { MoveSunny( -__PLAYER_MOVE_STEP, 0 ); }
void World :: MovePlayerRight( SunLight :: Input :: ControllerType type, int nId )  { MoveSunny( __PLAYER_MOVE_STEP, 0 ); }

/**
 * @brief The shapes, through IShapeSurface (M1): positions and thickness in map units, converted
 * by the renderer itself - the same drawing code a Tiled shape object uses.
 */
void World :: DrawShapes( void )  {

    SunLight :: DrawSurface :: IShapeSurface  &surface = *m_pRenderer;

    // A thick square, turning about its own centre (the points are relative to (fX, fY), and
    // rotation is about (fX, fY)).
    SunLight :: Base :: stVector2D  square[] = { { ( float ) -__SQUARE_HALF, ( float ) -__SQUARE_HALF },
                                                 { ( float )  __SQUARE_HALF, ( float ) -__SQUARE_HALF },
                                                 { ( float )  __SQUARE_HALF, ( float )  __SQUARE_HALF },
                                                 { ( float ) -__SQUARE_HALF, ( float )  __SQUARE_HALF } };

    surface.DrawPolygon( __SQUARE_CX, __SQUARE_CY, square, 4, __CYAN, __SQUARE_THICKNESS, m_nFrame * __SQUARE_SPIN );

    // A thick, turning ellipse ring and a thick rectangle.
    surface.DrawEllipse( __RING_X, __RING_Y, __RING_W, __RING_H, __ORANGE, __RING_THICKNESS, m_nFrame * __RING_SPIN );
    surface.DrawRectangle( __RECT_X, __RECT_Y, __RECT_W, __RECT_H, __GREEN, __RECT_THICKNESS );

    // A travelling sine wave, as an open thick polyline.
    std :: vector<SunLight :: Base :: stVector2D>  wave;

    for( double fX = 0.0; fX <= __WAVE_LENGTH; fX += __WAVE_STEP )  {
        double  fY = __WAVE_AMPLITUDE * std :: sin( ( fX / __WAVE_PERIOD ) * __TWO_PI - m_nFrame * __WAVE_SPEED );

        wave.push_back( SunLight :: Base :: stVector2D { ( float ) fX, ( float ) fY } );
    }

    surface.DrawPolyline( __WAVE_X0, __WAVE_Y, wave.data(), ( int ) wave.size(), __MAGENTA, __WAVE_THICKNESS );

    // A row of points, each a bit larger than the last.
    for( int nIdx = 0; nIdx < __POINTS_COUNT; nIdx++ )
        surface.DrawPoint( __POINTS_X0 + nIdx * __POINTS_GAP, __POINTS_Y, __YELLOW, ( nIdx + 1 ) * __POINT_SIZE_STEP );
}

/**
 * @brief The HUD, through IDrawSurface: raw screen pixels, outside the viewport.
 */
void World :: DrawHud( void )  {

    SunLight :: DrawSurface :: IDrawSurface  &surface = *m_pRenderer;

    surface.DrawText( "No map loaded - shapes (M1), a sprite (M2) and this HUD. Arrows move Sunny, Esc quits.",
                      __HUD_TEXT_X, __HUD_TEXT_Y, __HUD_FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );

    int  nFill = ( int ) ( ( m_nFrame % __HUD_BAR_PERIOD ) * __HUD_BAR_W / __HUD_BAR_PERIOD );

    surface.DrawFilledRectangle( __HUD_BAR_X, __HUD_BAR_Y, __HUD_BAR_W, __HUD_BAR_H, 0x30, 0x30, 0x30, __OPAQUE );
    surface.DrawFilledRectangle( __HUD_BAR_X, __HUD_BAR_Y, nFill, __HUD_BAR_H, 0x40, 0xD0, 0xE0, __OPAQUE );
}

/**
 * @brief Called by Run() once per frame, inside the frame's render target and after the sprites
 * were drawn - the place a mapless game draws its own content.
 */
void World :: OnUpdate( SunLight :: TileMap :: ITileMap& tileMap )  {

    DrawShapes();
    DrawHud();
    m_nFrame++;
}

void World :: OnStop( void )  {
}

bool World :: LoadSprites( void )  {

    if( !m_pCanvasSunny -> Load( __SUNNY_SPRITE_IDLE ) )
        return false;

    SunLight :: Base :: stDimension2D  dim;

    dim.pos.x = __SUNNY_START_X;
    dim.pos.y = __SUNNY_START_Y;
    dim.size.nWidth  = __SUNNY_TILE_SIZE;
    dim.size.nHeight = __SUNNY_TILE_SIZE;

    m_pCanvasSunny -> SetTileSize( __SUNNY_TILE_SIZE );
    m_pCanvasSunny -> SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
    m_pCanvasSunny -> SetDimension2D( dim );
    m_pSpriteSunny -> AddTextureSequence( 0, m_pCanvasSunny.get(), __SUNNY_SPRITE_IDLE_DELAY );
    m_pSpriteSunny -> SetActiveTextureSequence( 0 );
    m_pSpriteSunny -> SetVisible( true );

    // No map: __SUNNY_LAYER_ID is just a grouping key, and the sprite is drawn anyway (M2).
    return m_pRenderer -> AddSprite( __SUNNY_LAYER_ID, *m_pSpriteSunny );
}

/**
 * @brief Constructor. Initializes class data by reading base path.
 *
 * @param strBasePath base path needed.
 */
World :: World( std :: string strBasePath )  {

    m_nFrame = 0;
    m_pRenderer = std :: make_unique<SunLight :: Renderer :: TileMapRenderer>( __DISPLAY_W,
                                                                               __DISPLAY_H,
                                                                               __GAME_NAME,
                                                                               __FRAMES_PER_SECOND );
    m_pSpriteSunny = std :: make_unique<SunLight :: Sprite :: Sprite>();
    m_pCanvasSunny = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
}

/**
 * @brief Run World configuration. LoadMap is never called.
 */
bool World :: Run( void )  {

    SunLight :: Base :: stDimension2D  viewport;

    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &World :: MovePlayerUp, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &World :: MovePlayerDown, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &World :: MovePlayerLeft, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &World :: MovePlayerRight, this, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    viewport.pos.x = __VIEWPORT_POS_X;
    viewport.pos.y = __VIEWPORT_POS_Y;
    viewport.size.nWidth  = __VIEWPORT_WIDTH;
    viewport.size.nHeight = __VIEWPORT_HEIGHT;

    m_pRenderer -> GetViewport().SetDimension2D( viewport );
    m_pRenderer -> SetDrawFPS( __ENABLE_FPS_SHOW_LABEL );

    if( !m_pRenderer -> Start() )  {
        fprintf( stderr, "Cannot start the renderer\n" );
        return false;
    }

    if( !LoadSprites() )  {
        fprintf( stderr, "Error loading sprites\n" );
        return false;
    }

    m_pRenderer -> AddTileMapListener( this );

    printf( "No map loaded: shapes, a sprite and a HUD on a bare drawing surface. Arrows move Sunny, Esc quits.\n" );

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
