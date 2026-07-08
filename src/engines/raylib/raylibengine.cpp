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
             * This routines is planned to be removed after raylib 4.2.0 and was moved to a samples project directory
             * found at https://github.com/raysan5/raylib/blob/master/examples/textures/textures_draw_tiled.c
             *
             * @param hTexture
             * @param sourceIn
             * @param destIn
             * @param originIn
             * @param rotation
             * @param scale
             * @param tintIn
             */
            void RaylibEngine :: DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                                                   SunLight :: Base :: stRectangle sourceIn,
                                                   SunLight :: Base :: stRectangle destIn,
                                                   SunLight :: Base :: stVector2D originIn,
                                                   float rotation,
                                                   float scale,
                                                   SunLight :: Base :: stColor tintIn )  {

                Texture2D  texture = *reinterpret_cast<Texture2D*>( hTexture );
                Rectangle  source  { sourceIn.x, sourceIn.y, sourceIn.width, sourceIn.height };
                Rectangle  dest    { destIn.x, destIn.y, destIn.width, destIn.height };
                Vector2    origin  { originIn.x, originIn.y };
                Color      tint    { tintIn.nRed, tintIn.nGreen, tintIn.nBlue, tintIn.nAlpha };

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

            /**
             * Draw  pixel according the specified position.
             * @param nPosX The X coordinate to plot pixel;
             * @param nPosY The Y coordinate to plot pixel;
             * @param color Color of pixel;
             */
            void RaylibEngine :: SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color )  {
                ::DrawPixel( nPosX, nPosY, Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }

            /**
             * @brief Load a texture from disk.
             * @param szFileName Texture file name to load;
             * @param nWidth Output parameter receiving the loaded texture width;
             * @param nHeight Output parameter receiving the loaded texture height;
             * @return An opaque handle to the loaded texture, or nullptr on failure;
             */
            SunLight :: Base :: TextureHandle RaylibEngine :: LoadTexture( const char *szFileName,
                                                                            int& nWidth,
                                                                            int& nHeight )  {

                Texture2D *pTexture = new Texture2D;

                *pTexture = ::LoadTexture( szFileName );

                if( pTexture -> id <= 0 )  {
                    delete pTexture;
                    nWidth  = 0;
                    nHeight = 0;

                    return nullptr;
                }

                nWidth  = pTexture -> width;
                nHeight = pTexture -> height;

                return pTexture;
            }

            /**
             * @brief Unload a texture previously loaded by @see LoadTexture.
             * @param hTexture The texture handle to unload;
             */
            void RaylibEngine :: UnloadTexture( SunLight :: Base :: TextureHandle hTexture )  {

                Texture2D *pTexture = reinterpret_cast<Texture2D*>( hTexture );

                ::UnloadTexture( *pTexture );
                delete pTexture;
            }

            /**
             * @brief Draw a texture at the specified position.
             * @param hTexture The texture handle to draw;
             * @param nPosX X coordinate to draw texture;
             * @param nPosY Y coordinate to draw texture;
             * @param tint Color tint applied to texture;
             */
            void RaylibEngine :: DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                              int nPosX,
                                              int nPosY,
                                              SunLight :: Base :: stColor tint )  {

                Texture2D *pTexture = reinterpret_cast<Texture2D*>( hTexture );

                ::DrawTexture( *pTexture, nPosX, nPosY, Color{ tint.nRed, tint.nGreen, tint.nBlue, tint.nAlpha } );
            }

            /**
             * @brief Return the directory the running executable lives in.
             * Copies raylib's own internal static buffer into an owned
             * std::string immediately, so the result stays valid regardless
             * of subsequent raylib calls.
             * @return The application's own directory, or an empty string
             * if it could not be determined;
             */
            std :: string RaylibEngine :: GetApplicationDirectory( void )  {

                const char *szDirectory = ::GetApplicationDirectory();

                return ( szDirectory ? std :: string( szDirectory ) : std :: string() );
            }
        }
    }
}
