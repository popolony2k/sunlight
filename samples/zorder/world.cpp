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
#define __VIEWPORT_POS_X            40
#define __VIEWPORT_POS_Y            40
#define __VIEWPORT_WIDTH            1000
#define __VIEWPORT_HEIGHT           800
#define __ENABLE_FPS_SHOW_LABEL     true
#define __DEFAULT_MAP_ALIGNMENT     SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT
#define __TMX_MAP_FILE              "resources/map/zorder.tmx"
#define __CARDS_LAYER_ID            1
#define __CARD_SIZE                 120
#define __GAME_NAME                 "Sprite z-order test"

namespace  {

    const char * const  __CARD_IMAGES[World :: __CARD_COUNT] = {
        "resources/sprites/card_red.png",
        "resources/sprites/card_green.png",
        "resources/sprites/card_blue.png",
        "resources/sprites/card_gold.png",
    };

    // Screen-space position of each card (see BaseCanvas: drawn at position x zoom + the
    // viewport's own origin, camera ignored - at this sample's default zoom (1.0, never
    // changed) that is simply position + viewport origin). Staggered diagonally so each one
    // overlaps the next, the only way a draw-order change is visible at all.
    const int  __CARD_X[World :: __CARD_COUNT] = { 280, 330, 380, 430 };
    const int  __CARD_Y[World :: __CARD_COUNT] = { 260, 300, 340, 380 };
}

/**
 * @brief Brings one card to the front of the stack: a strictly higher z-order than every
 * other card has ever had, so the sort in TileMapRenderer::HandleSpriteUpdate always redraws
 * it last (on top) from this frame on - re-pressing the SAME key repeatedly still works,
 * since m_nNextZOrder only ever grows.
 * @param nCardIndex Which of the four cards (0..__CARD_COUNT - 1);
 * @param type Controller type described by SunLight :: Input :: ControllerType.
 * @param nId Controller id defined at SunLight :: Renderer :: TileMapRenderer :: SetUserKeyEventHandler.
 */
void World :: BringToFront( int nCardIndex, SunLight :: Input :: ControllerType type, int nId )  {

    m_Sprites[nCardIndex] -> SetZOrder( m_nNextZOrder++ );
}

bool World :: LoadSprites( void )  {

    for( int nIdx = 0; nIdx < __CARD_COUNT; nIdx++ )  {
        if( !m_Canvases[nIdx] -> Load( __CARD_IMAGES[nIdx] ) )
            return false;

        SunLight :: Base :: stDimension2D  dim;

        dim.pos.x = __CARD_X[nIdx];
        dim.pos.y = __CARD_Y[nIdx];
        dim.size.nWidth  = __CARD_SIZE;
        dim.size.nHeight = __CARD_SIZE;

        m_Canvases[nIdx] -> SetDimension2D( dim );
        m_Sprites[nIdx] -> AddTextureSequence( 0, m_Canvases[nIdx].get() );
        m_Sprites[nIdx] -> SetActiveTextureSequence( 0 );
        m_Sprites[nIdx] -> SetVisible( true );
        // Starting stack order (back to front): red, green, blue, gold - the same order
        // they're added in, matching the plan's own "all sprites at the same z-order draw in
        // the order they were added" default (here each card's OWN initial z-order, 0..3,
        // already encodes that same back-to-front order explicitly).
        m_Sprites[nIdx] -> SetZOrder( nIdx );

        if( !m_pRenderer -> AddSprite( __CARDS_LAYER_ID, *m_Sprites[nIdx] ) )
            return false;
    }

    return true;
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
    m_nNextZOrder = __CARD_COUNT;   // strictly above every card's own starting z-order (0..__CARD_COUNT - 1)

    for( int nIdx = 0; nIdx < __CARD_COUNT; nIdx++ )  {
        m_Sprites[nIdx]  = std :: make_unique<SunLight :: Sprite :: Sprite>();
        m_Canvases[nIdx] = std :: make_unique<SunLight :: Canvas :: TextureCanvas>();
    }
}

/**
 * @brief Run World configuration.
 */
bool World :: Run( void )  {

    SunLight :: Base :: stDimension2D  viewport;

    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_ONE,
                                           std :: bind( &World :: BringToFront, this, 0, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_TWO,
                                           std :: bind( &World :: BringToFront, this, 1, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_THREE,
                                           std :: bind( &World :: BringToFront, this, 2, std :: placeholders :: _1, std :: placeholders :: _2 ) );
    m_pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_FOUR,
                                           std :: bind( &World :: BringToFront, this, 3, std :: placeholders :: _1, std :: placeholders :: _2 ) );

    viewport.pos.x = __VIEWPORT_POS_X;
    viewport.pos.y = __VIEWPORT_POS_Y;
    viewport.size.nWidth  = __VIEWPORT_WIDTH;
    viewport.size.nHeight = __VIEWPORT_HEIGHT;

    m_pRenderer -> GetViewport().SetDimension2D( viewport );
    m_pRenderer -> SetDrawFPS( __ENABLE_FPS_SHOW_LABEL );
    m_pRenderer -> Start();

    if( !m_pRenderer -> LoadMap( __TMX_MAP_FILE, __DEFAULT_MAP_ALIGNMENT ) )  {
        fprintf( stderr, "Error loading map\n" );
        return false;
    }

    if( !m_pRenderer -> RegisterFont( "Sans", "/shared/fonts/caravellius8x8.fnt", false, false ) )
        fprintf( stderr, "Cannot load the Sans font\n" );

    if( !LoadSprites() )  {
        fprintf( stderr, "Error loading sprites\n" );
        return false;
    }

    printf( "Four overlapping cards, back to front: red, green, blue, gold.\n" );
    printf( "Press 1, 2, 3 or 4 to bring that card to the front of the stack. Esc quits.\n" );

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
