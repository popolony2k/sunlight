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
         */
        class IFileSystem  {

            public:

            virtual ~IFileSystem( void )  {}

            /**
             * @brief Converts a real, OS-native path into its own legal
             * virtual-path equivalent - normalizing '\' to '/', stripping
             * a Windows drive letter prefix ("C:", "D:", ...), and
             * stripping a leading "./". A real path is always safe to
             * pass as @see Mount's own strRealPath (source) argument, in
             * whatever native format the OS gives it - but never as
             * strMountPoint (destination), and never as @see ReadFile/
             * @see Exists's own strVirtualPath: PhysFS's own path
             * sanitizer (verified directly in it's own source) rejects
             * both a raw ':' anywhere in the string, and a "." path
             * segment specifically - which a leading "./" always produces
             * - as outright illegal, not just a mount-point restriction.
             *
             * The "./" case is a real, live-reproduced bug distinct from
             * the drive-letter one: raylib's own LoadBMFont (rtext.c)
             * builds a multi-file AngelCode BMFont's atlas image path via
             * GetDirectoryPath(fileName), which - confirmed directly in
             * raylib's own source - always prepends "./" to the result
             * whenever fileName has no drive letter and doesn't already
             * start with '/' (the common, relative-path case). A
             * SetFont("resources/fonts/foo.fnt") call therefore reaches
             * RaylibEngine's own FileSystemLoadFileDataCallback asking to
             * read "./resources/fonts/foo.png" - which PHYSFS_openRead()
             * silently rejects, with no trace at all (that total silence
             * is itself the diagnostic tell), falling back to raylib's
             * default font with no visible error. Single-file TTF/OTF
             * fonts are unaffected - no such internally-constructed
             * secondary path exists for them.
             *
             * Idempotent - safe to call on a string that's already a
             * legal virtual path (eg. "/", or this method's own prior
             * output), since none of these transformations touch it
             * further. A consumer needing to mount it's own real
             * directory "at itself" must call this explicitly and use the
             * *same* resulting string both as Mount()'s own mountPoint
             * argument and as the prefix for any virtual path it later
             * constructs against that mount (eg. exposing it to a
             * scripting layer as an "application directory" global) -
             * Mount() itself cannot safely do this conversion internally
             * on the caller's behalf, since ReadFile/Exists lookups
             * against virtual paths built from the *unconverted* real
             * path would then no longer match whatever Mount() actually
             * mounted things at.
             *
             * @param strRealPath Real, OS-native path to convert;
             * @return strRealPath's own equivalent, legal virtual path;
             */
            static std :: string ToVirtualPath( const std :: string &strRealPath );

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
            virtual bool Init( const char *szArgv0 ) = 0;

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
             * (see appendToPath).
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
            virtual bool Mount( const std :: string &strRealPath, const std :: string &strMountPoint, bool bAppendToPath ) = 0;

            /**
             * @brief Must be implemented to report whether a file exists
             * at the given virtual path, across every currently mounted
             * location.
             * @param strVirtualPath Virtual path to check;
             * @return true if a readable file exists there, false
             * otherwise (including when nothing has been mounted yet);
             */
            virtual bool Exists( const std :: string &strVirtualPath ) = 0;

            /**
             * @brief Must be implemented to read an entire file's
             * contents into memory in one call, resolving it against
             * every currently mounted location the same way @see Exists
             * does.
             * @param strVirtualPath Virtual path to read;
             * @param outData Receives the file's own raw bytes on
             * success; left untouched on failure;
             * @return true on success, false if the file doesn't exist or
             * couldn't be read;
             */
            virtual bool ReadFile( const std :: string &strVirtualPath, std :: vector<unsigned char> &outData ) = 0;
        };
    }
}

#endif  /* __IFILESYSTEM_H__ */
