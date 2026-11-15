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
                m_Mounts.clear();
            }

            /**
             * @brief PHYSFS_mount() itself already auto-detects whether
             * strRealPath is a loose directory or a real archive file -
             * nothing here needs to branch on that. Through the mount table
             * (MountEntry, I22 of the master plan), so the same location can
             * be mounted at several mount points.
             * @param strRealPath The real path of the folder or archive;
             * @param strMountPoint Where its contents appear ("/": the root);
             * @param bAppendToPath true to search it after every other mount, false before;
             * @return false when PhysFS cannot mount it;
             */
            bool PhysFsFileSystem :: Mount( const std :: string& strRealPath, const std :: string& strMountPoint, bool bAppendToPath )  {

                // A mount can be the first call a consumer makes, before any read, so PhysFS is
                // set up here too. Init does nothing when it is already set up (Scarab calls it first).
                if( !m_bInitialized )
                    Init( nullptr );

                bool  bMounted = MountEntry( strRealPath, strMountPoint, bAppendToPath, false );

                if( bMounted )
                    m_bAnyMounted = true;

                return bMounted;
            }

            /**
             * @brief Remove one mount made by Mount (I22 of the master plan): the table finds the name
             * PhysFS knows that (real path, mount point) by, and PHYSFS_unmount removes it alone - the
             * other mounts of the same path, and the write directory's, stay.
             * @param strRealPath The real path given to Mount;
             * @param strMountPoint The mount point given to Mount;
             * @return false when there is no such mount;
             */
            bool PhysFsFileSystem :: Unmount( const std :: string& strRealPath, const std :: string& strMountPoint )  {

                return m_bInitialized ? UnmountEntry( strRealPath, strMountPoint, false ) : false;
            }

            /**
             * @brief A mount point as PhysFS compares them: without leading or trailing '/' ("/" and ""
             * are both the root).
             * @param strMountPoint The mount point;
             * @return It, trimmed;
             */
            static std :: string NormalizedMountPoint( const std :: string& strMountPoint )  {

                size_t  nStart = strMountPoint.find_first_not_of( '/' );

                if( nStart == std :: string :: npos )
                    return std :: string();

                size_t  nEnd = strMountPoint.find_last_not_of( '/' );

                return strMountPoint.substr( nStart, nEnd - nStart + 1 );
            }

            /**
             * @brief The name to give PhysFS for one more mount of a real path (I22 of the master plan).
             * PhysFS keeps one mount per path STRING: doMount (physfs.c) returns success without mounting
             * when the same string is already in the search path, at any mount point. It stores and opens
             * the string as given (createDirHandle, openDirectory: no normalisation), so an equivalent
             * spelling of the same folder or archive - "./" put in front of its last component, once per
             * mount already using a spelling - mounts it again, in its own place in the search order. The
             * extension, which picks the archive reader, is untouched. The plain path is used when free.
             * @param strRealPath The real path;
             * @return A name no mount uses yet;
             */
            std :: string PhysFsFileSystem :: UniquePhysName( const std :: string& strRealPath ) const  {

                const std :: string  strSeparator = ::PHYSFS_getDirSeparator();
                const std :: string  strDot       = "." + strSeparator;

                // The path without trailing separators, split before its last component.
                std :: string  strPath = strRealPath;

                while( ( strPath.size() > 1 ) && ( ( strPath.back() == '/' ) || ( strPath.back() == strSeparator[0] ) ) )
                    strPath.pop_back();

                size_t  nLast = strPath.find_last_of( "/" + strSeparator );
                std :: string  strHead = ( nLast == std :: string :: npos ) ? std :: string() : strPath.substr( 0, nLast + 1 );
                std :: string  strTail = ( nLast == std :: string :: npos ) ? strPath : strPath.substr( nLast + 1 );
                std :: string  strCandidate = strRealPath;

                for( size_t nDots = 1; ; nDots++ )  {
                    bool  bUsed = false;

                    for( const stMount& mount : m_Mounts )  {
                        if( mount.strPhysName == strCandidate )  {
                            bUsed = true;
                            break;
                        }
                    }

                    if( !bUsed )
                        return strCandidate;

                    strCandidate = strHead;

                    for( size_t nDot = 0; nDot < nDots; nDot++ )
                        strCandidate += strDot;

                    strCandidate += strTail;
                }
            }

            /**
             * @brief Mount a real path at a mount point and record it (I22 of the master plan). The same
             * pair of the same kind already mounted changes nothing and succeeds (as PhysFS did); a new
             * pair gets a name of its own (UniquePhysName), so the same folder or archive can be mounted
             * at several mount points.
             * @param strRealPath The real path;
             * @param strMountPoint The mount point;
             * @param bAppendToPath true to search it after every other mount, false before;
             * @param bWriteDir Whether this is the write directory's own mount;
             * @return false when PhysFS cannot mount it;
             */
            bool PhysFsFileSystem :: MountEntry( const std :: string& strRealPath, const std :: string& strMountPoint, bool bAppendToPath, bool bWriteDir )  {

                std :: string  strNormalized = NormalizedMountPoint( strMountPoint );

                for( const stMount& mount : m_Mounts )  {
                    if( ( mount.bWriteDir == bWriteDir ) && ( mount.strRealPath == strRealPath ) && ( mount.strMountPoint == strNormalized ) )
                        return true;
                }

                std :: string  strPhysName = UniquePhysName( strRealPath );

                if( ::PHYSFS_mount( strPhysName.c_str(), strMountPoint.c_str(), bAppendToPath ? 1 : 0 ) == 0 )
                    return false;

                m_Mounts.push_back( stMount { strRealPath, strNormalized, strPhysName, bWriteDir } );

                return true;
            }

            /**
             * @brief Remove one recorded mount (I22 of the master plan): PHYSFS_unmount by the name
             * PhysFS knows it by, then out of the table.
             * @param strRealPath The real path;
             * @param strMountPoint The mount point;
             * @param bWriteDir Whether it is the write directory's own mount;
             * @return false when there is no such mount;
             */
            bool PhysFsFileSystem :: UnmountEntry( const std :: string& strRealPath, const std :: string& strMountPoint, bool bWriteDir )  {

                std :: string  strNormalized = NormalizedMountPoint( strMountPoint );

                for( std :: vector<stMount> :: iterator itMount = m_Mounts.begin(); itMount != m_Mounts.end(); itMount++ )  {
                    if( ( itMount -> bWriteDir == bWriteDir ) && ( itMount -> strRealPath == strRealPath ) &&
                        ( itMount -> strMountPoint == strNormalized ) )  {
                        if( ::PHYSFS_unmount( itMount -> strPhysName.c_str() ) == 0 )
                            return false;

                        m_Mounts.erase( itMount );

                        return true;
                    }
                }

                return false;
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
             * mount is removed first. Both through the mount table (I22), as the write directory's own
             * entry: a game's mount of the same folder elsewhere is a mount of its own, untouched.
             */
            bool PhysFsFileSystem :: SetWriteDirectory( const std :: string& strRealPath )  {

                if( !m_bInitialized )
                    Init( nullptr );

                std :: error_code  errorCode;

                std :: filesystem :: create_directories( strRealPath, errorCode );

                if( ::PHYSFS_setWriteDir( strRealPath.c_str() ) == 0 )
                    return false;

                // I22: through the mount table, as the write directory's own entry - so a game's own
                // mount of the same folder neither hides this one (PhysFS ignored a second mount of the
                // same path) nor is removed in its place when the write directory changes.
                if( !m_strWriteDir.empty() )
                    UnmountEntry( m_strWriteDir, "/", true );

                m_strWriteDir.clear();

                if( !MountEntry( strRealPath, "/", false, true ) )
                    return false;

                m_strWriteDir = strRealPath;

                return true;
            }

            /**
             * @brief Whether anything is at a virtual path in any mount - the write directory first,
             * when one is set - with the working directory mounted first if nothing was (EnsureReady).
             * PHYSFS_exists answers for any entry it finds (getRealDirHandle in physfs.c), so a folder
             * counts as well as a file.
             * @param strVirtualPath The virtual path, without its scheme;
             * @return true if PhysFS finds a file or a folder there;
             */
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

                // The read filter is BaseFileSystem::ReadFile's, applied after this returns.
                outData = std :: move( data );

                return true;
            }

            /**
             * @brief Writes the whole file into the write directory (SetWriteDirectory), creating the
             * folders on the way; false with no write directory set. Atomic (I14 of the master plan):
             * PHYSFS_openWrite truncates the target before the first byte (physfs.h) and PhysFS has no
             * rename, so the write goes to the write directory's real folder through
             * BaseFileSystem::WriteRealFile - a temporary file beside the target, flushed, then renamed
             * over it. The virtual path is mapped inside that folder with PhysFS's own rules
             * (RealWritePath), so it can never leave it. PhysFS reads the folder as it is on the disk
             * (a mounted native directory), so the new file reads back at once.
             */
            bool PhysFsFileSystem :: VirtualWriteFile( const std :: string& strVirtualPath, const std :: vector<unsigned char>& data )  {

                std :: string  strRealPath;

                if( !RealWritePath( m_strWriteDir, strVirtualPath, strRealPath ) )
                    return false;

                return WriteRealFile( strRealPath, data );
            }

            /**
             * @brief Removes a file of the write directory only (I14 of the master plan): the mounts
             * are read-only, so a shipped file of the same name stays, and reads again once the saved
             * one, which shadowed it, is gone. False with no write directory set.
             */
            bool PhysFsFileSystem :: VirtualRemoveFile( const std :: string& strVirtualPath )  {

                std :: string  strRealPath;

                if( !RealWritePath( m_strWriteDir, strVirtualPath, strRealPath ) )
                    return false;

                return RemoveRealFile( strRealPath );
            }
        }
    }
}
