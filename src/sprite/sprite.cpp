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

#include "sprite.h"


namespace SunLight {
    namespace Sprite  {
        /**
         * Constructor. Initialize all class data.
         */
        Sprite :: Sprite( void ) {

            m_itActiveSequence = m_Sequences.begin();
            m_bIsValidActiveSequence = ( m_itActiveSequence != m_Sequences.end() );
            m_bFrameHeld = false;
        }

        /**
         * Destructor. Deliberately does NOT unload the canvases added with
         * @see AddTextureSequence: the sprite only holds raw pointers to
         * canvases the CALLER owns, and there is no guarantee they are still
         * alive here - a canvas declared after its sprite (or a member
         * declared after it) is destroyed BEFORE it, and walking to it from
         * this destructor then reads a dead object (AddressSanitizer:
         * stack-use-after-scope in the old destructor, on exactly that
         * declaration order). A canvas frees its own texture when it is
         * destroyed (~TextureCanvas), so nothing is leaked; what changes is
         * only that a canvas which outlives its sprite keeps its texture
         * until then, instead of having it pulled out from under it. An
         * explicit @see Unload still unloads them all, for a caller that
         * wants that.
         *
         * It still empties its OWN sequence list (only its own data): the
         * renderer keeps a raw pointer to every sprite registered with
         * AddSprite and calls Unload() on it again at Stop(), and a sprite
         * destroyed while still registered (a caller should RemoveSprite it
         * first) used to be left looking empty to that second call because
         * this destructor ran Unload(). Without this clear() that stale
         * second call reads a destroyed, non-empty list and crashes
         * (observed while making this change), so the destructor keeps that
         * case exactly as benign as it was.
         */
        Sprite :: ~Sprite( void )  {

            // Tell the parent (the renderer, once registered with AddSprite) this sprite is going away, so
            // that it stops holding a pointer to it - see SetParent.
            if( GetParent() )
                GetParent() -> ChildRemoved( this );

            m_Sequences.clear();
        }

        /**
         * Change this sprite's parent. A parent may keep raw pointers to its
         * children (TileMapRenderer keeps the sprites registered with
         * AddSprite, to advance and draw them every frame), so the parent
         * being LEFT is told (@see BaseCanvas::ChildRemoved) and forgets
         * this sprite. The canvases that were following the old parent are
         * moved to the new one - AddTextureSequence parents a canvas to the
         * sprite's parent if it has one, else to the sprite itself, so those
         * are the canvases that would otherwise keep a pointer to a parent
         * that no longer knows about them (and may be destroyed): they get
         * the new parent, or this sprite when there is none. A canvas
         * parented to the sprite itself is left alone.
         * @param pParent The new parent, or nullptr for none;
         */
        void Sprite :: SetParent( SunLight :: Canvas :: BaseCanvas *pParent )  {

            SunLight :: Canvas :: BaseCanvas  *pOldParent = GetParent();

            if( pOldParent && ( pOldParent != pParent ) )
                pOldParent -> ChildRemoved( this );

            Canvas :: SetParent( pParent );

            if( pOldParent && ( pOldParent != pParent ) )  {
                for( auto &pair : m_Sequences )  {
                    pair.second -> ForEachTexture( [this, pOldParent, pParent]( SunLight :: Canvas :: TextureCanvas *pTexture )  {
                        if( pTexture -> GetParent() == pOldParent )
                            pTexture -> SetParent( pParent ? pParent : this );
                    } );
                }
            }
        }

        /**
         * Loads and add a texture to the internal texture map object.
         * @param pTexture Pointer to a @link TextureCanvas to add;
         * @param nDetalyMilli Time in millisecond to be used in texture
         * animation sequence;
         */
        void Sprite :: AddTextureSequence( int nSequence,
                                           SunLight :: Canvas :: TextureCanvas *pTexture,
                                           int64_t nDelayMilli ) {

            TextureSequenceList :: iterator       itItem     = m_Sequences.find( nSequence );
            SunLight :: TileMap :: stDimension2D& spritePos  = GetDimension2D();
            SunLight :: TileMap :: stDimension2D  texturePos = pTexture -> GetDimension2D();
            BaseCanvas                            *pParent   = GetParent();

            if( !pParent )
                pParent = this;

            if( ( spritePos.size.nWidth == 0 ) &&
                ( spritePos.size.nHeight == 0 ) )  {
                spritePos.size.nWidth  = texturePos.size.nWidth;
                spritePos.size.nHeight = texturePos.size.nHeight;
            }

            if( ( spritePos.pos.x == 0 ) && ( spritePos.pos.y == 0 ) )  {
                spritePos.pos.x = texturePos.pos.x;
                spritePos.pos.y = texturePos.pos.y;
            }

            pTexture -> SetVisible( GetVisible() );
            pTexture -> SetParent( pParent );
            pTexture -> SetDimension2DPtr( &spritePos );

            if( itItem == m_Sequences.end() )  {
                std :: unique_ptr<TextureMap>   pTextureMap = std :: make_unique<TextureMap>();

                pTextureMap -> AddTexture( pTexture, nDelayMilli );
                m_Sequences.insert( std :: make_pair( nSequence, std :: move( pTextureMap ) ) );
            }
            else  {
                itItem -> second -> AddTexture( pTexture, nDelayMilli );
            }
        }

        /**
         * Set the active sprite sequence animation.
         * @param nSequence The sequence id to activate;
         */
        bool Sprite :: SetActiveTextureSequence( int nSequence )  {

            m_itActiveSequence = m_Sequences.find( nSequence );
            m_bIsValidActiveSequence = ( m_itActiveSequence != m_Sequences.end() );

            /*
            * Reset current animation sequence on selected
            * texture.
            */
            if( m_bIsValidActiveSequence )  {
                m_itActiveSequence -> second -> GetTextureData().pTexture -> Reset();
            }

            return m_bIsValidActiveSequence;
        }

        /**
         * @brief Get the active sequence object iterator;
         * 
         * @return the active sequence number or -1 if is invalid;
         */
        int Sprite :: GetActiveTextureSequence( void )  {

            return ( m_bIsValidActiveSequence ? m_itActiveSequence -> first : -1 );
        }

        /**
         * @brief Get the active texture sequence object pointer;
         * 
         * @return the active sequence number or NULL if active texture is invalid;
         */
        SunLight :: Canvas :: TextureCanvas* Sprite :: GetActiveTexture( void )  {

            return ( m_bIsValidActiveSequence ? m_itActiveSequence -> second -> GetTextureData().pTexture : NULL );
        }

        /**
        * Move the sprite based on x,y steps passed a parameter.
        * @param step Reference to a @link stCoordinate2D containing
        * the x,y move steps;
        */
        void Sprite :: Move( SunLight :: TileMap :: stCoordinate2D& step )  {

            SunLight :: TileMap :: stDimension2D& dimension = GetDimension2D();

            dimension.pos.x+=step.x;
            dimension.pos.y+=step.y;
        }

        /**
        * Set the visible status of a drawing entity.
        * @param bVisible The new visible status;
        */
        void Sprite :: SetVisible( bool bVisible )  {

            TextureSequenceList :: iterator itItem;

            Canvas :: SetVisible( bVisible );

            for( itItem = m_Sequences.begin(); itItem != m_Sequences.end(); itItem++ )  {
                if( itItem -> second -> First() )  {
                    do  {
                        itItem -> second -> GetTextureData().pTexture -> SetVisible( bVisible );
                    } while( itItem -> second -> Next( false ) );

                    itItem -> second -> First();
                }
            }
        }

        /**
        * The STATE half of a sprite's frame: let the active texture map pick
        * its frame for the current time, and advance that texture's own
        * animation one step - without drawing. Meant to run once per frame;
        * @see Draw may then run any number of times (e.g. once per view)
        * without stepping anything a second time.
        *
        * When the texture map did not step to a new frame this frame (its
        * current one is not due to change yet) the frame is HELD: the canvas
        * is advanced with tile size 0, which stops its tile animation from
        * stepping, and @see Draw applies the same trick to show the held
        * frame.
        */
        void Sprite :: Advance( void )  {

            m_bFrameHeld = false;

            if( GetVisible() && m_bIsValidActiveSequence )  {
                m_bFrameHeld = !m_itActiveSequence -> second -> Next();

                SunLight :: Canvas :: TextureCanvas *pTextureCanvas = m_itActiveSequence -> second -> GetTextureData().pTexture;
                unsigned int  nTileSize;

                if( m_bFrameHeld )  {
                    nTileSize = pTextureCanvas -> GetTileSize();
                    pTextureCanvas -> SetTileSize( 0 );
                }

                pTextureCanvas -> Advance();

                if( m_bFrameHeld )
                    pTextureCanvas -> SetTileSize( nTileSize );
            }
        }

        /**
        * The DRAWING half of a sprite's frame: draw the active texture in
        * its current state. Changes no animation state (it never asks the
        * texture map for a new frame), so it is safe to call repeatedly for
        * one frame. Draws the held frame the same way Advance() left it.
        */
        void Sprite :: Draw( void )  {

            if( GetVisible() && m_bIsValidActiveSequence )  {
                SunLight :: Canvas :: TextureCanvas *pTextureCanvas = m_itActiveSequence -> second -> GetTextureData().pTexture;
                unsigned int  nTileSize;

                if( m_bFrameHeld )  {
                    nTileSize = pTextureCanvas -> GetTileSize();
                    pTextureCanvas -> SetTileSize( 0 );
                }

                pTextureCanvas -> Draw();

                if( m_bFrameHeld )
                    pTextureCanvas -> SetTileSize( nTileSize );
            }
        }

        /**
        * Implements the draw update method used to draw a sprite
        * object: @see Advance followed by @see Draw.
        */
        void Sprite :: Update( void )  {

            Advance();
            Draw();
        }

        /**
        * Unload all loaded sprites on this object, and let go of them: the
        * sequence list is emptied, so the canvases that were following this
        * sprite or its parent (AddTextureSequence parents a canvas to one or
        * the other) are unparented here - after this call nothing can reach
        * them through the sprite any more, and one that still pointed at a
        * parent that is about to be destroyed (the renderer, which unloads
        * every sprite registered with it when it stops or is destroyed)
        * would be left dangling.
        */
        void Sprite :: Unload( void )  {

            if( m_Sequences.size() > 0 )  {
                for( auto& pair : m_Sequences )  {
                    if( pair.second -> First() )  {
                        do  {
                            SunLight :: Canvas :: TextureCanvas  *pTexture = pair.second -> GetTextureData().pTexture;

                            pTexture -> Unload();

                            if( pTexture -> GetParent() && ( ( pTexture -> GetParent() == this ) || ( pTexture -> GetParent() == GetParent() ) ) )
                                pTexture -> SetParent( nullptr );
                        } while( pair.second -> Next( false ) );
                    }
                }

                m_Sequences.clear();
            }
        }
    }
}
