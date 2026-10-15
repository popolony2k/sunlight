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

#ifndef __SPRITEREGISTRY_H__
#define __SPRITEREGISTRY_H__

#include <deque>
#include <map>
#include <set>
#include <vector>
#include "sprite/sprite.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief The sprites a renderer draws (S1d of the master plan): the ones registered on each
         * layer, the ones it is the parent of, and the once-per-frame update of a layer's sprites
         * across the frame's view passes. It never owns a sprite (the caller does) and knows nothing
         * about TileMapRenderer: the renderer keeps the checks around it (started, the layer exists,
         * the sprite's collider), sets the sprite's parent, and tells it which layers to update when.
         */
        class SpriteRegistry  {

            typedef std :: deque<SunLight :: Sprite :: Sprite*>  SpriteList;

            std :: map<int, SpriteList>                m_Layers;            // ascending layer id: the mapless draw order
            std :: set<SunLight :: Sprite :: Sprite*>  m_Parented;          // a superset of m_Layers' sprites (see MarkParented)

            // Multi-pass frame: the sprites already advanced this frame, and those a pass reached but
            // had not on screen (yet) - see UpdateLayer.
            std :: set<SunLight :: Sprite :: Sprite*>  m_FrameAdvanced;
            std :: set<SunLight :: Sprite :: Sprite*>  m_FramePending;

            public:

            void OpenLayer( int nLayerId );
            bool Add( int nLayerId, SunLight :: Sprite :: Sprite *pSprite );
            bool Contains( int nLayerId, SunLight :: Sprite :: Sprite *pSprite ) const;
            bool Remove( int nLayerId, SunLight :: Sprite :: Sprite *pSprite );
            bool Empty( void ) const;
            std :: vector<int> LayerIds( void ) const;

            void MarkParented( SunLight :: Sprite :: Sprite *pSprite );
            SunLight :: Sprite :: Sprite* Forget( SunLight :: Canvas :: BaseCanvas *pChild, std :: vector<int> &layerIds );
            std :: set<SunLight :: Sprite :: Sprite*> ParentedSprites( void ) const;
            void ClearParented( void );

            void BeginFrame( void );
            void UpdateLayer( int nLayerId, bool bMultiView );
            void EndFrame( void );

            void UnloadAll( void );
        };
    }
}

#endif  /* __SPRITEREGISTRY_H__ */
