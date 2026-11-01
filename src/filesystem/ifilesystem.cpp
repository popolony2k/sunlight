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

#include "filesystem/ifilesystem.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>


namespace SunLight  {
    namespace FileSystem  {

        namespace  {

            const char * const  __SCHEME_SEPARATOR      = "://";
            const char * const  __SCHEME_VPATH          = "vpath";
            const char * const  __SCHEME_VIRTUALPATH    = "virtualpath";
            const char * const  __SCHEME_HOSTPATH       = "hostpath";
            const char * const  __SCHEME_HPATH          = "hpath";

            // A host file's bytes, read whole and in binary.
            bool ReadHostFile( const std :: string& strPath, std :: vector<unsigned char>& outData )  {

                std :: error_code  errorCode;

                if( !std :: filesystem :: is_regular_file( strPath, errorCode ) )
                    return false;

                std :: ifstream  file( strPath, std :: ios :: binary );

                if( !file.is_open() )
                    return false;

                std :: vector<unsigned char>  data( ( std :: istreambuf_iterator<char>( file ) ), std :: istreambuf_iterator<char>() );

                if( file.bad() )
                    return false;

                outData = std :: move( data );

                return true;
            }

            // A host file written whole, in binary, its missing folders created first.
            bool WriteHostFile( const std :: string& strPath, const std :: vector<unsigned char>& data )  {

                std :: error_code             errorCode;
                std :: filesystem :: path     parent = std :: filesystem :: path( strPath ).parent_path();

                if( !parent.empty() )
                    std :: filesystem :: create_directories( parent, errorCode );

                std :: ofstream  file( strPath, std :: ios :: binary | std :: ios :: trunc );

                if( !file.is_open() )
                    return false;

                file.write( ( const char * ) data.data(), ( std :: streamsize ) data.size() );

                return file.good();
            }
        }

        /**
         * @brief See the declaration (ifilesystem.h). Only letters before "://" make a scheme, so a
         * virtual path that happens to hold "://" further on is still a virtual path.
         */
        IFileSystem :: PathScheme IFileSystem :: SplitPath( const std :: string& strPath, std :: string& strRest )  {

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

        void IFileSystem :: SetReadFilter( ReadFilterCallback callback )  {

            m_ReadFilter = std :: move( callback );
        }

        void IFileSystem :: SetWriteFilter( WriteFilterCallback callback )  {

            m_WriteFilter = std :: move( callback );
        }

        void IFileSystem :: SetHostPathEnabled( bool bEnabled )  {

            m_bHostPathEnabled = bEnabled;
        }

        bool IFileSystem :: IsHostPathEnabled( void ) const  {

            return m_bHostPathEnabled;
        }

        bool IFileSystem :: Exists( const std :: string& strPath )  {

            std :: string       strRest;
            std :: error_code   errorCode;

            switch( SplitPath( strPath, strRest ) )  {
                case PATH_SCHEME_VIRTUAL :
                    return VirtualExists( strRest );

                case PATH_SCHEME_HOST :
                    return m_bHostPathEnabled && std :: filesystem :: is_regular_file( strRest, errorCode );

                default :
                    return false;
            }
        }

        /**
         * @brief The read filter runs after a fully successful raw read only: a missing or unreadable
         * file fails before it, so the filter never sees that case.
         */
        bool IFileSystem :: ReadFile( const std :: string& strPath, std :: vector<unsigned char>& outData )  {

            std :: string                 strRest;
            std :: vector<unsigned char>  data;
            bool                          bRead;

            switch( SplitPath( strPath, strRest ) )  {
                case PATH_SCHEME_VIRTUAL :
                    bRead = VirtualReadFile( strRest, data );
                    break;

                case PATH_SCHEME_HOST :
                    bRead = m_bHostPathEnabled && ReadHostFile( strRest, data );
                    break;

                default :
                    bRead = false;
                    break;
            }

            if( !bRead )
                return false;

            if( m_ReadFilter )  {
                std :: vector<unsigned char>  filtered;

                if( m_ReadFilter( data, filtered ) )
                    data = std :: move( filtered );
            }

            outData = std :: move( data );

            return true;
        }

        bool IFileSystem :: WriteFile( const std :: string& strPath, const std :: vector<unsigned char>& data )  {

            std :: string                 strRest;
            PathScheme                    scheme = SplitPath( strPath, strRest );
            std :: vector<unsigned char>  filtered;
            bool                          bFiltered = false;

            if( ( scheme == PATH_SCHEME_UNKNOWN ) || ( ( scheme == PATH_SCHEME_HOST ) && !m_bHostPathEnabled ) )
                return false;

            if( m_WriteFilter )
                bFiltered = m_WriteFilter( data, filtered );

            const std :: vector<unsigned char>  &bytes = bFiltered ? filtered : data;

            return ( scheme == PATH_SCHEME_HOST ) ? WriteHostFile( strRest, bytes ) : VirtualWriteFile( strRest, bytes );
        }

        /**
         * @brief See this method's own declaration (ifilesystem.h) for
         * the full rationale.
         */
        std :: string IFileSystem :: ToVirtualPath( const std :: string& strRealPath )  {

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
