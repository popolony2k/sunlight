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

#include "sprite.h"


namespace SunLight {
    namespace Sprite  {
        /**
         * Constructor. Initialize all class data.
         */
        Sprite :: Sprite( void ) {

            m_itActiveSequence = m_Sequences.begin();
            m_bIsValidActiveSequence = ( m_itActiveSequence != m_Sequences.end() );
        }

        /**
         * Destructor. Finalize all class data.
         */
        Sprite :: ~Sprite( void )  {

            Unload();
        }

        /**
         * Loads and add a texture to the internal texture map object.
         * @param pTexture Pointer to a @link TextureCanvas to add;
         * @param nDetalyMilli Time in millisecond to be used in texture
         * animation sequence;
         */
        void Sprite :: AddTextureSequence( int nSequence,
                                           SunLight :: Canvas :: TextureCanvas *pTexture,
                                           int64_t nDelayMilli ) {

            TextureSequenceList :: iterator       itItem    = m_Sequences.find( nSequence );
            SunLight :: TileMap :: stDimension2D& spritePos = GetDimension2D();
            BaseCanvas                            *pParent  = GetParent();

            if( !pParent )
                pParent = this;

            if( ( spritePos.size.nWidth == 0 ) &&
                ( spritePos.size.nHeight == 0 ) )  {
                SunLight :: TileMap :: stDimension2D   texturePos = pTexture -> GetDimension2D();

                spritePos.size.nWidth  = texturePos.size.nWidth;
                spritePos.size.nHeight = texturePos.size.nHeight;
            }

            if( ( spritePos.pos.x == 0 ) && ( spritePos.pos.y == 0 ) )  {
                SunLight :: TileMap :: stDimension2D   texturePos = pTexture -> GetDimension2D();

                spritePos.pos.x = texturePos.pos.x;
                spritePos.pos.y = texturePos.pos.y;
            }

            pTexture -> SetVisible( GetVisible() );
            pTexture -> SetParent( pParent );
            pTexture -> SetDimension2DPtr( &spritePos );

            if( itItem == m_Sequences.end() )  {
                TextureMap   *pTextureMap = new TextureMap();

                pTextureMap -> AddTexture( pTexture, nDelayMilli );
                m_Sequences.insert( std :: make_pair( nSequence, pTextureMap ) );
            }
            else  {
                itItem -> second -> AddTexture( pTexture, nDelayMilli );
            }
        }

        /**
         * Set the active sprite sequence animation.
         * @param nSequence The sequence id to activate;
         */
        bool Sprite :: SetActiveTextureSequence( int nSequence )  {

            m_itActiveSequence = m_Sequences.find( nSequence );
            m_bIsValidActiveSequence = ( m_itActiveSequence != m_Sequences.end() );

            /*
            * Reset current animation sequence on selected
            * texture.
            */
            if( m_bIsValidActiveSequence )  {
                m_itActiveSequence -> second -> GetTextureData().pTexture -> Reset();
            }

            return m_bIsValidActiveSequence;
        }

        /**
         * @brief Get the active sequence object iterator;
         * 
         * @return the active sequence number or -1 if is invalid;
         */
        int Sprite :: GetActiveTextureSequence( void )  {

            return ( m_bIsValidActiveSequence ? m_itActiveSequence -> first : -1 );
        }

        /**
         * @brief Get the active texture sequence object pointer;
         * 
         * @return the active sequence number or NULL if active texture is invalid;
         */
        SunLight :: Canvas :: TextureCanvas* Sprite :: GetActiveTexture( void )  {

            return ( m_bIsValidActiveSequence ? m_itActiveSequence -> second -> GetTextureData().pTexture : NULL );
        }

        /**
        * Move the sprite based on x,y steps passed a parameter.
        * @param step Reference to a @link stCoordinate2D containing
        * the x,y move steps;
        */
        void Sprite :: Move( SunLight :: TileMap :: stCoordinate2D& step )  {

            SunLight :: TileMap :: stDimension2D& dimension = GetDimension2D();

            dimension.pos.x+=step.x;
            dimension.pos.y+=step.y;
        }

        /**
        * Set the visible status of a drawing entity.
        * @param bVisible The new visible status;
        */
        void Sprite :: SetVisible( bool bVisible )  {

            TextureSequenceList :: iterator itItem;

            Canvas :: SetVisible( bVisible );

            for( itItem = m_Sequences.begin(); itItem != m_Sequences.end(); itItem++ )  {
                if( itItem -> second -> First() )  {
                    do  {
                        itItem -> second -> GetTextureData().pTexture -> SetVisible( bVisible );
                    } while( itItem -> second -> Next( false ) );

                    itItem -> second -> First();
                }
            }
        }

        /**
        * Implements the draw update method used to draw a sprite
        * object;
        */
        void Sprite :: Update( void )  {

            if( GetVisible() && m_bIsValidActiveSequence )  {
                bool                                bDisableFrameUpdate = !m_itActiveSequence -> second -> Next();
                SunLight :: Canvas :: TextureCanvas *pTextureCanvas     = m_itActiveSequence -> second -> GetTextureData().pTexture;
                unsigned int  nTileSize;

                if( bDisableFrameUpdate )  {
                    nTileSize = pTextureCanvas -> GetTileSize();
                    pTextureCanvas -> SetTileSize( 0 );
                }

                pTextureCanvas -> Update();

                if( bDisableFrameUpdate )
                    pTextureCanvas -> SetTileSize( nTileSize );
            }
        }

        /**
        * Unload all loaded sprites on this object.
        */
        void Sprite :: Unload( void )  {

            if( m_Sequences.size() > 0 )  {
                for( std :: pair<int, TextureMap*> pair : m_Sequences )  {
                    if( pair.second -> First() )  {
                        do  {
                            pair.second -> GetTextureData().pTexture -> Unload();
                        } while( pair.second -> Next( false ) );
                    }
                }

                m_Sequences.clear();
            }
        }
    }
}
