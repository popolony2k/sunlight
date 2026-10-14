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

#include "renderer/text/fontregistry.h"
#include "engines/enginefactory.h"
#include <cstdio>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Load a font and register it for a family and style, replacing (and so
         * releasing) a font already registered for the same family and style.
         * @param szFamily Family name, as Tiled's fontfamily spells it;
         * @param szFilePath Font file to load (any format IEngine::LoadFont reads);
         * @param bBold Whether this is the bold face of the family;
         * @param bItalic Whether this is the italic face of the family;
         * @return true if the font loaded and is registered; false otherwise, and an earlier
         * font for the same family and style is kept;
         */
        bool FontRegistry :: Register( const char *szFamily, const char *szFilePath, bool bBold, bool bItalic )  {

            if( ( szFamily == nullptr ) || ( szFilePath == nullptr ) )
                return false;

            std :: unique_ptr<SunLight :: Font :: IFont>  pFont = SunLight :: Engines :: EngineFactory :: GetEngine().LoadFont( szFilePath );

            if( pFont == nullptr )
                return false;

            for( __stRegisteredFont &registered : m_Fonts )  {
                if( ( registered.strFamily == szFamily ) && ( registered.bBold == bBold ) && ( registered.bItalic == bItalic ) )  {
                    // Assigning destroys the old font, which releases it.
                    registered.pFont = std :: move( pFont );

                    return true;
                }
            }

            m_Fonts.push_back( __stRegisteredFont { szFamily, bBold, bItalic, std :: move( pFont ) } );

            return true;
        }

        /**
         * @brief The font registered for a family and style.
         * @return The font; nullptr when none is registered for exactly that family and style;
         */
        SunLight :: Font :: IFont* FontRegistry :: Find( const char *szFamily, bool bBold, bool bItalic ) const  {

            for( const __stRegisteredFont &registered : m_Fonts )  {
                if( ( registered.strFamily == szFamily ) && ( registered.bBold == bBold ) && ( registered.bItalic == bItalic ) )
                    return registered.pFont.get();
            }

            return nullptr;
        }

        /**
         * @brief Log, once per family and style, that a text object's font is not registered.
         * Standard error, like the map-load refusals.
         */
        void FontRegistry :: WarnMissing( const char *szFamily, bool bBold, bool bItalic )  {

            std :: string  strKey = std :: string( szFamily ) + ( bBold ? "|bold" : "|regular" ) + ( bItalic ? "|italic" : "|upright" );

            if( !m_WarnedFonts.insert( strKey ).second )
                return;

            fprintf( stderr, "Text object font [%s%s%s] is not registered (RegisterFont); its text is drawn as ?????? instead.\n",
                     szFamily, bBold ? " bold" : "", bItalic ? " italic" : "" );
        }

        /**
         * @brief Release every registered font, and forget the warnings already given. The fonts
         * must go while the window that loaded them is still open: TileMapRenderer calls this
         * from Stop(), before the window closes, and from its destructor.
         */
        void FontRegistry :: Clear( void )  {

            // Each font is destroyed here, which releases it.
            m_Fonts.clear();
            m_WarnedFonts.clear();
        }
    }
}
