/*
 * sprite.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include <map>
#include "collision/collider.h"
#include "sprite/texturemap.h"


namespace SunLight {
    namespace Sprite  {
        /**
         * @brief Sunlight sprite management class implementation.
         * 
         */
        class Sprite : public SunLight :: Canvas :: DrawCanvas  {

            /**
             * @brief Texture sequence list definition.
             * 
             */
            typedef std :: map<int, TextureMap*>  TextureSequenceList;

            TextureSequenceList             m_Sequences;
            TextureSequenceList :: iterator m_itActiveSequence;
            bool                            m_bIsValidActiveSequence;

            public:

            Sprite( void );
            virtual ~Sprite( void );

            void AddTextureSequence( int nSequence,
                                    SunLight :: Canvas :: TextureCanvas* pTexture,
                                    int64_t nDelayMilli = -1 );
            bool SetActiveTextureSequence( int nSequence );
            int GetActiveTextureSequence( void );
            SunLight :: Canvas :: TextureCanvas* GetActiveTexture( void );

            void SetVisible( bool bVisible );

            void Move( SunLight :: TileMap :: stCoordinate2D& step );

            void Update( void );
            void Unload( void );
        };
    }
}

#endif /* __SPRITE_H__ */
