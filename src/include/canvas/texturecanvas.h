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

#ifndef __TEXTURECANVAS_H__
#define __TEXTURECANVAS_H__

#include <raylib.h>
#include <string>
#include "canvas/drawcanvas.h"


namespace SunLight {
    namespace Canvas  {

        /**
         * @brief Enumeration describing all valid animation modes.
         */
        enum AnimationMode  {
            TEXTURE_ANIMATION_MODE_MANUAL = 0,
            TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR,
            TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT,
            TEXTURE_ANIMATION_MODE_ANIMATE_LEFT,
            TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT,
            TEXTURE_ANIMATION_MODE_ANIMATE_CENTER
        };

        /**
         * @brief Texture canvas class implementation.
         * 
         */
        class TextureCanvas : public SunLight :: Canvas :: DrawCanvas  {

            Texture2D                   m_Texture;
            unsigned int                m_nTileSize;
            unsigned int                m_nActiveTileIndex;
            unsigned int                m_nCenterTileIndex;
            int                         m_nCurrentTile;
            bool                        m_bAnimationRight;
            bool                        m_bIsResetting;
            AnimationMode               m_AnimationMode;


            public:

            TextureCanvas( void );
            virtual ~TextureCanvas( void );

            bool Load( std :: string strTextureFile );
            bool Unload( void );

            void Reset( void );

            void SetTileSize( unsigned int nTileSize );
            unsigned int GetTileSize( void );

            void SetActiveTileIndex( unsigned int nActiveTileIndex );
            unsigned int GetActiveTileIndex( void );

            void SetCenterTileIndex( unsigned int nTileIndex );
            unsigned int GetCenterTileIndex( void );

            void SetAnimationMode( AnimationMode mode );
            AnimationMode GetAnimationMode( void );

            void Update( void );
        };
    }
}

#endif /* __TEXTURECANVAS_H__ */
