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
 * @brief World class implementation - M3 of the master plan: TileMapRenderer used purely as a
 * drawing surface, LoadMap never called. Everything drawn here comes from the three mapless
 * paths Phase M finished: IShapeSurface shapes (M1), a sprite registered with AddSprite on a
 * plain grouping id (M2), and IDrawSurface's own screen-space text and filled rectangles.
 * Shapes and the HUD are drawn every frame from OnUpdate, which Run() calls inside the frame's
 * render target, after the sprites.
 */
class World : public SunLight :: TileMap :: ITileMapListener {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteSunny;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasSunny;

    // Frames drawn so far - drives the shapes' animation (rotation, wave phase, bar fill).
    long  m_nFrame;

    void MovePlayerUp( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerDown( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerRight( SunLight :: Input :: ControllerType type, int nId );
    void MoveSunny( int nDx, int nDy );

    bool LoadSprites( void );
    void DrawShapes( void );
    void DrawHud( void );

    public :

    World( std :: string strBasePath );

    bool Run( void );

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap ) override;
    void OnStop( void ) override;
};

 #endif // __WORLD_H__
