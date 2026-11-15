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

#include "filesystem/filepath.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>


namespace SunLight  {
    namespace FileSystem  {

        namespace  {

            const char * const  __SCHEME_SEPARATOR      = "://";
            const char * const  __SCHEME_VPATH          = "vpath";
            const char * const  __SCHEME_VIRTUALPATH    = "virtualpath";
            const char * const  __SCHEME_HOSTPATH       = "hostpath";
            const char * const  __SCHEME_HPATH          = "hpath";

            const size_t  __DRIVE_LENGTH = 2;     // a Windows drive segment: a letter and ':' ("C:")

            // The scheme a path starts with, as written ("vpath://", "hpath://", ...; "" for none or an
            // unknown one), and the path after it (I12 of the master plan: the path helpers work on
            // that rest only, and put the scheme back, so "//" is never mistaken for an empty segment).
            std :: string SchemeOf( const std :: string& strPath, std :: string& strRest, bool& bHost )  {

                FilePath :: PathScheme  scheme = FilePath :: SplitPath( strPath, strRest );

                bHost = ( scheme == FilePath :: PATH_SCHEME_HOST );

                if( scheme == FilePath :: PATH_SCHEME_UNKNOWN )
                    strRest = strPath;

                return strPath.substr( 0, strPath.size() - strRest.size() );
            }

            // A Windows drive segment ("C:"), which only a host path can start with.
            bool IsDrive( const std :: string& strPart )  {

                return ( strPart.size() == __DRIVE_LENGTH ) && isalpha( ( unsigned char ) strPart[0] ) && ( strPart[1] == ':' );
            }

            // backslashes become '/', and "." and ".." are collapsed LEXICALLY; "" when ".." would climb
            // above the root (or above a host path's drive).
            std :: string CollapsePath( const std :: string& strPath, bool bHost )  {

                std :: string  joined = strPath;

                std :: replace( joined.begin(), joined.end(), '\\', '/' );

                bool                          bAbsolute = ( !joined.empty() && joined[0] == '/' );
                bool                          bDrive    = false;
                std :: vector<std :: string>  parts;
                size_t                        nStart = 0;

                while( nStart <= joined.size() )  {
                    size_t         nEnd  = joined.find( '/', nStart );
                    std :: string  part;

                    if( nEnd == std :: string :: npos )
                        nEnd = joined.size();

                    part   = joined.substr( nStart, nEnd - nStart );
                    nStart = nEnd + 1;

                    if( part.empty() || part == "." )
                        continue;

                    if( part == ".." )  {
                        if( parts.empty() || ( bDrive && ( parts.size() == 1 ) ) )
                            return std :: string();

                        parts.pop_back();
                        continue;
                    }

                    if( bHost && parts.empty() && !bAbsolute && IsDrive( part ) )
                        bDrive = true;

                    parts.push_back( part );
                }

                std :: string  result = ( bAbsolute ? "/" : "" );

                for( size_t nCount = 0; nCount < parts.size(); nCount++ )
                    result += ( nCount ? "/" : "" ) + parts[nCount];

                return result;
            }
        }

        /**
         * @brief The scheme a path starts with, and the path after it. Only letters before "://" at
         * the very start make a scheme, so a virtual path that happens to hold "://" further on is
         * still a virtual path. "vpath" and "virtualpath" are virtual, "hostpath" and "hpath" are
         * host, compared as written (lowercase); any other is unknown, and then strRest is the
         * whole path.
         * @param strPath The path, with or without a scheme;
         * @param strRest Receives the path without its scheme (the whole path when there is none, or
         * an unknown one);
         * @return Where the path lives;
         */
        FilePath :: PathScheme FilePath :: SplitPath( const std :: string& strPath, std :: string& strRest )  {

            size_t  nSeparator = strPath.find( __SCHEME_SEPARATOR );

            strRest = strPath;

            if( ( nSeparator == std :: string :: npos ) || ( nSeparator == 0 ) )
                return PATH_SCHEME_VIRTUAL;

            for( size_t nChar = 0; nChar < nSeparator; nChar++ )  {
                if( !std :: isalpha( ( unsigned char ) strPath[nChar] ) )
                    return PATH_SCHEME_VIRTUAL;
            }

            std :: string  strScheme = strPath.substr( 0, nSeparator );

            strRest = strPath.substr( nSeparator + std :: char_traits<char> :: length( __SCHEME_SEPARATOR ) );

            if( ( strScheme == __SCHEME_VPATH ) || ( strScheme == __SCHEME_VIRTUALPATH ) )
                return PATH_SCHEME_VIRTUAL;

            if( ( strScheme == __SCHEME_HOSTPATH ) || ( strScheme == __SCHEME_HPATH ) )
                return PATH_SCHEME_HOST;

            strRest = strPath;

            return PATH_SCHEME_UNKNOWN;
        }

        /**
         * @brief The folder part of a path, its trailing '/' kept, its scheme kept in front: the
         * scheme is split off (SchemeOf), backslashes become '/', and everything up to the last '/'
         * is kept. Moved here from the map loader's ExternalResources::DirectoryOfVirtualPath (I13 of
         * the master plan), unchanged.
         * @param strPath The path, with or without a scheme;
         * @return The scheme and the folder part ("" for the folder part when there is no '/');
         */
        std :: string FilePath :: ParentPath( const std :: string& strPath )  {

            std :: string  strRest;
            bool           bHost;
            std :: string  strScheme = SchemeOf( strPath, strRest, bHost );

            std :: replace( strRest.begin(), strRest.end(), '\\', '/' );

            size_t  nSlash = strRest.find_last_of( '/' );

            return strScheme + ( ( nSlash == std :: string :: npos ) ? std :: string() : strRest.substr( 0, nSlash + 1 ) );
        }

        /**
         * @brief strDir and strRelative as one path. A strRelative with a scheme of its own is used
         * alone; otherwise strDir's scheme is split off, the rest of strDir and strRelative are
         * glued as they are and collapsed (CollapsePath: backslashes to '/', empty and "." segments
         * dropped, ".." resolved, never above the root or a host path's drive), and the scheme put
         * back. Moved here from the map loader's ExternalResources::JoinVirtualPath (I13 of the
         * master plan), unchanged - it used to drop the "//" of a scheme as an empty segment, found
         * by Scarab after I12.
         * @param strDir The folder, with or without a scheme (as ParentPath gives it);
         * @param strRelative The path relative to it;
         * @return The joined path; "" when ".." would climb above the root or a drive;
         */
        std :: string FilePath :: JoinPath( const std :: string& strDir, const std :: string& strRelative )  {

            std :: string  strRest;
            bool           bHost;
            std :: string  strScheme = SchemeOf( strRelative, strRest, bHost );
            std :: string  strJoined;

            if( strScheme.empty() )  {
                strScheme  = SchemeOf( strDir, strJoined, bHost );
                strJoined += strRelative;
            }
            else
                strJoined = strRest;

            std :: string  strCollapsed = CollapsePath( strJoined, bHost );

            return strCollapsed.empty() ? std :: string() : ( strScheme + strCollapsed );
        }

        /**
         * @brief A real, OS-native path as a legal virtual path: backslashes become '/', a drive
         * letter prefix ("C:" - a ':' at index 1) is stripped, and a leading "./" is stripped. PhysFS
         * rejects ':' and a "." segment (its sanitizePlatformIndependentPath); the declaration in
         * filepath.h has the full rationale (raylib's BMFont atlas path). Idempotent.
         * @param strRealPath The real, OS-native path;
         * @return Its virtual-path equivalent;
         */
        std :: string FilePath :: ToVirtualPath( const std :: string& strRealPath )  {

            std :: string  strVirtual = strRealPath;

            for( char& c : strVirtual )  {
                if( c == '\\' )
                    c = '/';
            }

            if( ( strVirtual.size() >= 2 ) && ( strVirtual[1] == ':' ) )
                strVirtual = strVirtual.substr( 2 );

            if( ( strVirtual.size() >= 2 ) && ( strVirtual[0] == '.' ) && ( strVirtual[1] == '/' ) )
                strVirtual = strVirtual.substr( 2 );

            return strVirtual;
        }
    }
}
