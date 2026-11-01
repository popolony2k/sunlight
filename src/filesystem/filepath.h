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

#ifndef __FILEPATH_H__
#define __FILEPATH_H__

#include <string>


namespace SunLight  {
    namespace FileSystem  {

        /**
         * @brief The FileSystem's path helpers: pure string logic, the same for every backend, so
         * all static (I13 of the master plan - they lived on IFileSystem, which is an interface
         * again: see BaseFileSystem).
         *
         * Path schemes (I12 of the master plan): every path given to IFileSystem::Exists,
         * IFileSystem::ReadFile and IFileSystem::WriteFile may start with a scheme naming where it
         * lives - "vpath://" or "virtualpath://" for a virtual path (the default: a path with no
         * scheme is one, so every path written before schemes existed means what it did), and
         * "hostpath://" or "hpath://" for a real OS path in the OS's own style (a Windows drive
         * letter included). Any other scheme is refused. Schemes are lowercase. BaseFileSystem
         * resolves them (SplitPath) for every backend.
         */
        class FilePath  {

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
             * @brief The scheme a path starts with, and the path without it.
             * A scheme is the letters before "://" at the start of the path;
             * a path without one is virtual.
             * @param strPath The path, with or without a scheme;
             * @param strRest Receives the path without its scheme;
             * @return Where the path lives;
             */
            static PathScheme SplitPath( const std :: string& strPath, std :: string& strRest );

            /**
             * @brief The folder part of a path, its trailing '/' kept ("maps/level.tmx" gives
             * "maps/", "level.tmx" gives ""). A scheme stays in front ("vpath://maps/level.tmx"
             * gives "vpath://maps/", "vpath://level.tmx" gives "vpath://"). Its result is what
             * @ref JoinPath expects as strDir: the pair builds a path relative to a file's folder.
             * Pure string logic, the same for every backend (I13 of the master plan).
             * @param strPath The path, with or without a scheme;
             * @return The folder part, trailing '/' included; "" when there is none (after any
             * scheme);
             */
            static std :: string ParentPath( const std :: string& strPath );

            /**
             * @brief strDir and strRelative as one path: strDir is glued to strRelative as it is (so
             * it ends with '/', as @ref ParentPath gives it), backslashes become '/', and "." and
             * ".." are collapsed lexically (virtual paths reject them). strDir's scheme stays in
             * front and only the rest is collapsed. Every empty segment is dropped, so a strRelative
             * starting with '/' stays inside strDir ("dir/" + "/main.lua" gives "dir/main.lua");
             * the result is absolute only when the joined path itself starts with '/' (strDir empty
             * or absolute). A host path keeps the OS's rules: absolute stays absolute, and ".."
             * never climbs above a Windows drive ("C:"). A strRelative with a scheme of its own
             * already says where it lives and is used alone. Pure string logic, the same for every
             * backend (I13 of the master plan).
             * @param strDir The folder, with or without a scheme;
             * @param strRelative The path relative to it;
             * @return The joined path; "" when ".." would climb above the root (or a drive) - not
             * a legal path;
             */
            static std :: string JoinPath( const std :: string& strDir, const std :: string& strRelative );

            /**
             * @brief Converts a real, OS-native path into its own legal
             * virtual-path equivalent - normalizing '\' to '/', stripping
             * a Windows drive letter prefix ("C:", "D:", ...), and
             * stripping a leading "./". A real path is always safe to
             * pass as IFileSystem::Mount's own strRealPath (source) argument, in
             * whatever native format the OS gives it - but never as
             * strMountPoint (destination), and never as IFileSystem::ReadFile/
             * IFileSystem::Exists's own strVirtualPath: PhysFS's own path
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
        };
    }
}

#endif /* __FILEPATH_H__ */
