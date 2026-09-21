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

#ifndef __BASECANVAS_H__
#define __BASECANVAS_H__

#include "base/graphicobject.h"
#include "base/viewport.h"


namespace SunLight {
    namespace Canvas  {
        /**
         * @brief Base canvas implementation.
         * 
         */
        class BaseCanvas : public SunLight :: Base :: GraphicObject  {

            BaseCanvas                    *m_pParent;
            bool                          m_bWorldSpace;
            SunLight :: Base :: Viewport  *m_pViewport;
            SunLight :: Base :: Viewport  m_Viewport;


            public:

            BaseCanvas( void );
            virtual ~BaseCanvas( void );

            virtual void SetParent( BaseCanvas *pParent );
            BaseCanvas* GetParent( void );

            // Called by a child on its parent when the child is DESTROYED or given a different parent, so
            // that a parent which keeps raw pointers to its children (TileMapRenderer's registered
            // sprites) can forget it. Does nothing by default.
            virtual void ChildRemoved( BaseCanvas *pChild );

            // WORLD SPACE. By default a canvas' position is relative to the view that draws it (a sprite is
            // drawn at position x zoom from the view's origin and ignores the view's camera). A world-space
            // canvas' position is in MAP coordinates instead: it is drawn where the map is drawn at that
            // position - the camera of whichever view draws it is added first, exactly as for a map tile -
            // so one canvas is correct in every view whatever its zoom and camera.
            // COLLISION: a collider uses its owner's dimension as is, so the collider of a world-space
            // sprite is in MAP coordinates and one of a screen-relative sprite is not - do not put both
            // kinds in one collision rule (ColliderToColliderRule).
            virtual void SetWorldSpace( bool bWorldSpace );
            bool IsWorldSpace( void );

            // The offset to ADD to a map position to get a position relative to the view being drawn: the
            // camera of the active view, i.e. the negated map point shown at the view's top-left. 0, 0 unless
            // an ancestor - the renderer - answers otherwise.
            virtual void GetCameraOffset( float &fX, float &fY );

            virtual void SetVisible( bool bVisible );
            virtual bool GetVisible( void );

            void SetViewport( SunLight :: Base :: Viewport *pViewport );
            SunLight :: Base :: Viewport& GetViewport( void );
        };
    }
}

#endif /* __BASECANVAS_H__ */
