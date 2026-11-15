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

#include "renderer/map/tmxworkarounds.h"
#include <cstring>
#include <string>

#define __MAP_START_TAG          "<map"          // the root element's start tag
#define __STAGGER_AXIS_ATTRIBUTE "staggeraxis"
#define __TILED_AXIS_X           "x"             // what Tiled writes for the X axis
#define __LIBTMX_AXIS_X          "columns"       // what libtmx's parse_stagger_axis accepts for it

namespace SunLight  {
    namespace Renderer  {

        namespace  {

            bool IsXmlSpace( unsigned char c )  {

                return ( c == ' ' ) || ( c == '\t' ) || ( c == '\n' ) || ( c == '\r' );
            }

            // Where the root element's start tag begins: "<map" followed by a space or '>' (so not
            // "<mapping"), or data.size() when there is none.
            size_t FindMapTag( const std :: vector<unsigned char>& data )  {

                const size_t  nTagLength = std :: strlen( __MAP_START_TAG );

                for( size_t nPos = 0; nPos + nTagLength < data.size(); nPos++ )  {
                    if( ( std :: memcmp( &data[nPos], __MAP_START_TAG, nTagLength ) == 0 ) &&
                        ( IsXmlSpace( data[nPos + nTagLength] ) || ( data[nPos + nTagLength] == '>' ) ) )
                        return nPos;
                }

                return data.size();
            }
        }

        /**
         * @brief Make Tiled's staggeraxis="x" readable by libtmx (I20 of the master plan): libtmx's
         * parse_stagger_axis (tmx_utils.c) takes "y" or "columns", but Tiled writes "x" or "y" (its TMX
         * reference, and staggerAxisToString in its map.cpp), so every X-staggered map Tiled saves was
         * refused ("unsupported 'staggeraxis' 'x'"). Only the root <map ...> start tag is looked at - its
         * attributes walked one by one, quoted values skipped whole, so a '>' or the attribute's name
         * inside another value, or a "staggeraxis" in a property further down, is never touched - and only
         * the value "x" changes, to "columns", libtmx's word for the same axis. A map that libtmx would
         * read already passes through unchanged. Remove once libtmx reads "x":
         * https://github.com/baylej/tmx/issues/90 (the value it produces stays one libtmx accepts).
         * @param data The map's bytes, rewritten in place;
         * @return true when the value was rewritten;
         */
        bool TmxWorkarounds :: FixStaggerAxis( std :: vector<unsigned char>& data )  {

            const size_t  nAttributeLength = std :: strlen( __STAGGER_AXIS_ATTRIBUTE );
            size_t        nPos             = FindMapTag( data );

            if( nPos >= data.size() )
                return false;

            nPos += std :: strlen( __MAP_START_TAG );

            while( nPos < data.size() )  {
                while( ( nPos < data.size() ) && IsXmlSpace( data[nPos] ) )
                    nPos++;

                if( ( nPos >= data.size() ) || ( data[nPos] == '>' ) || ( data[nPos] == '/' ) )
                    return false;                       // the end of the tag: no such attribute

                // The attribute's name, up to '=' or a space.
                size_t  nNameStart = nPos;

                while( ( nPos < data.size() ) && ( data[nPos] != '=' ) && !IsXmlSpace( data[nPos] ) && ( data[nPos] != '>' ) )
                    nPos++;

                std :: string  strName( data.begin() + nNameStart, data.begin() + nPos );

                while( ( nPos < data.size() ) && IsXmlSpace( data[nPos] ) )
                    nPos++;

                if( ( nPos >= data.size() ) || ( data[nPos] != '=' ) )
                    return false;                       // not well-formed: leave it to libtmx to say so

                nPos++;

                while( ( nPos < data.size() ) && IsXmlSpace( data[nPos] ) )
                    nPos++;

                if( ( nPos >= data.size() ) || ( ( data[nPos] != '"' ) && ( data[nPos] != '\'' ) ) )
                    return false;

                unsigned char  quote       = data[nPos];
                size_t         nValueStart = ++nPos;

                while( ( nPos < data.size() ) && ( data[nPos] != quote ) )
                    nPos++;

                if( nPos >= data.size() )
                    return false;

                if( ( strName.size() == nAttributeLength ) && ( strName == __STAGGER_AXIS_ATTRIBUTE ) )  {
                    std :: string  strValue( data.begin() + nValueStart, data.begin() + nPos );

                    if( strValue != __TILED_AXIS_X )
                        return false;                   // "y", "columns", ...: libtmx's to judge

                    const char  *szReplacement = __LIBTMX_AXIS_X;

                    data.erase( data.begin() + nValueStart, data.begin() + nPos );
                    data.insert( data.begin() + nValueStart, szReplacement, szReplacement + std :: strlen( szReplacement ) );

                    return true;
                }

                nPos++;                                 // past the closing quote
            }

            return false;
        }
    }
}
