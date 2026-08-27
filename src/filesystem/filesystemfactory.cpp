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

#include "filesystemfactory.h"

#ifndef DEFAULT_FILESYSTEM
    #error "Unexpected value of DEFAULT_FILESYSTEM"
#endif

#if DEFAULT_FILESYSTEM == 1    /* USES PHYSFS */
    #include "backends/physfs/physfsfilesystem.h"

    #define __DEFAULT_FILESYSTEM  SunLight :: FileSystem :: PhysFs :: PhysFsFileSystem
#else
    #error "Unknown value of DEFAULT_FILESYSTEM"
#endif


namespace SunLight {
    namespace FileSystem  {

        static IFileSystem  *s_pOverrideFileSystem = nullptr;

        /**
         * @brief Get the single @see IFileSystem instance for the backend
         * selected at build time, unless overridden by @see
         * SetFileSystem() (tests only).
         */
        IFileSystem& FileSystemFactory :: GetFileSystem( void )  {

            if( s_pOverrideFileSystem )
                return *s_pOverrideFileSystem;

            static __DEFAULT_FILESYSTEM fileSystem;

            return fileSystem;
        }

        /**
         * @brief Override the filesystem returned by @see GetFileSystem().
         */
        void FileSystemFactory :: SetFileSystem( IFileSystem *pOverride )  {

            s_pOverrideFileSystem = pOverride;
        }
    }
}
