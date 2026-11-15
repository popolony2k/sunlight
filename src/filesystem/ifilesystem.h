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

#ifndef __IFILESYSTEM_H__
#define __IFILESYSTEM_H__

#include <string>
#include <vector>
#include <functional>


namespace SunLight  {
    namespace FileSystem  {

        /**
         * @brief Backend resource-filesystem generic interface. A single
         * mounted resource root - either a real, loose directory on disk
         * or a real archive (.zip, ...) - is read through identically by
         * every caller; nothing above this interface needs to know or
         * care which one is actually mounted. Every concrete backend
         * (PhysFS, ...) implements this using it's own native library
         * internally, exposing only plain SunLight types here.
         *
         * A "virtual path" throughout this interface is a path relative
         * to whatever was mounted, always using '/' as the separator
         * (matching the underlying PhysFS convention) - never an absolute
         * OS path, and never OS-specific separators.
         *
         * Every path may carry a scheme - see FilePath (I12 of the master plan). This is a pure
         * interface (I13): the shared behaviour every backend has - schemes, host paths, the
         * host-path switch, the read and write filters - is BaseFileSystem's, which a backend
         * derives from.
         */
        class IFileSystem  {

            public:

            /**
             * @brief Optional post-read transform: given the raw bytes
             * @see ReadFile just read, may replace them with something
             * else before they reach the caller. Returns true if it
             * replaced outData with new content, false to leave the
             * original bytes untouched. IFileSystem has no opinion on
             * what a registered callback does - eg. a consumer can use
             * this to transparently decrypt content packed with it's own
             * build-time shared secret, with sunlight itself carrying no
             * crypto dependency or awareness of what the callback does. A
             * consumer that never calls @see SetReadFilter never has this
             * called at all, and @see ReadFile behaves exactly as it
             * always has.
             */
            typedef std :: function<bool( const std :: vector<unsigned char>& in, std :: vector<unsigned char>& out )>  ReadFilterCallback;

            /**
             * @brief Optional pre-write transform, the counterpart of
             * @see ReadFilterCallback: given the bytes @see WriteFile was
             * asked to write, may replace them before they are written
             * (true), or leave them as they are (false). A game that
             * decrypts its pack with a read filter encrypts its own saved
             * files with the matching write filter, so they read back
             * through the same pair.
             */
            typedef std :: function<bool( const std :: vector<unsigned char>& in, std :: vector<unsigned char>& out )>  WriteFilterCallback;

            virtual ~IFileSystem( void )  {}

            /**
             * @brief Must be implemented to register an optional filter
             * applied to every @see ReadFile result, after the raw bytes
             * are read but before they reach the caller. Unset (the
             * default) means @see ReadFile behaves exactly as before this
             * hook existed - fully backward compatible; only invoked
             * after a fully successful raw read, never for a missing/
             * unreadable file (that still fails exactly as before).
             * @param callback The filter to apply, or an empty
             * ReadFilterCallback to clear a previously-set one;
             */
            virtual void SetReadFilter( ReadFilterCallback callback ) = 0;

            /**
             * @brief Register an optional filter applied to the bytes of
             * every @see WriteFile before they are written - virtual and
             * host paths alike. Unset by default: the bytes are written as
             * given.
             * @param callback The filter, or an empty WriteFilterCallback
             * to clear a previously-set one;
             */
            virtual void SetWriteFilter( WriteFilterCallback callback ) = 0;

            /**
             * @brief Allow or refuse host paths ("hostpath://", "hpath://").
             * Allowed by default. Refused, every host-path @see Exists,
             * @see ReadFile and @see WriteFile fails - eg. a scripting layer
             * that must keep its scripts inside the virtual path space.
             * Mounting a real path (@see Mount, @see SetWriteDirectory) is
             * the program's own setup, not a host-path access, and is not
             * affected.
             * @param bEnabled Whether host paths are allowed;
             */
            virtual void SetHostPathEnabled( bool bEnabled ) = 0;
            virtual bool IsHostPathEnabled( void ) const = 0;

            /**
             * @brief Must be implemented to perform whatever one-time
             * startup the concrete backend needs before @see Mount can be
             * called - eg. PhysFS's own PHYSFS_init(). Safe to call more
             * than once; implementations should no-op if already
             * initialized.
             * @param szArgv0 The running process's own argv[0], forwarded
             * unchanged from main() - some backends use this to locate
             * platform-specific support files;
             * @return true on success, false on failure;
             */
            virtual bool Init( const char* szArgv0 ) = 0;

            /**
             * @brief Must be implemented to release whatever @see Init
             * acquired. Safe to call even if @see Init was never called
             * or already failed.
             */
            virtual void Shutdown( void ) = 0;

            /**
             * @brief Must be implemented to mount a real, on-disk
             * location - a loose directory or a real archive file, the
             * concrete backend auto-detects which - so it's contents
             * become readable through this interface's own virtual path
             * space. Multiple mounts can coexist, stacked in mount order
             * (see appendToPath) - the same location included, at several
             * mount points (I22 of the master plan), each mount in its own
             * place in that order; mounting the same location at the same
             * mount point again changes nothing and succeeds.
             * @param strRealPath Real, OS-native path to the directory or
             * archive to mount;
             * @param strMountPoint Where in the virtual path space this
             * mount's own contents appear - "/" mounts at the virtual
             * root;
             * @param bAppendToPath true to search this mount only after
             * every already-mounted location (the usual case), false to
             * search it first;
             * @return true on success, false on failure (eg. the path
             * doesn't exist, or isn't a directory/archive the backend
             * recognizes);
             */
            virtual bool Mount( const std :: string& strRealPath, const std :: string& strMountPoint, bool bAppendToPath ) = 0;

            /**
             * @brief Must be implemented to remove ONE mount made by @see Mount (I22 of the master
             * plan): the one of that real path at that mount point - the same folder or archive may
             * be mounted at several mount points, each in its own place in the search order, and the
             * others stay. The write directory's own mount (@see SetWriteDirectory) is not removed by it.
             * @param strRealPath The real path given to Mount;
             * @param strMountPoint The mount point given to Mount ("/" and "" are the same);
             * @return false when no such mount exists;
             */
            virtual bool Unmount( const std :: string& strRealPath, const std :: string& strMountPoint ) = 0;

            /**
             * @brief Must be implemented to set the real folder a virtual
             * path is written into (@see WriteFile) - archives are
             * read-only, so a virtual write never goes into one. The
             * folder is created if it does not exist, and is also mounted
             * at the virtual root ahead of every other mount, so a file
             * written there reads back by the same virtual path and wins
             * over a shipped file of the same name (user settings over a
             * game's defaults). It does not count as a mount for the
             * automatic mount of the working directory (PhysFS backend).
             * @param strRealPath Real, OS-native path of the folder;
             * @return true on success;
             */
            virtual bool SetWriteDirectory( const std :: string& strRealPath ) = 0;

            /**
             * @brief Whether a file exists at a path (see the path schemes
             * in the class comment): across every mounted location for a
             * virtual path, on the OS for a host path.
             * @param strPath The path;
             * @return true if a readable file exists there; false
             * otherwise, for an unknown scheme, or a host path while host
             * paths are refused;
             */
            virtual bool Exists( const std :: string& strPath ) = 0;

            /**
             * @brief Read an entire file into memory in one call, from a
             * virtual or a host path (see the path schemes in the class
             * comment), then through the read filter, if one is set.
             * @param strPath The path;
             * @param outData Receives the bytes on success; left untouched
             * on failure;
             * @return true on success; false if the file doesn't exist or
             * couldn't be read, for an unknown scheme, or a host path
             * while host paths are refused;
             */
            virtual bool ReadFile( const std :: string& strPath, std :: vector<unsigned char>& outData ) = 0;

            /**
             * @brief Write an entire file in one call, through the write
             * filter if one is set, replacing any file there: a virtual
             * path into the write directory (@see SetWriteDirectory), a
             * host path to the OS. Missing folders on the way are created.
             * Atomic (I14 of the master plan): the bytes go to a temporary
             * file beside the target ("<name>.tmp"), flushed to the disk,
             * which then replaces the target in one rename - a crash, a
             * power loss or a full disk leaves the old file or the new one,
             * never an empty or half-written one. A process killed in the
             * middle can leave the ".tmp" beside the target; the next write
             * of that file replaces it.
             * @param strPath The path;
             * @param data The bytes;
             * @return true on success; false if it couldn't be written (the
             * old file then is as it was), a virtual path while no write
             * directory is set, an unknown scheme, or a host path while host
             * paths are refused;
             */
            virtual bool WriteFile( const std :: string& strPath, const std :: vector<unsigned char>& data ) = 0;

            /**
             * @brief Remove a file (I14 of the master plan), under the same
             * rules as @see WriteFile: a virtual path only inside the write
             * directory - never a file of a mounted archive or folder, which
             * are read-only, so removing a saved file there makes a shipped
             * file of the same name readable again - and a host path only
             * while host paths are allowed. Files only: a folder is never
             * removed.
             * @param strPath The path;
             * @return true if a file was removed; false if there was none
             * (so a caller can tell nothing was there), it is a folder, it
             * couldn't be removed, a virtual path while no write directory
             * is set, an unknown scheme, or a host path while host paths
             * are refused;
             */
            virtual bool RemoveFile( const std :: string& strPath ) = 0;
        };
    }
}

#endif /* __IFILESYSTEM_H__ */
