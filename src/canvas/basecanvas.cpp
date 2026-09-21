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

#include "basecanvas.h"
#include <stdio.h>


namespace SunLight {
    namespace Canvas  {
        /**
         * Constructor. Initialize all class data.
         */
        BaseCanvas :: BaseCanvas( void )  {

            m_pParent     = NULL;
            m_bWorldSpace = false;
            m_pViewport = &m_Viewport;
        }

        /**
         * Destructor. Finalize all class data.
         */
        BaseCanvas :: ~BaseCanvas( void )  {

        }

        /**
         * Set parent object.
         * @param pParent The new parent object of this object;
         */
        void BaseCanvas :: SetParent( BaseCanvas *pParent )  {

            m_pParent = pParent;
        }

        /**
         * A child is being destroyed or moved to another parent: nothing to
         * do here, a parent that keeps pointers to its children overrides this.
         * @param pChild The child that is leaving;
         */
        void BaseCanvas :: ChildRemoved( BaseCanvas *pChild )  {

        }

        /**
         * Return the parent object of this object;
         */
        BaseCanvas* BaseCanvas :: GetParent( void )  {

            return m_pParent;
        }

        /**
         * Make this canvas' position a MAP position (true) or leave it relative to
         * the view that draws it (false, the default). See the header.
         * @param bWorldSpace The new mode;
         */
        void BaseCanvas :: SetWorldSpace( bool bWorldSpace )  {

            m_bWorldSpace = bWorldSpace;
        }

        bool BaseCanvas :: IsWorldSpace( void )  {

            return m_bWorldSpace;
        }

        /**
         * The camera offset of the view currently being drawn, asked of the
         * parent chain (only the renderer knows the cameras). A canvas with
         * no renderer above it has none.
         */
        void BaseCanvas :: GetCameraOffset( float &fX, float &fY )  {

            if( m_pParent )  {
                m_pParent -> GetCameraOffset( fX, fY );
            }
            else  {
                fX = 0.0f;
                fY = 0.0f;
            }
        }

        /**
         * Set the visible status of this entity.
         * @param bVisible The new visible status;
         */
        void BaseCanvas :: SetVisible( bool bVisible )  {

            GraphicObject :: SetVisible( bVisible );
        }

        /**
         * Get the visible status.
         */
        bool BaseCanvas :: GetVisible( void )  {

            if( m_pParent )
                return ( m_pParent -> GetVisible() ? GraphicObject :: GetVisible() : false );

            return GraphicObject :: GetVisible();
        }

        /**
         * Set a new viewport object to this canvas.
         * @param pViewport The new viewport object to set;
         */
        void BaseCanvas :: SetViewport( SunLight :: Base :: Viewport *pViewport )  {

            m_pViewport = pViewport;
        }

        /**
         * Get the viewport of this object. If this object is a child of
         * another object, the viewport returned is the parent viewport;
         */
        SunLight :: Base :: Viewport& BaseCanvas :: GetViewport( void )  {

            return ( m_pParent ? m_pParent -> GetViewport() : *m_pViewport );
        }
    }
}