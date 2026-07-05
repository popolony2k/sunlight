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

#include "texturecanvas.h"
#include <cstring>
#include "engines/enginefactory.h"
#include "base/primitives.h"


namespace SunLight {
    namespace Canvas  {
        /**
         * Constructor. Initialize all class data.
         */
        TextureCanvas :: TextureCanvas( void )  {

            m_nTileSize        = 0;
            m_nCenterTileIndex = 0;
            m_AnimationMode    = TEXTURE_ANIMATION_MODE_MANUAL;
            m_hTexture         = nullptr;
            m_nTextureWidth    = 0;
            m_nTextureHeight   = 0;
            SetColor( WHITE_COLOR );
            Reset();
            m_bIsResetting = false;
        }

        /**
         * Destructor. Finalize all class data.
         */
        TextureCanvas :: ~TextureCanvas( void )  {

            Unload();
        }

        /**
         * Load a texture specified by parameter.
         * @param strTextureFile The texture filename to load;
         */
        bool TextureCanvas :: Load( std :: string strTextureFile )  {

            SunLight :: TileMap :: stDimension2D& dimension = GetDimension2D();

            m_hTexture = SunLight :: Engines :: EngineFactory :: GetEngine().LoadTexture( strTextureFile.c_str(),
                                                                                          m_nTextureWidth,
                                                                                          m_nTextureHeight );

            if( m_hTexture != nullptr )  {
                if( ( dimension.size.nHeight == 0 ) &&
                    ( dimension.size.nHeight == 0 )  ) {
                    dimension.size.nHeight = m_nTextureHeight;
                    dimension.size.nWidth  = m_nTextureWidth;
                }

                return true;
            }

            return false;
        }

        /**
         * Unload a previously loaded texture  by @link Load method.
         */
        bool TextureCanvas :: Unload( void )  {

            if( m_hTexture != nullptr )  {
                SunLight :: Engines :: EngineFactory :: GetEngine().UnloadTexture( m_hTexture );
                m_hTexture = nullptr;
                return true;
            }

            return false;
        }

        /**
         * Reset canvas object.
         */
        void TextureCanvas :: Reset( void )  {

            m_nActiveTileIndex = m_nCenterTileIndex;
            m_nCurrentTile     = ( m_nTileSize * m_nActiveTileIndex );
            m_bAnimationRight  = true;
            m_bIsResetting     = true;
        }

        /**
         * Set texture frame split size. Canvas can be used as sequence of animated tiles
         * in conjunction with sprites. In this case user can specify the tile split size
         * in X axis.
         * Each update will increase the step specified by this method.
         * @param nTileSize The tile size to be used to split this texture.
         */
        void TextureCanvas :: SetTileSize( unsigned int nTileSize )  {

            m_nTileSize = nTileSize;
        }
        /**
         * Get texture frame split size.
         */
        unsigned int TextureCanvas :: GetTileSize( void ) {

            return m_nTileSize;
        }

        /**
         * @brief Set the Active Tile object index.
         * 
         * @param nActiveTileIndex The current tile object index to be activated;
         */
        void TextureCanvas :: SetActiveTileIndex( unsigned int nActiveTileIndex )  {

            if( m_nTextureWidth >= ( int ) ( m_nTileSize * nActiveTileIndex ) )
                m_nActiveTileIndex = nActiveTileIndex;
        }

        /**
         * @brief Get the Active Tile object index; 
         * 
         * @return unsigned int The current active index; 
         */
        unsigned int TextureCanvas :: GetActiveTileIndex( void )  {

            return m_nActiveTileIndex;
        }

        /**
         * @brief Set the center Tile index.
         * 
         * @param nTileIndex The centered tile object index;
         */
        void TextureCanvas :: SetCenterTileIndex( unsigned int nTileIndex )  {

            if( m_nTextureWidth >= ( int ) ( m_nTileSize * nTileIndex ) )
                m_nCenterTileIndex = nTileIndex;
        }

        /**
         * @brief Get the center Tile Index; 
         * 
         * @return unsigned int The current active index; 
         */
        unsigned int TextureCanvas :: GetCenterTileIndex( void )  {

            return m_nCenterTileIndex;
        }

        /**
         * @brief Set Tile Animation mode for object; 
         * 
         * @param mode The new animation mode; 
         */
        void TextureCanvas :: SetAnimationMode( AnimationMode mode )  {
            
            m_AnimationMode = mode;
        }

        /**
         * @brief Get the object Animation Mode;
         * 
         * @return TextureCanvasAnimationMode The current Object animation mode set;
         */
        AnimationMode TextureCanvas :: GetAnimationMode( void )  {

            return m_AnimationMode;
        }

        /**
         * Implements the draw update method used to draw a sprite
         * object;
         */
        void TextureCanvas :: Update( void )  {

            if( GetVisible() )  {
                SunLight :: Base :: Viewport&       vp   = GetViewport();
                SunLight :: TileMap :: stDimension2D& vpDm = vp.GetDimension2D();
                SunLight :: TileMap :: stDimension2D& dm   = GetDimension2D();
                SunLight :: TileMap :: stDimension2D  clip;

                if( vp.GetClippedRect( dm, clip ) ) {

                    /*
                    * All cut operations are calculated considering the
                    * base texture that is non-scaled.
                    * GetClippedArea fClipW, fClipH results used in cut operation
                    * have a zoom factor applied to its results, so "removing" this
                    * "noise" is needed by dividing it's results by zoom factor.
                    * This is needed because when texture reaches canvas boundaries
                    * the texture is cut in a wrong position.
                    */
                    SunLight :: Base :: stColor& color       = GetColor();
                    float                        fZoomFactor = vp.GetZoomProperties().fZoomFactor;
                    int                          nClipX      = ( clip.pos.x == vpDm.pos.x ?
                                                                 ( int ) std :: abs( ( clip.size.nWidth / fZoomFactor ) -
                                                                 dm.size.nWidth ) : 0 );
                    int                          nClipY      = ( clip.pos.y == vpDm.pos.y ?
                                                                 ( int ) std :: abs( ( clip.size.nHeight / fZoomFactor ) -
                                                                 dm.size.nHeight ) : 0 );

                    switch( m_AnimationMode )  {
                        case TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR :
                            m_nCurrentTile = ( m_nCurrentTile >= m_nTextureWidth ? 0 :
                                            m_nCurrentTile + m_nTileSize );
                            break;
                        case TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT :
                            if( m_nTileSize != 0 )  {
                                if( m_bAnimationRight )  {
                                    if( m_nActiveTileIndex < ( m_nTextureWidth / m_nTileSize ) )  {
                                        m_nCurrentTile = ( m_nTileSize * m_nActiveTileIndex );
                                        m_nActiveTileIndex++;
                                    }
                                    else  {
                                        if( m_nActiveTileIndex > 0 )  {
                                            m_bAnimationRight = false;
                                            m_nActiveTileIndex--;
                                            m_nCurrentTile = ( m_nTileSize * m_nActiveTileIndex );
                                        }
                                    }
                                }
                                else  {
                                    if( m_nActiveTileIndex > 0 )  {
                                        m_nActiveTileIndex--;
                                        m_nCurrentTile = ( m_nTileSize * m_nActiveTileIndex );
                                    }
                                    else  {
                                        m_nCurrentTile = 0;
                                        m_bAnimationRight = true;
                                    }
                                }
                            }
                            break;
                        case TEXTURE_ANIMATION_MODE_MANUAL :
                            m_nCurrentTile = ( m_nTileSize * m_nActiveTileIndex );
                        break;
                        case TEXTURE_ANIMATION_MODE_ANIMATE_RIGHT:
                            if( m_nTileSize != 0 )  {
                                if( m_nActiveTileIndex < ( m_nTextureWidth / m_nTileSize ) )  {
                                    m_nCurrentTile = ( m_nTileSize * m_nActiveTileIndex );
                                    m_nActiveTileIndex++;
                                }
                            }
                            break;
                        case TEXTURE_ANIMATION_MODE_ANIMATE_LEFT :
                            if( ( m_nTileSize != 0 ) && ! m_bIsResetting )  {  
                                if( m_nActiveTileIndex > 0 )  {
                                    m_nActiveTileIndex--;
                                    m_nCurrentTile = ( m_nTileSize * m_nActiveTileIndex );
                                }
                                else
                                    m_nCurrentTile = 0;
                            }
                            break;
                        case TEXTURE_ANIMATION_MODE_ANIMATE_CENTER :
                            if( m_nTileSize != 0 )  {
                                int nTileIndex = m_nCenterTileIndex;
                                
                                if( m_nActiveTileIndex > m_nCenterTileIndex )  {
                                    nTileIndex = --m_nActiveTileIndex;
                                }
                                else  {
                                    if( m_nActiveTileIndex < m_nCenterTileIndex )  {
                                        nTileIndex = ++m_nActiveTileIndex;
                                    }
                                }

                                m_nCurrentTile = ( m_nTileSize * nTileIndex );
                            }
                            break;
                    }

                    SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureTiled(
                                                          m_hTexture,
                                                          SunLight :: Base :: stRectangle { ( float ) m_nCurrentTile + nClipX,
                                                                      ( float ) nClipY,
                                                                      ( float ) ( m_nTileSize > 0 ?
                                                                                  m_nTileSize :
                                                                                  m_nTextureWidth ),
                                                                      ( float ) m_nTextureHeight },
                                                          SunLight :: Base :: stRectangle { ( float ) clip.pos.x,
                                                                      ( float ) clip.pos.y,
                                                                      ( float ) clip.size.nWidth,
                                                                      ( float ) clip.size.nHeight },
                                                          SunLight :: Base :: stVector2D { 0.0, 0.0 },
                                                          0.0, // TODO: Rotation
                                                          fZoomFactor,
                                                          color );
                }
            }

            m_bIsResetting = false;
        }
    }
}
