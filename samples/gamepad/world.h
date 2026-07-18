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
 #include <memory>
 #include <string>


 /**
 * @brief World class implementation.
 */
class World {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteSunny;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasSunny;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteMonkey;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasMonkey;
    std :: string m_strBasePath;

    void MoveSunnyUp( SunLight :: Input :: ControllerType type, int nId );
    void MoveSunnyDown( SunLight :: Input :: ControllerType type, int nId );
    void MoveSunnyLeft( SunLight :: Input :: ControllerType type, int nId );
    void MoveSunnyRight( SunLight :: Input :: ControllerType type, int nId );
    void MoveMonkeyUp( SunLight :: Input :: ControllerType type, int nId );
    void MoveMonkeyDown( SunLight :: Input :: ControllerType type, int nId );
    void MoveMonkeyLeft( SunLight :: Input :: ControllerType type, int nId );
    void MoveMonkeyRight( SunLight :: Input :: ControllerType type, int nId );
    void ZoomIn( SunLight :: Input :: ControllerType type, int nId );
    void ZoomOut( SunLight :: Input :: ControllerType type, int nId );
    void ResetZoom( SunLight :: Input :: ControllerType type, int nId );

    bool LoadSprites( void );

    public :

    World( std :: string strBasePath );

    bool Run( void );
};

 #endif // __WORLD_H__
