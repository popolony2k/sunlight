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

#include "renderer/sprite/spriteregistry.h"
#include <algorithm>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Open a layer's sprite list, empty, if it has none yet. A list once opened stays,
         * even emptied (Remove, Forget): only UnloadAll closes them. TileMapRenderer::AddSprite
         * opens the list before it checks the sprite and its collider, as it always did, so a
         * refused sprite still leaves its layer open - and Empty() (no list open at all) is what
         * decides whether a renderer with no map runs its sprite frame.
         * @param nLayerId The layer;
         */
        void SpriteRegistry :: OpenLayer( int nLayerId )  {

            m_Layers[nLayerId];
        }

        /**
         * @brief Register a sprite on a layer, after the sprites already there (the draw order
         * among equal z-orders, see UpdateLayer). Opens the layer's list if needed.
         * @param nLayerId The layer;
         * @param pSprite The sprite;
         * @return false if the sprite is already on that layer (nothing changes);
         */
        bool SpriteRegistry :: Add( int nLayerId, SunLight :: Sprite :: Sprite *pSprite )  {

            if( Contains( nLayerId, pSprite ) )
                return false;

            m_Layers[nLayerId].push_back( pSprite );

            return true;
        }

        /**
         * @brief Whether a sprite is registered on a layer.
         */
        bool SpriteRegistry :: Contains( int nLayerId, SunLight :: Sprite :: Sprite *pSprite ) const  {

            std :: map<int, SpriteList> :: const_iterator  itLayer = m_Layers.find( nLayerId );

            return ( itLayer != m_Layers.end() ) &&
                   ( std :: find( itLayer -> second.begin(), itLayer -> second.end(), pSprite ) != itLayer -> second.end() );
        }

        /**
         * @brief Take a sprite off one layer. It stays parented (see MarkParented): a removed
         * sprite keeps the renderer as its parent until it is destroyed or re-parented.
         * @return false if it was not on that layer;
         */
        bool SpriteRegistry :: Remove( int nLayerId, SunLight :: Sprite :: Sprite *pSprite )  {

            std :: map<int, SpriteList> :: iterator  itLayer = m_Layers.find( nLayerId );

            if( itLayer != m_Layers.end() )  {
                SpriteList :: iterator  itItem = std :: find( itLayer -> second.begin(), itLayer -> second.end(), pSprite );

                if( itItem != itLayer -> second.end() )  {
                    itLayer -> second.erase( itItem );
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Whether no layer's list is open (see OpenLayer) - not whether no sprite is registered.
         */
        bool SpriteRegistry :: Empty( void ) const  {

            return m_Layers.empty();
        }

        /**
         * @brief Every layer with an open list, in ascending id order: the only layer order a
         * renderer with no map has (TileMapRenderer::DrawPassContent).
         */
        std :: vector<int> SpriteRegistry :: LayerIds( void ) const  {

            std :: vector<int>  ids;

            for( const std :: pair<const int, SpriteList> &layer : m_Layers )
                ids.push_back( layer.first );

            return ids;
        }

        /**
         * @brief Record a sprite whose parent the renderer has become: one that must tell it when
         * it is destroyed or moves to another parent (Forget), on a layer or not any more.
         */
        void SpriteRegistry :: MarkParented( SunLight :: Sprite :: Sprite *pSprite )  {

            m_Parented.insert( pSprite );
        }

        /**
         * @brief A parented sprite is leaving (TileMapRenderer::ChildRemoved): take it off every
         * layer it is on and stop tracking it. A child that is not a parented sprite is ignored.
         * @param pChild The child leaving, as its BaseCanvas (compared as one, so a Sprite's
         * BaseCanvas sub-object matches whatever address the caller holds for it);
         * @param layerIds Receives the layers it was taken off, ascending (the renderer takes its
         * collider out of each);
         * @return The sprite it was, or nullptr if it was not a parented sprite;
         */
        SunLight :: Sprite :: Sprite* SpriteRegistry :: Forget( SunLight :: Canvas :: BaseCanvas *pChild, std :: vector<int> &layerIds )  {

            for( std :: set<SunLight :: Sprite :: Sprite*> :: iterator itSprite = m_Parented.begin(); itSprite != m_Parented.end(); itSprite++ )  {
                SunLight :: Sprite :: Sprite  *pSprite = *itSprite;

                if( static_cast<SunLight :: Canvas :: BaseCanvas*>( pSprite ) == pChild )  {
                    for( std :: pair<const int, SpriteList> &layer : m_Layers )  {
                        SpriteList :: iterator  itItem = std :: find( layer.second.begin(), layer.second.end(), pSprite );

                        if( itItem != layer.second.end() )  {
                            layerIds.push_back( layer.first );
                            layer.second.erase( itItem );
                        }
                    }

                    // A sprite destroyed mid-frame must not be advanced at the frame's end.
                    m_FrameAdvanced.erase( pSprite );
                    m_FramePending.erase( pSprite );
                    m_Parented.erase( itSprite );

                    return pSprite;
                }
            }

            return nullptr;
        }

        /**
         * @brief A copy of the parented sprites: the renderer's destructor lets go of each, and
         * that calls back into Forget, which changes the set.
         */
        std :: set<SunLight :: Sprite :: Sprite*> SpriteRegistry :: ParentedSprites( void ) const  {

            return m_Parented;
        }

        /**
         * @brief Stop tracking every parented sprite (the renderer's destructor, once it let go of them).
         */
        void SpriteRegistry :: ClearParented( void )  {

            m_Parented.clear();
        }

        /**
         * @brief Start a multi-pass frame: no sprite advanced or pending yet.
         */
        void SpriteRegistry :: BeginFrame( void )  {

            m_FrameAdvanced.clear();
            m_FramePending.clear();
        }

        /**
         * @brief Update one layer's sprites for the pass being drawn.
         *
         * L3 of the master plan: a layer's own sprites draw in ascending z-order, stably - equal
         * values (every sprite at the default 0, among others) keep the order they were added in
         * (Add's push_back), so an all-zero layer's draw order is byte-identical to before L3.
         * Re-sorted on every call rather than only when a z-order actually changed: stable_sort on
         * an already-correctly-ordered deque reproduces that same order exactly, so sorting
         * unconditionally is always correct, and SpriteList's raw pointers give it std::deque's
         * own random-access iterators, keeping the sort itself cheap (O(n log n) comparisons) even
         * at this frequency - a dirty-flag optimization was not worth the extra sprite-to-renderer
         * wiring it would need (Sprite has no back-channel to its owning renderer) for a cost no
         * test or real workload here has shown to matter.
         *
         * With a single view each sprite is updated (Sprite::Update: Advance then Draw). With
         * several (bMultiView), several passes may draw a sprite in one frame (one per view showing
         * its layer): its animation state must still advance only once per frame, in the FIRST
         * pass in which it is on screen (a canvas only steps while it is inside the viewport of
         * the view being drawn, so advancing in a pass that does not show it would waste the step
         * while a later view does), and every pass just draws the frame that settled. A sprite
         * reached by passes but on screen in none of them is advanced once at the end of the
         * frame (EndFrame), as a single view would have done: its frame choice still steps, its
         * canvas stands still.
         * @param nLayerId The layer; one with no list draws nothing;
         * @param bMultiView Whether the frame has more than one view pass (see BeginFrame/EndFrame);
         */
        void SpriteRegistry :: UpdateLayer( int nLayerId, bool bMultiView )  {

            std :: map<int, SpriteList> :: iterator  itLayer = m_Layers.find( nLayerId );

            if( itLayer == m_Layers.end() )
                return;

            std :: stable_sort( itLayer -> second.begin(), itLayer -> second.end(),
                                 []( SunLight :: Sprite :: Sprite *pLeft, SunLight :: Sprite :: Sprite *pRight )  {
                                     return pLeft -> GetZOrder() < pRight -> GetZOrder();
                                 } );

            for( SunLight :: Sprite :: Sprite *pSprite : itLayer -> second )  {
                if( !bMultiView )  {
                    pSprite -> Update();
                    continue;
                }

                if( m_FrameAdvanced.find( pSprite ) == m_FrameAdvanced.end() )  {
                    if( pSprite -> IsOnScreen() )  {
                        pSprite -> Advance();
                        m_FrameAdvanced.insert( pSprite );
                        m_FramePending.erase( pSprite );
                    }
                    else  {
                        m_FramePending.insert( pSprite );
                    }
                }

                pSprite -> Draw();
            }
        }

        /**
         * @brief End a multi-pass frame: the sprites some pass reached but no pass had on screen
         * are advanced once, exactly as a single view would (see UpdateLayer). They draw nothing.
         */
        void SpriteRegistry :: EndFrame( void )  {

            for( SunLight :: Sprite :: Sprite *pSprite : m_FramePending )
                pSprite -> Advance();

            m_FramePending.clear();
        }

        /**
         * @brief Unload every registered sprite (Sprite::Unload) and close every layer's list. The
         * parented sprites stay tracked: they still have the renderer as their parent.
         */
        void SpriteRegistry :: UnloadAll( void )  {

            for( std :: pair<const int, SpriteList> &layer : m_Layers )  {
                for( SunLight :: Sprite :: Sprite *pSprite : layer.second )
                    pSprite -> Unload();
            }

            m_Layers.clear();
        }
    }
}
