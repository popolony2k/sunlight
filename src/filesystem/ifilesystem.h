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
         * Path schemes (I12 of the master plan): every path given to
         * @see Exists, @see ReadFile and @see WriteFile may start with a
         * scheme naming where it lives - "vpath://" or "virtualpath://"
         * for a virtual path (the default: a path with no scheme is one,
         * so every path written before schemes existed means what it
         * did), and "hostpath://" or "hpath://" for a real OS path in the
         * OS's own style (a Windows drive letter included). Any other
         * scheme is refused. Schemes are lowercase. This class resolves the scheme, reads and
         * writes host paths itself, honours the host-path switch and
         * applies the read and write filters - once, for every backend;
         * a backend implements only the virtual-path part (the protected
         * Virtual* methods).
         */
        class IFileSystem  {

            public:

            /**
             * @brief Where a path lives, by its scheme (see the class comment).
             */
            enum PathScheme  {
                PATH_SCHEME_VIRTUAL,    // no scheme, "vpath://" or "virtualpath://"
                PATH_SCHEME_HOST,       // "hostpath://" or "hpath://"
                PATH_SCHEME_UNKNOWN     // any other "<letters>://": refused
            };

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
             * @brief The scheme a path starts with, and the path without it.
             * A scheme is the letters before "://" at the start of the path;
             * a path without one is virtual.
             * @param strPath The path, with or without a scheme;
             * @param strRest Receives the path without its scheme;
             * @return Where the path lives;
             */
            static PathScheme SplitPath( const std :: string& strPath, std :: string& strRest );

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
            void SetReadFilter( ReadFilterCallback callback );

            /**
             * @brief Register an optional filter applied to the bytes of
             * every @see WriteFile before they are written - virtual and
             * host paths alike. Unset by default: the bytes are written as
             * given.
             * @param callback The filter, or an empty WriteFilterCallback
             * to clear a previously-set one;
             */
            void SetWriteFilter( WriteFilterCallback callback );

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
            void SetHostPathEnabled( bool bEnabled );
            bool IsHostPathEnabled( void ) const;

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
             * LoadFont("resources/fonts/foo.fnt") call therefore reaches
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
            static std :: string ToVirtualPath( const std :: string& strRealPath );

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
            virtual bool Mount( const std :: string& strRealPath, const std :: string& strMountPoint, bool bAppendToPath ) = 0;

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
            bool Exists( const std :: string& strPath );

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
            bool ReadFile( const std :: string& strPath, std :: vector<unsigned char>& outData );

            /**
             * @brief Write an entire file in one call, through the write
             * filter if one is set, replacing any file there: a virtual
             * path into the write directory (@see SetWriteDirectory), a
             * host path to the OS. Missing folders on the way are created.
             * @param strPath The path;
             * @param data The bytes;
             * @return true on success; false if it couldn't be written, a
             * virtual path while no write directory is set, an unknown
             * scheme, or a host path while host paths are refused;
             */
            bool WriteFile( const std :: string& strPath, const std :: vector<unsigned char>& data );

            protected:

            // The virtual-path part a backend implements: the path has no scheme, and no filter
            // is applied here (WriteFile/ReadFile apply them).
            virtual bool VirtualExists( const std :: string& strVirtualPath ) = 0;
            virtual bool VirtualReadFile( const std :: string& strVirtualPath, std :: vector<unsigned char>& outData ) = 0;
            virtual bool VirtualWriteFile( const std :: string& strVirtualPath, const std :: vector<unsigned char>& data ) = 0;

            private:

            ReadFilterCallback   m_ReadFilter;               // empty (unset) by default
            WriteFilterCallback  m_WriteFilter;              // empty (unset) by default
            bool                 m_bHostPathEnabled = true;
        };
    }
}

#endif /* __IFILESYSTEM_H__ */
