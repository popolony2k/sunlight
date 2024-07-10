/*
 * raylibengine.h
 *
 *  Created on: Nov 29, 2022
 *      Author: popolony2k
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
            };
        }
    }
}
#endif  /* __RAYLIBENGINE_H__ */


