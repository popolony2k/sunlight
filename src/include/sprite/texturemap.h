/*
 * texture.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __TEXTUREMAP_H__
#define __TEXTUREMAP_H__

#include <queue>
#include <string>
#include "canvas/texturecanvas.h"
#include "base/object.h"


namespace SunLight {
    namespace Sprite  {
        /**
         * @brief TextureMap class implementation.
         * 
         */
        class TextureMap : public SunLight :: Base :: Object {

            public:

            /**
             * Texture structure definition.
             */
            struct stTextureData  {
                int64_t         nDelayMilli;
                int64_t         nNextTime;
                SunLight :: Canvas :: TextureCanvas*  pTexture;
            };

            TextureMap( void );
            virtual ~TextureMap( void );

            void AddTexture( SunLight :: Canvas :: TextureCanvas *pTexture, int64_t nDelayMilli = -1 );

            bool First( void );
            bool Next( bool bCircularMode = true );
            TextureMap :: stTextureData& GetTextureData( void );

            private:

            typedef std :: deque<stTextureData*> TextureList;

            TextureList               m_TextureList;
            TextureList :: iterator   m_itTexture;
        };
    }
}

#endif /* __TEXTUREMAP_H__ */
