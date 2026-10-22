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
#include <string>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __VIEWPORT_POS_X            20
#define __VIEWPORT_POS_Y            60
#define __VIEWPORT_WIDTH            640     // the whole 20 x 32 px map, at zoom 1
#define __VIEWPORT_HEIGHT           640
#define __ENABLE_FPS_SHOW_LABEL     true
#define __MAP_ALIGNMENT             SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT
#define __TMX_MAP_FILE              "resources/map/test.tmx"
#define __SUNNY_FILE                "resources/sprites/sunny_idle_down.png"
#define __GLOW_FILE                 "resources/sprites/glow.png"
#define __SHADOW_FILE               "resources/sprites/shadow.png"
#define __SUNNY_SIZE                32
#define __GLOW_SIZE                 128
#define __SHADOW_WIDTH              128
#define __SHADOW_HEIGHT             64
#define __SUNNY_START_X             300
#define __SUNNY_START_Y             300
#define __SHADOW_X                  60
#define __SHADOW_Y                  480
#define __PLAYER_MOVE_STEP          4
#define __FONT_FILE                 "/shared/fonts/AnonymousPro-Bold.ttf"

// The map's layer ids the sprites join (with no map they only order the sprites): the shadow over
// the houses (5), the glow (6), under Sunny (7) so it never hides Sunny.
#define __SHADOW_LAYER_ID           5
#define __GLOW_LAYER_ID             6
#define __SUNNY_LAYER_ID            7
#define __GAME_NAME                 "Blend modes test"

namespace  {

    // The panels' background (the map's own sky colour).
    const SunLight :: Base :: stColor  __SKY       { 135, 206, 235, 0xFF };

    // With no map, the window is this dusk blue: a mid tone every mode visibly changes, darker than
    // the panels so their edges show, and dark enough for the white HUD text.
    const SunLight :: Base :: stColor  __DUSK      { 60, 90, 130, 0xFF };

    // The add panel's background: over the bright sky every colour added saturates to white, over
    // a dark one the squares mix (red and green make yellow, all three white).
    const SunLight :: Base :: stColor  __NIGHT     { 25, 25, 40, 0xFF };

    // The four panels, in raw screen pixels: a 2 x 2 grid right of the viewport.
    const int     __PANEL_X0         = 700;
    const int     __PANEL_Y0         = 60;
    const int     __PANEL_SIZE       = 260;
    const int     __PANEL_GAP        = 20;
    const int     __PANEL_COLUMNS    = 2;
    const int     __PANEL_LABEL_GAP  = 6;

    // Each panel: three overlapping squares, red, green and blue, drawn in the panel's mode.
    const int     __SQUARE_SIZE      = 120;
    const int     __SQUARE_INSET     = 40;      // from the panel's top-left corner
    const int     __SQUARE_STEP      = 50;      // how far each square sits from the previous one
    const unsigned char  __SQUARE_ALPHA = 200;

    const int     __HUD_X            = 20;
    const int     __HUD_Y            = 20;
    const int     __HUD_NOTE_Y       = 720;
    const int     __FONT_SIZE        = 20;
    const unsigned char  __OPAQUE    = 0xFF;

    struct stPanel  {
        SunLight :: Base :: BlendMode  mode;
        const char*                    szName;
        SunLight :: Base :: stColor    background;
        SunLight :: Base :: stColor    label;
    };

    const SunLight :: Base :: stColor  __BLACK     { 0, 0, 0, 0xFF };
    const SunLight :: Base :: stColor  __WHITE     { 0xFF, 0xFF, 0xFF, 0xFF };

    const stPanel  __PANELS[] = { { SunLight :: Base :: BLEND_MODE_NORMAL,   "NORMAL",   __SKY,   __BLACK },
                                  { SunLight :: Base :: BLEND_MODE_ADD,      "ADD",      __NIGHT, __WHITE },
                                  { SunLight :: Base :: BLEND_MODE_MULTIPLY, "MULTIPLY", __SKY,   __BLACK },
                                  { SunLight :: Base :: BLEND_MODE_SUBTRACT, "SUBTRACT", __SKY,   __BLACK } };

    const SunLight :: Base :: stColor  __SQUARES[] = { { 230, 60, 60, __SQUARE_ALPHA },
                                                       { 60, 200, 60, __SQUARE_ALPHA },
                                                       { 60, 90, 230, __SQUARE_ALPHA } };
}

/**
 * @brief Move Sunny, and its glow with it.
 * @param nDx Horizontal step, in pixels;
 * @param nDy Vertical step, in pixels;
 */
void World :: MoveSunny( int nDx, int nDy )  {

    SunLight :: Base :: stCoordinate2D  step = { nDx, nDy };

    m_Sunny.sprite.Move( step );
    m_Glow.sprite.Move( step );
}

/**
 * @brief Give the glow a mode of its own, or none (it then draws by whatever mode is in effect).
 * @param bHasMode Whether the glow gets a mode;
 * @param mode The mode, when bHasMode;
 * @param szName The mode's name, for the HUD;
 */
void World :: SetGlowMode( bool bHasMode, SunLight :: Base :: BlendMode mode, const char* szName )  {

    if( bHasMode )
        m_Glow.sprite.SetBlendMode( mode );
    else
        m_Glow.sprite.ClearBlendMode();

    m_strGlowMode = szName;
}

/**
 * @brief The four panels: each one's squares drawn by the game itself between BeginBlendMode and
 * EndBlendMode, over a background (sky; night for add) and under a label, both drawn in normal mode.
 */
void World :: DrawPanels( void )  {

    SunLight :: DrawSurface :: IDrawSurface  &surface = *m_pRenderer;
    int                                       nPanel  = 0;

    for( const stPanel& panel : __PANELS )  {
        int  nX = __PANEL_X0 + ( nPanel % __PANEL_COLUMNS ) * ( __PANEL_SIZE + __PANEL_GAP );
        int  nY = __PANEL_Y0 + ( nPanel / __PANEL_COLUMNS ) * ( __PANEL_SIZE + __PANEL_GAP );
        int  nSquare = 0;

        surface.DrawFilledRectangle( nX, nY, __PANEL_SIZE, __PANEL_SIZE,
                                     panel.background.nRed, panel.background.nGreen, panel.background.nBlue, __OPAQUE );

        surface.BeginBlendMode( panel.mode );

        for( const SunLight :: Base :: stColor& color : __SQUARES )  {
            int  nOffset = __SQUARE_INSET + nSquare * __SQUARE_STEP;

            surface.DrawFilledRectangle( nX + nOffset, nY + nOffset, __SQUARE_SIZE, __SQUARE_SIZE,
                                         color.nRed, color.nGreen, color.nBlue, color.nAlpha );
            nSquare++;
        }

        surface.EndBlendMode();

        surface.DrawText( panel.szName, nX + __PANEL_LABEL_GAP, nY + __PANEL_LABEL_GAP, __FONT_SIZE,
                          panel.label.nRed, panel.label.nGreen, panel.label.nBlue, __OPAQUE );
        nPanel++;
    }
}

/**
 * @brief The HUD: the keys and the glow's mode.
 */
void World :: DrawHud( void )  {

    SunLight :: DrawSurface :: IDrawSurface  &surface = *m_pRenderer;
    std :: string                             strLine = std :: string( m_bUseMap ? "Map" : "No map" ) +
                                                        " - glow: " + m_strGlowMode +
                                                        "   [1] none [2] normal [3] add [4] multiply [5] subtract   arrows move Sunny, Esc quits";

    surface.DrawText( strLine.c_str(), __HUD_X, __HUD_Y, __FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );
    surface.DrawText( "The shadow (bottom left) always multiplies. Its transparent pixels are white: no white box may show.",
                      __HUD_X, __HUD_NOTE_Y, __FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );
}

/**
 * @brief Called by Run() once per frame, inside the frame's render target and after the sprites.
 * @param tileMap The renderer;
 */
void World :: OnUpdate( SunLight :: TileMap :: ITileMap& tileMap )  {

    DrawPanels();
    DrawHud();
}

/**
 * @brief Nothing to release: the sprites are members.
 */
void World :: OnStop( void )  {
}

/**
 * @brief Load one texture into a one-frame sprite and register it.
 * @param rig The sprite and its texture;
 * @param szFile The texture's file;
 * @param nX Left, in the view's pixels;
 * @param nY Top, in the view's pixels;
 * @param nWidth Width, in pixels;
 * @param nHeight Height, in pixels;
 * @param nLayerId The layer it joins;
 * @return Whether it loaded and registered;
 */
bool World :: LoadSprite( stSpriteRig& rig, const char* szFile, int nX, int nY, int nWidth, int nHeight, int nLayerId )  {

    if( !rig.canvas.Load( szFile ) )
        return false;

    rig.canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { nX, nY }, { nWidth, nHeight } } );
    rig.sprite.AddTextureSequence( 0, &rig.canvas );
    rig.sprite.SetActiveTextureSequence( 0 );
    rig.sprite.SetVisible( true );

    return m_pRenderer -> AddSprite( nLayerId, rig.sprite );
}

/**
 * @brief Sunny, its glow centred on it, and the shadow.
 * @return Whether all three loaded;
 */
bool World :: LoadSprites( void )  {

    int  nGlowX = __SUNNY_START_X + ( __SUNNY_SIZE - __GLOW_SIZE ) / 2;
    int  nGlowY = __SUNNY_START_Y + ( __SUNNY_SIZE - __GLOW_SIZE ) / 2;

    if( !LoadSprite( m_Sunny, __SUNNY_FILE, __SUNNY_START_X, __SUNNY_START_Y, __SUNNY_SIZE, __SUNNY_SIZE, __SUNNY_LAYER_ID ) ||
        !LoadSprite( m_Glow, __GLOW_FILE, nGlowX, nGlowY, __GLOW_SIZE, __GLOW_SIZE, __GLOW_LAYER_ID ) ||
        !LoadSprite( m_Shadow, __SHADOW_FILE, __SHADOW_X, __SHADOW_Y, __SHADOW_WIDTH, __SHADOW_HEIGHT, __SHADOW_LAYER_ID ) )
        return false;

    m_Shadow.sprite.SetBlendMode( SunLight :: Base :: BLEND_MODE_MULTIPLY );
    SetGlowMode( true, SunLight :: Base :: BLEND_MODE_ADD, "add" );

    return true;
}

/**
 * @brief Constructor.
 * @param bUseMap Whether to load the map, or draw with no map;
 */
World :: World( bool bUseMap )  {

    m_bUseMap   = bUseMap;
    m_pRenderer = std :: make_unique<SunLight :: Renderer :: TileMapRenderer>( __DISPLAY_W,
                                                                               __DISPLAY_H,
                                                                               __GAME_NAME,
                                                                               __FRAMES_PER_SECOND );
}

/**
 * @brief Set up the keys, the viewport, the map (unless "nomap") and the sprites, then run.
 * @return Whether it ran;
 */
bool World :: Run( void )  {

    SunLight :: Input :: INPUT_EVENT_HANDLER  handlers[] = {
        [this]( SunLight :: Input :: ControllerType, int ) { MoveSunny( 0, -__PLAYER_MOVE_STEP ); },
        [this]( SunLight :: Input :: ControllerType, int ) { MoveSunny( 0, __PLAYER_MOVE_STEP ); },
        [this]( SunLight :: Input :: ControllerType, int ) { MoveSunny( -__PLAYER_MOVE_STEP, 0 ); },
        [this]( SunLight :: Input :: ControllerType, int ) { MoveSunny( __PLAYER_MOVE_STEP, 0 ); } };

    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, handlers[ 0 ] );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, handlers[ 1 ] );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, handlers[ 2 ] );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, handlers[ 3 ] );

    // A key handler runs on every frame the key is held, so each mode has its own key.
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_ONE,
        [this]( SunLight :: Input :: ControllerType, int ) { SetGlowMode( false, SunLight :: Base :: BLEND_MODE_NORMAL, "none" ); } );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_TWO,
        [this]( SunLight :: Input :: ControllerType, int ) { SetGlowMode( true, SunLight :: Base :: BLEND_MODE_NORMAL, "normal" ); } );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_THREE,
        [this]( SunLight :: Input :: ControllerType, int ) { SetGlowMode( true, SunLight :: Base :: BLEND_MODE_ADD, "add" ); } );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_FOUR,
        [this]( SunLight :: Input :: ControllerType, int ) { SetGlowMode( true, SunLight :: Base :: BLEND_MODE_MULTIPLY, "multiply" ); } );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_FIVE,
        [this]( SunLight :: Input :: ControllerType, int ) { SetGlowMode( true, SunLight :: Base :: BLEND_MODE_SUBTRACT, "subtract" ); } );

    m_pRenderer -> GetViewport().SetDimension2D( SunLight :: Base :: stDimension2D { { __VIEWPORT_POS_X, __VIEWPORT_POS_Y },
                                                                                      { __VIEWPORT_WIDTH, __VIEWPORT_HEIGHT } } );
    m_pRenderer -> SetDrawFPS( __ENABLE_FPS_SHOW_LABEL );

    if( !m_bUseMap )
        m_pRenderer -> SetWindowBackgroundColor( __DUSK );

    if( !m_pRenderer -> Start() )  {
        fprintf( stderr, "Cannot start the renderer\n" );
        return false;
    }

    // A readable font for the HUD and the labels (raylib's built-in one is a small bitmap font).
    if( !m_pRenderer -> SetFont( __FONT_FILE ) )
        fprintf( stderr, "Cannot load the font [%s]: using the built-in one\n", __FONT_FILE );

    if( m_bUseMap && !m_pRenderer -> LoadMap( __TMX_MAP_FILE, __MAP_ALIGNMENT ) )  {
        fprintf( stderr, "Error loading map\n" );
        return false;
    }

    if( !LoadSprites() )  {
        fprintf( stderr, "Error loading sprites\n" );
        return false;
    }

    m_pRenderer -> AddTileMapListener( this );
    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
