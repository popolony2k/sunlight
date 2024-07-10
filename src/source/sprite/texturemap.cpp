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

#include "texturemap.h"
#include <chrono>

using namespace std :: chrono;


namespace SunLight {
    namespace Sprite  {
        /**
         * Destructor. Finalize all class data.
         */
        TextureMap :: TextureMap( void )  {

        }

        /**
         * Destructor. Finalize all class data.
         */
        TextureMap :: ~TextureMap( void )  {

            for( stTextureData *pTexture : m_TextureList )  {
                delete pTexture;
            }
        }

        /**
         * Loads and add a texture to the internal texture map.
         * @param texture Texture to add;
         * @param nDetalyMilli Time in millisecond to be used in texture
         * animation sequence;
         */
        void TextureMap :: AddTexture( SunLight :: Canvas :: TextureCanvas *pTexture,
                                       int64_t nDelayMilli )  {

            stTextureData              *pData = new stTextureData;
            steady_clock :: time_point now    = steady_clock :: now();

            pData -> pTexture    = pTexture;
            pData -> nDelayMilli = nDelayMilli;
            pData -> nNextTime   = duration_cast<milliseconds>( now.time_since_epoch() ).count();
            pData -> nNextTime+=nDelayMilli;

            m_TextureList.push_back( pData );

            if( m_TextureList.size() == 1 )
                First();
        }

        /**
         * Get the first texture on list.
         */
        bool TextureMap :: First( void )  {

            m_itTexture = m_TextureList.begin();

            return ( m_TextureList.size() > 0 );
        }

        /**
         * Get the next texture on list.
         * @param bCircularMode Navigate on list using circular mode;
         * WARNING: Be careful because this is a circular list.
         * Avoid to iterate checking the end of list when using in circular
         * mode because in this case there's no end of list.
         * This function check if current texture is inside it's
         * animation update window before going to the next texture frame.
         */
        bool TextureMap :: Next( bool bCircularMode )  {

            if( bCircularMode )  {
                if( m_TextureList.size() > 0 )  {
                    if( ( * m_itTexture ) -> nDelayMilli != -1 )  {
                        steady_clock :: time_point now = steady_clock :: now();
                        int64_t nTimeMilli = duration_cast<milliseconds>( now.time_since_epoch() ).count();

                        if( nTimeMilli < ( * m_itTexture ) -> nNextTime )  {
                            return false;
                        }

                        m_itTexture++;

                        if( m_itTexture == m_TextureList.end() )  {
                            m_itTexture = m_TextureList.begin();
                        }

                        ( * m_itTexture ) -> nNextTime = ( nTimeMilli +
                                                        ( * m_itTexture ) -> nDelayMilli );
                        return true;
                    }
                    else  {
                        m_itTexture++;

                        if( m_itTexture == m_TextureList.end() )  {
                            m_itTexture = m_TextureList.begin();
                            return false;
                        }
                    }

                    return true;
                }

                return false;
            }
            else  {
                m_itTexture++;
                return ( m_itTexture != m_TextureList.end() );
            }
        }

        /**
         * Get the current texture on list.
         */
        TextureMap :: stTextureData& TextureMap :: GetTextureData( void )  {

            return **m_itTexture;
        }
    }
}
