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

#ifndef __FONTREGISTRY_H__
#define __FONTREGISTRY_H__

#include "font/ifont.h"
#include <memory>
#include <set>
#include <string>
#include <vector>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief The fonts text objects are drawn with, by Tiled family and style (S1b of the
         * master plan): a family name as Tiled's fontfamily spells it, and bold and italic
         * flags. It owns every font it loaded (through IEngine::LoadFont); a font is released
         * when it is replaced, when Clear() is called, or with the registry - while the window
         * that loaded it is still open, as an IFont requires.
         *
         * It also remembers which family and style it has already warned about (WarnMissing),
         * so a text object with an unregistered font logs once, not every frame.
         */
        class FontRegistry  {

            struct __stRegisteredFont  {
                std :: string                                 strFamily;
                bool                                          bBold;
                bool                                          bItalic;
                std :: unique_ptr<SunLight :: Font :: IFont>  pFont;
            };

            std :: vector<__stRegisteredFont>  m_Fonts;
            std :: set<std :: string>          m_WarnedFonts;   // "family|bold|italic" keys already warned about

            public:

            bool Register( const char *szFamily, const char *szFilePath, bool bBold, bool bItalic );
            SunLight :: Font :: IFont* Find( const char *szFamily, bool bBold, bool bItalic ) const;
            void WarnMissing( const char *szFamily, bool bBold, bool bItalic );
            void Clear( void );
        };
    }
}

#endif  /* __FONTREGISTRY_H__ */
