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

#include "renderer/world/worldfile.h"
#include "filesystem/filesystemfactory.h"
#include "filesystem/filepath.h"
#include <nlohmann/json.hpp>

#define __WORLD_KEY_MAPS        "maps"          // the explicit list of maps
#define __WORLD_KEY_PATTERNS    "patterns"      // maps found by a regular expression over the folder
#define __WORLD_KEY_FILE_NAME   "fileName"      // a map's path, relative to the .world file
#define __WORLD_KEY_X           "x"             // its left edge, in the world's pixels
#define __WORLD_KEY_Y           "y"             // its top edge

namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Read a Tiled .world file into the maps it places (I19f of the master plan). Read
         * through the FileSystem (mounts, archives, the read filter, path schemes), parsed as JSON
         * without exceptions, and checked: an object with a "maps" array whose every entry has a
         * string "fileName" and integer "x" and "y" - Tiled's own format (its manual: "All values are
         * in pixels"). Each map's "width"/"height" are not read: sunlight sizes a map itself, by the
         * same rule Tiled's editor does (its renderer's box). "onlyShowAdjacentMaps" is not read:
         * every map is loaded. A non-empty "patterns" array is refused - placing maps by a regular
         * expression over the folder needs a directory listing IFileSystem does not offer - rather
         * than silently loading only part of the world.
         * @param strWorldFile The .world file, a FileSystem path;
         * @param maps Receives the maps, in the file's order, their paths resolved against the .world
         * file's folder;
         * @param strError Receives why, when it fails;
         * @return false when the file cannot be read, is not such JSON, or uses patterns;
         */
        bool WorldFile :: Read( const std :: string& strWorldFile, std :: vector<stWorldFileMap>& maps, std :: string& strError )  {

            std :: vector<unsigned char>  data;

            maps.clear();

            if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strWorldFile, data ) )  {
                strError = "could not be read";
                return false;
            }

            nlohmann :: json  world = nlohmann :: json :: parse( data.begin(), data.end(), nullptr, false );

            if( world.is_discarded() || !world.is_object() )  {
                strError = "is not a JSON object";
                return false;
            }

            if( world.contains( __WORLD_KEY_PATTERNS ) && world[__WORLD_KEY_PATTERNS].is_array() && !world[__WORLD_KEY_PATTERNS].empty() )  {
                strError = "places maps by \"patterns\", which is not supported (list them in \"maps\")";
                return false;
            }

            if( !world.contains( __WORLD_KEY_MAPS ) || !world[__WORLD_KEY_MAPS].is_array() )  {
                strError = "has no \"maps\" array";
                return false;
            }

            std :: string  strFolder = SunLight :: FileSystem :: FilePath :: ParentPath( strWorldFile );

            for( const nlohmann :: json& entry : world[__WORLD_KEY_MAPS] )  {
                if( !entry.is_object() ||
                    !entry.contains( __WORLD_KEY_FILE_NAME ) || !entry[__WORLD_KEY_FILE_NAME].is_string() ||
                    !entry.contains( __WORLD_KEY_X ) || !entry[__WORLD_KEY_X].is_number_integer() ||
                    !entry.contains( __WORLD_KEY_Y ) || !entry[__WORLD_KEY_Y].is_number_integer() )  {
                    maps.clear();
                    strError = "has a map without a string \"fileName\" and integer \"x\" and \"y\"";
                    return false;
                }

                maps.push_back( stWorldFileMap { SunLight :: FileSystem :: FilePath :: JoinPath( strFolder, entry[__WORLD_KEY_FILE_NAME].get<std :: string>() ),
                                                 entry[__WORLD_KEY_X].get<int>(),
                                                 entry[__WORLD_KEY_Y].get<int>() } );
            }

            return true;
        }
    }
}
