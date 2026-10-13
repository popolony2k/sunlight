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
 #include <array>
 #include <memory>
 #include <string>


 /**
 * @brief World class implementation - L3 of the master plan (sprite draw order within a
 * layer), demonstrated as four overlapping "cards" (screen-space sprites, all registered on
 * the same layer): pressing 1, 2, 3 or 4 calls Sprite::SetZOrder on that card with a new,
 * always-higher value, moving it to the front of the stack live - proving the renderer
 * re-sorts a layer's own sprites by z-order every frame, not just once at load time.
 */
class World {

    public :

    static const int __CARD_COUNT = 4;

    private :

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>                m_pRenderer;
    std :: array<std :: unique_ptr<SunLight :: Sprite :: Sprite>, __CARD_COUNT>        m_Sprites;
    std :: array<std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>, __CARD_COUNT>  m_Canvases;

    // Always above every z-order handed out so far (starts past the cards' own initial 0..3),
    // so pressing a key always brings that card strictly above every other one, however many
    // times any key was pressed before.
    int  m_nNextZOrder;

    void BringToFront( int nCardIndex, SunLight :: Input :: ControllerType type, int nId );

    bool LoadSprites( void );

    public :

    World( std :: string strBasePath );

    bool Run( void );
};

 #endif // __WORLD_H__
