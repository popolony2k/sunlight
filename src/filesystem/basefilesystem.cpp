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
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

#if defined( _WIN32 )
    #include <io.h>             // _commit, _fileno
#else
    #include <unistd.h>         // fsync, fileno
#endif

#define __TEMP_SUFFIX      ".tmp"     // the atomic write's temporary file: "<name>.tmp", beside the target


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

            /**
             * @brief Writes bytes to a new file and makes them durable: written, flushed to the OS,
             * then flushed to the disk itself, and closed - each step checked. The one place with
             * platform code (I14 of the master plan): the C++ standard library cannot force data to
             * the disk (std::ofstream::flush only reaches the OS cache, and a stream gives no OS
             * handle to sync), so this uses C's FILE with the OS's own sync - fsync( fileno() ) on
             * POSIX, _commit( _fileno() ) on Windows - and _wfopen on Windows, so a path with
             * non-ASCII characters (wide there) opens too. Without the sync a power loss could keep
             * a later rename and lose these bytes on some filesystems - the empty file the atomic
             * write is there to avoid.
             * @param path The file to create (replaced if it exists);
             * @param data The bytes;
             * @return true if every byte reached the disk and the file closed cleanly;
             */
            bool WriteDurably( const std :: filesystem :: path& path, const std :: vector<unsigned char>& data )  {

#if defined( _WIN32 )
                FILE  *pFile = ::_wfopen( path.c_str(), L"wb" );
#else
                FILE  *pFile = ::fopen( path.c_str(), "wb" );
#endif

                if( pFile == nullptr )
                    return false;

                bool  bWritten = data.empty() || ( ::fwrite( data.data(), 1, data.size(), pFile ) == data.size() );

                bWritten = ( ::fflush( pFile ) == 0 ) && bWritten;
#if defined( _WIN32 )
                bWritten = ( ::_commit( ::_fileno( pFile ) ) == 0 ) && bWritten;
#else
                bWritten = ( ::fsync( ::fileno( pFile ) ) == 0 ) && bWritten;
#endif
                bWritten = ( ::fclose( pFile ) == 0 ) && bWritten;

                return bWritten;
            }
        }

        /**
         * @brief Keeps the read filter. ReadFile applies it to every successful read - virtual and
         * host paths alike - after the raw bytes are read; an empty callback clears it.
         * @param callback The filter, or an empty ReadFilterCallback;
         */
        void BaseFileSystem :: SetReadFilter( ReadFilterCallback callback )  {

            m_ReadFilter = std :: move( callback );
        }

        /**
         * @brief Keeps the write filter. WriteFile applies it to the bytes before they are written -
         * virtual and host paths alike; an empty callback clears it.
         * @param callback The filter, or an empty WriteFilterCallback;
         */
        void BaseFileSystem :: SetWriteFilter( WriteFilterCallback callback )  {

            m_WriteFilter = std :: move( callback );
        }

        /**
         * @brief Allows or refuses host paths: Exists, ReadFile, WriteFile and RemoveFile check it
         * before touching a host path. Virtual paths, Mount and SetWriteDirectory never do.
         * @param bEnabled Whether host paths are allowed (true until changed);
         */
        void BaseFileSystem :: SetHostPathEnabled( bool bEnabled )  {

            m_bHostPathEnabled = bEnabled;
        }

        /**
         * @brief Whether host paths are allowed.
         * @return true unless SetHostPathEnabled( false ) was called;
         */
        bool BaseFileSystem :: IsHostPathEnabled( void ) const  {

            return m_bHostPathEnabled;
        }

        /**
         * @brief Whether something is there: a virtual path is the backend's answer (VirtualExists,
         * across its mounts - the PhysFS backend counts a folder too); a host path must be a regular
         * file on the OS, checked only while host paths are allowed; an unknown scheme is never there.
         * @param strPath The path;
         * @return true if the backend finds it (virtual) or a regular file is there (host);
         */
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

        /**
         * @brief Writes a whole file: an unknown scheme, or a host path while host paths are refused,
         * is refused before anything is touched; the write filter, if set, turns the bytes into what
         * is written; a host path is written by WriteRealFile (atomic), a virtual path by the backend
         * (VirtualWriteFile, into its write directory - atomic too in the PhysFS backend).
         * @param strPath The path;
         * @param data The bytes, before the write filter;
         * @return true on success; false otherwise, the old file then left as it was;
         */
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

            return ( scheme == FilePath :: PATH_SCHEME_HOST ) ? WriteRealFile( strRest, bytes ) : VirtualWriteFile( strRest, bytes );
        }

        /**
         * @brief Removes a file (I14 of the master plan): a virtual path through the backend
         * (VirtualRemoveFile - inside its write directory only, never a mounted archive or folder);
         * a host path by RemoveRealFile, only while host paths are allowed; an unknown scheme is
         * refused. Files only: a folder is never removed.
         * @param strPath The path;
         * @return true if a file was removed; false if none was there, it is a folder, it could not
         * be removed, or the path is refused;
         */
        bool BaseFileSystem :: RemoveFile( const std :: string& strPath )  {

            std :: string  strRest;

            switch( FilePath :: SplitPath( strPath, strRest ) )  {
                case FilePath :: PATH_SCHEME_VIRTUAL :
                    return VirtualRemoveFile( strRest );

                case FilePath :: PATH_SCHEME_HOST :
                    return m_bHostPathEnabled && RemoveRealFile( strRest );

                default :
                    return false;
            }
        }

        /**
         * @brief The atomic write (I14 of the master plan). The bytes go to "<target>.tmp" in the
         * target's own folder - the same volume, so the rename that follows replaces the target in
         * one step - and are made durable before it (WriteDurably: checked to the last byte and
         * flushed to the disk, for a power loss), then the file is renamed over the target
         * (std::filesystem::rename, which replaces an existing file). Any failure removes the
         * temporary file - only one this call created - and leaves the target as it was. Missing
         * folders on the way are created, as before.
         * @param strRealPath The OS path of the file to write;
         * @param data The bytes (already through the write filter);
         * @return true on success; false with the target as it was;
         */
        bool BaseFileSystem :: WriteRealFile( const std :: string& strRealPath, const std :: vector<unsigned char>& data )  {

            std :: error_code             errorCode;
            std :: filesystem :: path     target( strRealPath );
            std :: filesystem :: path     temp = target;

            temp += __TEMP_SUFFIX;

            if( target.has_parent_path() )
                std :: filesystem :: create_directories( target.parent_path(), errorCode );

            if( std :: filesystem :: exists( temp, errorCode ) && !std :: filesystem :: is_regular_file( temp, errorCode ) )
                return false;           // something else holds the temporary name: never remove it

            bool  bWritten = WriteDurably( temp, data );

            if( bWritten )
                std :: filesystem :: rename( temp, target, errorCode );

            if( !bWritten || errorCode )  {
                std :: error_code  removeCode;

                std :: filesystem :: remove( temp, removeCode );

                return false;
            }

            return true;
        }

        /**
         * @brief Removes a regular file, never a folder (I14 of the master plan): anything that is
         * not a regular file - nothing at all, a folder - is left and answered false.
         * @param strRealPath The OS path of the file;
         * @return true if a file was removed;
         */
        bool BaseFileSystem :: RemoveRealFile( const std :: string& strRealPath )  {

            std :: error_code  errorCode;

            if( !std :: filesystem :: is_regular_file( strRealPath, errorCode ) )
                return false;

            return std :: filesystem :: remove( strRealPath, errorCode ) && !errorCode;
        }

        /**
         * @brief A virtual path's file inside a real write directory. A leading '/' and empty segments
         * are dropped, as PhysFS drops them; ".", "..", ':' and '\\' are refused, as PhysFS refuses them
         * (its sanitizePlatformIndependentPath), so the file is always inside strWriteDir.
         * @param strWriteDir The write directory's real folder ("" when none is set);
         * @param strVirtualPath The virtual path, without its scheme;
         * @param strRealPath Receives the file's real path;
         * @return false when there is no write directory, the path is empty, or it is refused;
         */
        bool BaseFileSystem :: RealWritePath( const std :: string& strWriteDir, const std :: string& strVirtualPath, std :: string& strRealPath )  {

            if( strWriteDir.empty() )
                return false;

            std :: filesystem :: path  real( strWriteDir );
            bool                       bAny  = false;
            size_t                     nStart = 0;

            if( ( strVirtualPath.find( ':' ) != std :: string :: npos ) || ( strVirtualPath.find( '\\' ) != std :: string :: npos ) )
                return false;

            while( nStart <= strVirtualPath.size() )  {
                size_t  nEnd = strVirtualPath.find( '/', nStart );

                if( nEnd == std :: string :: npos )
                    nEnd = strVirtualPath.size();

                std :: string  strPart = strVirtualPath.substr( nStart, nEnd - nStart );

                nStart = nEnd + 1;

                if( strPart.empty() )
                    continue;

                if( ( strPart == "." ) || ( strPart == ".." ) )
                    return false;

                real /= strPart;
                bAny  = true;
            }

            if( !bAny )
                return false;

            strRealPath = real.string();

            return true;
        }
    }
}
