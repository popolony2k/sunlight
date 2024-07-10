/*
 * texturecanvas.h
 *
 *  Created on: Jul 8, 2021
 *      Author: popolony2k
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
