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

#ifndef __RAYLIBENGINE_H__
#define __RAYLIBENGINE_H__

#include "raylib.h"

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief Raylib engine provider implementation.
             * All specific raylib calls are implemented here. 
             */
            class RaylibEngine  {

                public:

                static void DrawTextureTiled( Texture2D texture,
                                              Rectangle source, 
                                              Rectangle dest, 
                                              Vector2 origin,
                                              float rotation, 
                                              float scale, 
                                              Color tint);
                static void SetPixel( int nPosX, int nPosY, Color color );
            };
        }
    }
}
#endif  /* __RAYLIBENGINE_H__ */


