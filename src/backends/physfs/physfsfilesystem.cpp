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

#include "backends/physfs/physfsfilesystem.h"

#include <physfs.h>
#include <filesystem>
#include <system_error>


namespace SunLight  {
    namespace FileSystem  {
        namespace PhysFs  {

            /**
             * @brief Calls PHYSFS_init() once - safe to call repeatedly,
             * matches every other real Init in this codebase's own
             * idempotent-on-repeat convention (eg. TileMapRenderer's own
             * one-shot setup steps).
             */
            bool PhysFsFileSystem :: Init( const char* szArgv0 )  {

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
                m_strWriteDir.clear();      // PHYSFS_deinit forgets the write directory and every mount
            }

            /**
             * @brief PHYSFS_mount() itself already auto-detects whether
             * strRealPath is a loose directory or a real archive file -
             * nothing here needs to branch on that.
             */
            bool PhysFsFileSystem :: Mount( const std :: string& strRealPath, const std :: string& strMountPoint, bool bAppendToPath )  {

                // A mount can be the first call a consumer makes, before any read, so PhysFS is
                // set up here too. Init does nothing when it is already set up (Scarab calls it first).
                if( !m_bInitialized )
                    Init( nullptr );

                bool  bMounted = ::PHYSFS_mount( strRealPath.c_str(), strMountPoint.c_str(), bAppendToPath ? 1 : 0 ) != 0;

                if( bMounted )
                    m_bAnyMounted = true;

                return bMounted;
            }

            /**
             * @brief Lazy, run-once default: if nothing has been mounted
             * by the time the very first read happens, mounts the
             * process's own current working directory (".") at the
             * virtual root, so a consumer that never calls @see Mount at
             * all still gets exactly the same behavior plain OS file
             * access always gave before this filesystem abstraction
             * existed - "do nothing" must never mean "every load silently
             * fails" (this is a real fix, not a
             * hypothetical: it's the exact gap that broke every one of
             * sunlight's own bundled samples the first time this
             * abstraction shipped, since none of them called Mount).
             * A consumer wanting real archive support still calls @see
             * Mount explicitly - with strAppendToPath=false, or simply
             * before the first read - and that mount is what's already in
             * place by the time this runs, so this fallback never
             * displaces it (@see m_bAnyMounted is already true by then).
             *
             * CWD, not IEngine::GetApplicationDirectory() - verified live
             * these are NOT the same thing for every consumer: sunlight's own
             * bundled samples are documented to run as
             * `<build-dir>/samples/.../foo_test samples/foo/` from the
             * checkout root, so their own relative resource paths (built
             * from argv[1]) resolve against the *invocation* directory,
             * which is nowhere near wherever the built binary itself
             * lives. Mounting "." at the virtual root reproduces exactly
             * what a plain relative-path fopen()/ifstream always resolved
             * against - CWD, unconditionally - which is what every
             * consumer that never calls Mount() was actually relying on
             * before this abstraction existed. (Scarab's own consumers
             * work either way, since it explicitly Mounts its own
             * application directory before any read happens - this
             * fallback never even runs for it.) PHYSFS_init(nullptr) is
             * safe here specifically because nothing in this class relies
             * on PhysFS's own argv0-derived base-dir detection
             * (PHYSFS_getBaseDir()) - the real mount path always comes
             * from "." instead, so the "some Unix systems need argv[0]"
             * caveat in PhysFS's own docs doesn't apply to this usage.
             */
            void PhysFsFileSystem :: EnsureReady( void )  {

                if( m_bDefaultMountEnsured )
                    return;

                m_bDefaultMountEnsured = true;

                if( !m_bInitialized )
                    Init( nullptr );

                if( !m_bAnyMounted )
                    Mount( ".", "/", true );
            }

            /**
             * @brief PHYSFS_setWriteDir, then the same folder mounted at the virtual root ahead of
             * every other mount (PHYSFS_mount with append 0), so what is written reads back by the
             * same virtual path and wins over a shipped file of the same name. It does not set
             * m_bAnyMounted: a consumer that never mounts still gets the working directory mounted
             * on the first read (EnsureReady), behind this folder. A previous write directory's
             * mount is removed first.
             */
            bool PhysFsFileSystem :: SetWriteDirectory( const std :: string& strRealPath )  {

                if( !m_bInitialized )
                    Init( nullptr );

                std :: error_code  errorCode;

                std :: filesystem :: create_directories( strRealPath, errorCode );

                if( ::PHYSFS_setWriteDir( strRealPath.c_str() ) == 0 )
                    return false;

                if( !m_strWriteDir.empty() )
                    ::PHYSFS_unmount( m_strWriteDir.c_str() );

                m_strWriteDir.clear();

                if( ::PHYSFS_mount( strRealPath.c_str(), "/", 0 ) == 0 )
                    return false;

                m_strWriteDir = strRealPath;

                return true;
            }

            bool PhysFsFileSystem :: VirtualExists( const std :: string& strVirtualPath )  {

                EnsureReady();

                return ::PHYSFS_exists( strVirtualPath.c_str() ) != 0;
            }

            /**
             * @brief Reads the whole file in one PHYSFS_readBytes() call -
             * every resource this engine loads (textures, sounds, maps,
             * scripts) is small enough that streaming isn't warranted, the
             * same "load it all up front" assumption raylib's own
             * LoadFileData() already makes.
             */
            bool PhysFsFileSystem :: VirtualReadFile( const std :: string& strVirtualPath, std :: vector<unsigned char>& outData )  {

                EnsureReady();

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

                // The read filter is IFileSystem::ReadFile's, applied after this returns.
                outData = std :: move( data );

                return true;
            }

            /**
             * @brief Writes the whole file into the write directory (SetWriteDirectory), creating the
             * folders on the way (PHYSFS_mkdir creates every missing one); false with no write
             * directory set. PhysFS takes a write path relative to the write directory.
             */
            bool PhysFsFileSystem :: VirtualWriteFile( const std :: string& strVirtualPath, const std :: vector<unsigned char>& data )  {

                if( m_strWriteDir.empty() || ( ::PHYSFS_getWriteDir() == nullptr ) )
                    return false;

                size_t  nLastSlash = strVirtualPath.find_last_of( '/' );

                if( ( nLastSlash != std :: string :: npos ) && ( nLastSlash > 0 ) &&
                    ( ::PHYSFS_mkdir( strVirtualPath.substr( 0, nLastSlash ).c_str() ) == 0 ) )
                    return false;

                PHYSFS_File  *pFile = ::PHYSFS_openWrite( strVirtualPath.c_str() );

                if( !pFile )
                    return false;

                PHYSFS_sint64  nWritten = data.empty() ? 0 : ::PHYSFS_writeBytes( pFile, data.data(), ( PHYSFS_uint64 ) data.size() );
                bool           bClosed  = ::PHYSFS_close( pFile ) != 0;

                return bClosed && ( nWritten == ( PHYSFS_sint64 ) data.size() );
            }
        }
    }
}
