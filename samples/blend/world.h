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


#ifndef __WORLD_H__
#define __WORLD_H__

#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "tilemap/itilemaplistener.h"
#include <memory>
#include <string>


/**
 * @brief World class implementation - I2 of the master plan: blend modes, over a map or with no
 * map. Sunny walks with a glow sprite whose mode the number keys choose (Sprite::SetBlendMode); a
 * soft shadow sprite always multiplies; and four panels, one per mode, are drawn by the game
 * itself between IDrawSurface::BeginBlendMode and EndBlendMode.
 */
class World : public SunLight :: TileMap :: ITileMapListener  {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;

    // A sprite of one texture, and that texture.
    struct stSpriteRig  {
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;
    };

    stSpriteRig  m_Sunny;
    stSpriteRig  m_Glow;
    stSpriteRig  m_Shadow;

    // Whether a map is loaded (the default) or the renderer is a bare drawing surface ("nomap").
    bool  m_bUseMap;

    // The glow's mode, as the HUD names it: "none" when the sprite has no mode of its own.
    std :: string  m_strGlowMode;

    void MoveSunny( int nDx, int nDy );
    void SetGlowMode( bool bHasMode, SunLight :: Base :: BlendMode mode, const char* szName );

    bool LoadSprite( stSpriteRig& rig, const char* szFile, int nX, int nY, int nWidth, int nHeight, int nLayerId );
    bool LoadSprites( void );
    void DrawPanels( void );
    void DrawHud( void );

    public :

    World( bool bUseMap );

    bool Run( void );

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap ) override;
    void OnStop( void ) override;
};

#endif /* __WORLD_H__ */
