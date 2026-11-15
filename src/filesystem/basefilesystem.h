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

#ifndef __BASEFILESYSTEM_H__
#define __BASEFILESYSTEM_H__

#include "filesystem/ifilesystem.h"
#include <string>
#include <vector>


namespace SunLight  {
    namespace FileSystem  {

        /**
         * @brief What every IFileSystem backend shares (I13 of the master plan; the behaviour is
         * I12's): the path schemes (FilePath::SplitPath), host-path reads and writes with the
         * standard library, the host-path switch, and the read and write filters - once, so every
         * backend behaves the same and a filter cannot be forgotten. A backend derives from this
         * and implements Init, Shutdown, Mount, SetWriteDirectory and the virtual-path part (the
         * protected Virtual* methods, given a path without its scheme, with no filter applied).
         */
        class BaseFileSystem : public IFileSystem  {

            ReadFilterCallback   m_ReadFilter;               // empty (unset) by default
            WriteFilterCallback  m_WriteFilter;              // empty (unset) by default
            bool                 m_bHostPathEnabled = true;

            public:

            void SetReadFilter( ReadFilterCallback callback ) override;
            void SetWriteFilter( WriteFilterCallback callback ) override;
            void SetHostPathEnabled( bool bEnabled ) override;
            bool IsHostPathEnabled( void ) const override;
            bool Exists( const std :: string& strPath ) override;
            bool ReadFile( const std :: string& strPath, std :: vector<unsigned char>& outData ) override;
            bool WriteFile( const std :: string& strPath, const std :: vector<unsigned char>& data ) override;
            bool RemoveFile( const std :: string& strPath ) override;

            protected:

            virtual bool VirtualExists( const std :: string& strVirtualPath ) = 0;
            virtual bool VirtualReadFile( const std :: string& strVirtualPath, std :: vector<unsigned char>& outData ) = 0;
            virtual bool VirtualWriteFile( const std :: string& strVirtualPath, const std :: vector<unsigned char>& data ) = 0;
            virtual bool VirtualRemoveFile( const std :: string& strVirtualPath ) = 0;

            // I14 of the master plan: what a backend whose write directory is a real folder uses, as
            // WriteFile and RemoveFile do for host paths. WriteRealFile is the atomic write (a temporary
            // file beside the target, flushed to the disk, then renamed over it); RemoveRealFile removes
            // a file, never a folder; RealWritePath maps a virtual path to the file inside a real write
            // directory, refusing what PhysFS refuses (".", "..", ':', '\') so it never leaves the folder.
            static bool WriteRealFile( const std :: string& strRealPath, const std :: vector<unsigned char>& data );
            static bool RemoveRealFile( const std :: string& strRealPath );
            static bool RealWritePath( const std :: string& strWriteDir, const std :: string& strVirtualPath, std :: string& strRealPath );
        };
    }
}

#endif /* __BASEFILESYSTEM_H__ */
