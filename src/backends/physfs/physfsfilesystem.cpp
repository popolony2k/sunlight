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

#include "physfsfilesystem.h"

#include <physfs.h>


namespace SunLight  {
    namespace FileSystem  {
        namespace PhysFs  {

            /**
             * @brief Calls PHYSFS_init() once - safe to call repeatedly,
             * matches every other real Init in this codebase's own
             * idempotent-on-repeat convention (eg. TileMapRenderer's own
             * one-shot setup steps).
             */
            bool PhysFsFileSystem :: Init( const char *szArgv0 )  {

                if( m_bInitialized )
                    return true;

                if( !::PHYSFS_init( szArgv0 ) )
                    return false;

                m_bInitialized = true;

                return true;
            }

            /**
             * @brief Calls PHYSFS_deinit() - safe even if Init() was never
             * called or failed (PHYSFS_isInit() guards it).
             */
            void PhysFsFileSystem :: Shutdown( void )  {

                if( !m_bInitialized )
                    return;

                if( ::PHYSFS_isInit() )
                    ::PHYSFS_deinit();

                m_bInitialized = false;
            }

            /**
             * @brief PHYSFS_mount() itself already auto-detects whether
             * strRealPath is a loose directory or a real archive file -
             * nothing here needs to branch on that.
             */
            bool PhysFsFileSystem :: Mount( const std :: string &strRealPath, const std :: string &strMountPoint, bool bAppendToPath )  {

                return ::PHYSFS_mount( strRealPath.c_str(), strMountPoint.c_str(), bAppendToPath ? 1 : 0 ) != 0;
            }

            bool PhysFsFileSystem :: Exists( const std :: string &strVirtualPath )  {

                return ::PHYSFS_exists( strVirtualPath.c_str() ) != 0;
            }

            /**
             * @brief Reads the whole file in one PHYSFS_readBytes() call -
             * every resource this engine loads (textures, sounds, maps,
             * scripts) is small enough that streaming isn't warranted, the
             * same "load it all up front" assumption raylib's own
             * LoadFileData() already makes.
             */
            bool PhysFsFileSystem :: ReadFile( const std :: string &strVirtualPath, std :: vector<unsigned char> &outData )  {

                PHYSFS_File  *pFile = ::PHYSFS_openRead( strVirtualPath.c_str() );

                if( !pFile )
                    return false;

                PHYSFS_sint64  nSize = ::PHYSFS_fileLength( pFile );

                if( nSize < 0 )  {
                    ::PHYSFS_close( pFile );

                    return false;
                }

                std :: vector<unsigned char>  data( ( size_t ) nSize );

                PHYSFS_sint64  nRead = ( nSize > 0 ) ? ::PHYSFS_readBytes( pFile, data.data(), ( PHYSFS_uint64 ) nSize ) : 0;

                ::PHYSFS_close( pFile );

                if( nRead != nSize )
                    return false;

                outData = std :: move( data );

                return true;
            }
        }
    }
}
