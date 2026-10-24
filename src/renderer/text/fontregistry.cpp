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
#include <algorithm>
#include <cstdio>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Register a loaded font for a family and style, replacing (and so letting go
         * of) a font already registered for the same family and style. The font is shared, not
         * copied: the same font may also be the active font (SetFont) or serve other families.
         * @param szFamily Family name, as Tiled's fontfamily spells it;
         * @param pFont The font (from IEngine::LoadFont);
         * @param bBold Whether this is the bold face of the family;
         * @param bItalic Whether this is the italic face of the family;
         * @return true if the font is registered; false for no family, no font or a font no
         * longer valid, and an earlier font for the same family and style is kept;
         */
        bool FontRegistry :: Register( const char* szFamily, std :: shared_ptr<SunLight :: Font :: IFont> pFont, bool bBold, bool bItalic )  {

            if( ( szFamily == nullptr ) || ( pFont == nullptr ) || !pFont -> IsValid() )
                return false;

            for( __stRegisteredFont& registered : m_Fonts )  {
                if( ( registered.strFamily == szFamily ) && ( registered.bBold == bBold ) && ( registered.bItalic == bItalic ) )  {
                    // Assigning lets go of the old font, released if no one else holds it.
                    registered.pFont = std :: move( pFont );

                    return true;
                }
            }

            m_Fonts.push_back( __stRegisteredFont { szFamily, bBold, bItalic, std :: move( pFont ) } );

            return true;
        }

        /**
         * @brief Let go of a font in every family and style it was registered for (I3 of the
         * master plan): the font itself is all that is needed. Text objects of those families
         * draw the placeholder again, as for a font never registered.
         * @param pFont The font, as LoadFont gave it;
         * @return true if it was registered for at least one family and style;
         */
        bool FontRegistry :: Unregister( const SunLight :: Font :: IFont* pFont )  {

            size_t  nBefore = m_Fonts.size();

            m_Fonts.erase( std :: remove_if( m_Fonts.begin(), m_Fonts.end(),
                                             [pFont]( const __stRegisteredFont& registered )  {
                                                 return ( pFont != nullptr ) && ( registered.pFont.get() == pFont );
                                             } ),
                           m_Fonts.end() );

            return m_Fonts.size() != nBefore;
        }

        /**
         * @brief Let go of the font registered for one family and style, leaving the font's other
         * families and styles, if any, registered.
         * @param szFamily Family name, as Tiled's fontfamily spells it;
         * @param bBold Whether this is the bold face of the family;
         * @param bItalic Whether this is the italic face of the family;
         * @return true if a font was registered for exactly that family and style;
         */
        bool FontRegistry :: Unregister( const char* szFamily, bool bBold, bool bItalic )  {

            if( szFamily == nullptr )
                return false;

            for( auto pIt = m_Fonts.begin(); pIt != m_Fonts.end(); ++pIt )  {
                if( ( pIt -> strFamily == szFamily ) && ( pIt -> bBold == bBold ) && ( pIt -> bItalic == bItalic ) )  {
                    m_Fonts.erase( pIt );
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief The font registered for a family and style.
         * @return The font; nullptr when none is registered for exactly that family and style;
         */
        SunLight :: Font :: IFont* FontRegistry :: Find( const char* szFamily, bool bBold, bool bItalic ) const  {

            for( const __stRegisteredFont& registered : m_Fonts )  {
                if( ( registered.strFamily == szFamily ) && ( registered.bBold == bBold ) && ( registered.bItalic == bItalic ) )
                    return registered.pFont.get();
            }

            return nullptr;
        }

        /**
         * @brief Log, once per family and style, that a text object's font is not registered.
         * Standard error, like the map-load refusals.
         */
        void FontRegistry :: WarnMissing( const char* szFamily, bool bBold, bool bItalic )  {

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

            // Each font is let go here, and released if no one else holds it.
            m_Fonts.clear();
            m_WarnedFonts.clear();
        }
    }
}
