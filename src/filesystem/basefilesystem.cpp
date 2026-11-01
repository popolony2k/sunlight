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

#include "filesystem/basefilesystem.h"
#include "filesystem/filepath.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>


namespace SunLight  {
    namespace FileSystem  {

        namespace  {

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

        void BaseFileSystem :: SetReadFilter( ReadFilterCallback callback )  {

            m_ReadFilter = std :: move( callback );
        }

        void BaseFileSystem :: SetWriteFilter( WriteFilterCallback callback )  {

            m_WriteFilter = std :: move( callback );
        }

        void BaseFileSystem :: SetHostPathEnabled( bool bEnabled )  {

            m_bHostPathEnabled = bEnabled;
        }

        bool BaseFileSystem :: IsHostPathEnabled( void ) const  {

            return m_bHostPathEnabled;
        }

        bool BaseFileSystem :: Exists( const std :: string& strPath )  {

            std :: string       strRest;
            std :: error_code   errorCode;

            switch( FilePath :: SplitPath( strPath, strRest ) )  {
                case FilePath :: PATH_SCHEME_VIRTUAL :
                    return VirtualExists( strRest );

                case FilePath :: PATH_SCHEME_HOST :
                    return m_bHostPathEnabled && std :: filesystem :: is_regular_file( strRest, errorCode );

                default :
                    return false;
            }
        }

        /**
         * @brief The read filter runs after a fully successful raw read only: a missing or unreadable
         * file fails before it, so the filter never sees that case.
         */
        bool BaseFileSystem :: ReadFile( const std :: string& strPath, std :: vector<unsigned char>& outData )  {

            std :: string                 strRest;
            std :: vector<unsigned char>  data;
            bool                          bRead;

            switch( FilePath :: SplitPath( strPath, strRest ) )  {
                case FilePath :: PATH_SCHEME_VIRTUAL :
                    bRead = VirtualReadFile( strRest, data );
                    break;

                case FilePath :: PATH_SCHEME_HOST :
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

        bool BaseFileSystem :: WriteFile( const std :: string& strPath, const std :: vector<unsigned char>& data )  {

            std :: string                 strRest;
            FilePath :: PathScheme        scheme = FilePath :: SplitPath( strPath, strRest );
            std :: vector<unsigned char>  filtered;
            bool                          bFiltered = false;

            if( ( scheme == FilePath :: PATH_SCHEME_UNKNOWN ) || ( ( scheme == FilePath :: PATH_SCHEME_HOST ) && !m_bHostPathEnabled ) )
                return false;

            if( m_WriteFilter )
                bFiltered = m_WriteFilter( data, filtered );

            const std :: vector<unsigned char>  &bytes = bFiltered ? filtered : data;

            return ( scheme == FilePath :: PATH_SCHEME_HOST ) ? WriteHostFile( strRest, bytes ) : VirtualWriteFile( strRest, bytes );
        }
    }
}
