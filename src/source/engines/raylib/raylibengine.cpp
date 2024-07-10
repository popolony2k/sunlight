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

#include "engines/raylib/raylibengine.h"

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief Draw part of a texture (defined by a rectangle) with rotation and scale tiled into dest 
             * This routines is planned to be removed after rayli 4.2.0 and was moved to a samples project directory
             * found at https://github.com/raysan5/raylib/blob/master/examples/textures/textures_draw_tiled.c
             * 
             * @param texture 
             * @param source 
             * @param dest 
             * @param origin 
             * @param rotation 
             * @param scale 
             * @param tint 
             */
            void RaylibEngine :: DrawTextureTiled( Texture2D texture, 
                                                   Rectangle source, 
                                                   Rectangle dest, 
                                                   Vector2 origin, 
                                                   float rotation, 
                                                   float scale, 
                                                   Color tint )  {
                                    
                if ((texture.id <= 0) || (scale <= 0.0f)) return;  // Wanna see a infinite loop?!...just delete this line!
                if ((source.width == 0) || (source.height == 0)) return;

                int tileWidth = (int)(source.width*scale), tileHeight = (int)(source.height*scale);
                if ((dest.width < tileWidth) && (dest.height < tileHeight))
                {
                    // Can fit only one tile
                    ::DrawTexturePro( texture, 
                                      Rectangle{ source.x, source.y, 
                                                 ((float)dest.width/tileWidth)*source.width, 
                                                 ((float)dest.height/tileHeight)*source.height },
                                      Rectangle { dest.x, 
                                                  dest.y, 
                                                  dest.width,
                                                  dest.height }, 
                                      origin, 
                                      rotation, 
                                      tint );
                }
                else if (dest.width <= tileWidth)
                {
                    // Tiled vertically (one column)
                    int dy = 0;
                    for (;dy+tileHeight < dest.height; dy += tileHeight)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     ((float)dest.width/tileWidth)*source.width, 
                                                     source.height }, 
                                          Rectangle{ dest.x, 
                                                     dest.y + dy, 
                                                     dest.width, 
                                                     (float)tileHeight }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }

                    // Fit last tile
                    if (dy < dest.height)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     ((float)dest.width/tileWidth)*source.width, 
                                                     ((float)(dest.height - dy)/tileHeight)*source.height },
                                          Rectangle{ dest.x, 
                                                     dest.y + dy, 
                                                     dest.width, 
                                                     dest.height - dy }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }
                }
                else if (dest.height <= tileHeight)
                {
                    // Tiled horizontally (one row)
                    int dx = 0;
                    for (;dx+tileWidth < dest.width; dx += tileWidth)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     source.width, 
                                                     ((float)dest.height/tileHeight)*source.height }, 
                                          Rectangle{ dest.x + dx, 
                                                     dest.y, 
                                                     (float)tileWidth, 
                                                     dest.height }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }

                    // Fit last tile
                    if (dx < dest.width)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     ((float)(dest.width - dx)/tileWidth)*source.width, 
                                                     ((float)dest.height/tileHeight)*source.height },
                                          Rectangle{ dest.x + dx, 
                                                     dest.y, 
                                                     dest.width - dx,
                                                     dest.height }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }
                }
                else
                {
                    // Tiled both horizontally and vertically (rows and columns)
                    int dx = 0;
                    for (;dx+tileWidth < dest.width; dx += tileWidth)
                    {
                        int dy = 0;
                        for (;dy+tileHeight < dest.height; dy += tileHeight)
                        {
                            ::DrawTexturePro( texture, 
                                              source, 
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         (float)tileWidth, 
                                                         (float)tileHeight }, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }

                        if (dy < dest.height)
                        {
                            ::DrawTexturePro( texture, 
                                              Rectangle{ source.x, 
                                                         source.y, 
                                                         source.width, 
                                                         ((float)(dest.height - dy)/tileHeight)*source.height },
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         (float)tileWidth, 
                                                         dest.height - dy }, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }
                    }

                    // Fit last column of tiles
                    if (dx < dest.width)
                    {
                        int dy = 0;
                        for (;dy+tileHeight < dest.height; dy += tileHeight)
                        {
                            ::DrawTexturePro( texture, 
                                              Rectangle{ source.x, 
                                                         source.y, 
                                                         ((float)(dest.width - dx)/tileWidth)*source.width, 
                                                         source.height },
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         dest.width - dx, 
                                                         (float)tileHeight}, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }

                        // Draw final tile in the bottom right corner
                        if (dy < dest.height)
                        {
                            ::DrawTexturePro( texture, 
                                              Rectangle{ source.x, 
                                                         source.y, 
                                                         ((float)(dest.width - dx)/tileWidth)*source.width, 
                                                         ((float)(dest.height - dy)/tileHeight)*source.height },
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         dest.width - dx, 
                                                         dest.height - dy }, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }
                    }
                }
            }
        }
    }
}