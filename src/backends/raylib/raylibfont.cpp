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

#include "backends/raylib/raylibfont.h"

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief Take ownership of a loaded font's state (see
             * @see RaylibEngine::LoadFont, which creates it).
             */
            RaylibFont :: RaylibFont( std :: shared_ptr<__stRaylibFont> pState ) : m_pState( std :: move( pState ) )  {
            }

            /**
             * @brief Release the raylib font, unless the engine already did
             * when the window closed - then only the state is dropped.
             */
            RaylibFont :: ~RaylibFont( void )  {

                if( m_pState -> bValid )  {
                    ::UnloadFont( m_pState -> font );
                    m_pState -> bValid = false;
                }
            }

            /**
             * @brief False once the window has closed and the engine released
             * this font (see @see IFont::IsValid).
             */
            bool RaylibFont :: IsValid( void ) const  {

                return m_pState -> bValid;
            }

            /**
             * @brief Width of a line in this font, or 0 once it is invalid
             * (see @see IFont::MeasureText).
             */
            int RaylibFont :: MeasureText( const char *szText, int nFontSize )  {

                if( !m_pState -> bValid )
                    return 0;

                Vector2  size = ::MeasureTextEx( m_pState -> font, szText, ( float ) nFontSize, __DEFAULT_TEXT_SPACING );

                return ( int ) size.x;
            }

            void RaylibFont :: DrawTextRotated( const char *szText,
                                                int nPosX,
                                                int nPosY,
                                                int nFontSize,
                                                float rotation,
                                                SunLight :: Base :: stColor color )  {

                if( !m_pState -> bValid )
                    return;

                ::DrawTextPro( m_pState -> font, szText, Vector2{ ( float ) nPosX, ( float ) nPosY }, Vector2 { 0.0f, 0.0f },
                               rotation, ( float ) nFontSize, __DEFAULT_TEXT_SPACING,
                               Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }

            /**
             * @brief Draw a line in this font, or nothing once it is invalid
             * (see @see IFont::DrawText).
             */
            void RaylibFont :: DrawText( const char *szText,
                                         int nPosX,
                                         int nPosY,
                                         int nFontSize,
                                         SunLight :: Base :: stColor color )  {

                if( !m_pState -> bValid )
                    return;

                ::DrawTextEx( m_pState -> font, szText, Vector2{ ( float ) nPosX, ( float ) nPosY },
                              ( float ) nFontSize, __DEFAULT_TEXT_SPACING,
                              Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }
        }
    }
}
