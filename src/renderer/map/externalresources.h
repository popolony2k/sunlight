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

#ifndef __EXTERNALRESOURCES_H__
#define __EXTERNALRESOURCES_H__

#include "tmx.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace SunLight  {
    namespace Renderer  {
        namespace ExternalResources  {

            /**
             * @brief One external tileset (.tsx) or object template (.tx) that a map names, as written in the
             * map or in another external file.
             */
            struct ExternalReference  {
                bool           bTemplate;      // an object template (.tx) rather than a tileset (.tsx)
                std :: string  strKey;         // the attribute text, as written
            };

            /**
             * Directory part of a virtual path, trailing '/' included ("" if it has none).
             */
            std :: string DirectoryOfVirtualPath( const std :: string &strPath );

            /**
             * strDir + strRelative as one virtual path: backslashes become '/', and "." and ".."
             * segments are collapsed LEXICALLY (the FileSystem rejects them). A ".." that would
             * climb above the root gives "" - not a legal virtual path.
             */
            std :: string JoinVirtualPath( const std :: string &strDir, const std :: string &strRelative );

            /**
             * Finds the external tilesets and templates that one XML document names, in order.
             */
            void ScanExternalReferences( const std :: string &strXml, std :: vector<ExternalReference> &references );

            /**
             * Reads one external file through the FileSystem and hands it to libtmx's Resource Manager under
             * the key the map used. Preloads what that file names first, then the file itself. Each file is
             * loaded once, and a key already provided is not provided again.
             */
            void PreloadExternalResource( tmx_resource_manager *pRcMgr,
                                          const ExternalReference &reference,
                                          const std :: string &strReferencingDir,
                                          std :: map<std :: string, std :: string> &loadedKeys,
                                          std :: set<std :: string> &visited );

            /**
             * The directory whose images the external file being preloaded names, or "" outside a preload.
             */
            std :: string ImageBase( void );
        }
    }
}
#endif  /* __EXTERNALRESOURCES_H__ */
