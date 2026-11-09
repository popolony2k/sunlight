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

#ifndef __WORLDFILE_H__
#define __WORLDFILE_H__

#include <string>
#include <vector>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief One map a Tiled .world file places (I19f of the master plan): its path, already
         * resolved against the .world file's folder (FilePath::JoinPath, so a path scheme carries
         * over), and the top-left of its pixel box in the world's pixels.
         */
        struct stWorldFileMap  {
            std :: string  strPath;
            int            nX;
            int            nY;
        };

        /**
         * @brief Reads a Tiled .world file (I19f of the master plan) - a JSON file, which libtmx does
         * not read - into the list of maps it places. Only that: it loads no map and touches no
         * renderer, so World::LoadWorld (TileMapRenderer::LoadWorldFile) does the loading, all or
         * nothing. The JSON library stays inside worldfile.cpp: no sunlight header includes it.
         */
        class WorldFile  {

            public:

            static bool Read( const std :: string& strWorldFile, std :: vector<stWorldFileMap>& maps, std :: string& strError );
        };
    }
}

#endif /* __WORLDFILE_H__ */
