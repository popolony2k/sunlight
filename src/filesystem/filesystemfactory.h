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

#ifndef __FILESYSTEMFACTORY_H__
#define __FILESYSTEMFACTORY_H__

#include "filesystem/ifilesystem.h"


namespace SunLight {
    namespace FileSystem  {

        /**
         * @brief Resource-filesystem access point used to retrieve the
         * @see IFileSystem implementation selected at build time
         * (DEFAULT_FILESYSTEM). This is the only place in the codebase
         * that needs to know which concrete backend is compiled in - same
         * shape as Engines::EngineFactory.
         */
        class FileSystemFactory  {

            public:

            static IFileSystem& GetFileSystem( void );

            /**
             * @brief Override the filesystem returned by @see
             * GetFileSystem() - for tests only, to substitute a mock
             * @see IFileSystem without touching the real one. Pass
             * nullptr to restore the default, build-time backend. The
             * override is not owned by the factory - the caller keeps it
             * alive for as long as it's set.
             *
             * @param pOverride The replacement filesystem, or nullptr to
             * reset back to the default backend;
             */
            static void SetFileSystem( IFileSystem *pOverride );
        };
    }
}

#endif  /* __FILESYSTEMFACTORY_H__ */
