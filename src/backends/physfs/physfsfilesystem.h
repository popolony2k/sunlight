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

#ifndef __PHYSFSFILESYSTEM_H__
#define __PHYSFSFILESYSTEM_H__

#include "filesystem/ifilesystem.h"


namespace SunLight  {
    namespace FileSystem  {
        namespace PhysFs  {

            /**
             * @brief PhysFS-backed @see IFileSystem implementation. PhysFS
             * itself already provides the "loose directory or real
             * archive, mounted, read identically" abstraction this
             * interface wants - this class is a thin adapter translating
             * SunLight's own plain types at the boundary, nothing more.
             */
            class PhysFsFileSystem : public IFileSystem  {

                bool                m_bInitialized = false;
                bool                m_bAnyMounted = false;
                bool                m_bDefaultMountEnsured = false;
                ReadFilterCallback  m_ReadFilter;  // empty (unset) by default

                /**
                 * @brief Runs at most once, lazily, from @see Exists / @see
                 * ReadFile - see those methods' own doc comments and this
                 * method's definition for the full "why" (default fallback
                 * mount so a consumer that never calls @see Mount still
                 * gets plain OS-directory access, exactly as if this
                 * filesystem abstraction didn't exist).
                 */
                void EnsureReady( void );

                public:

                bool Init( const char *szArgv0 ) override;
                void Shutdown( void ) override;

                bool Mount( const std :: string &strRealPath, const std :: string &strMountPoint, bool bAppendToPath ) override;
                bool Exists( const std :: string &strVirtualPath ) override;
                bool ReadFile( const std :: string &strVirtualPath, std :: vector<unsigned char> &outData ) override;
                void SetReadFilter( ReadFilterCallback callback ) override;
            };
        }
    }
}

#endif  /* __PHYSFSFILESYSTEM_H__ */
