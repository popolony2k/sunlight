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

#include "canvas.h"
#include <cstring>


namespace SunLight {
    namespace Canvas  {
        /**
         * Constructor. Initialize all class data.
         */
        Canvas :: Canvas( void )  {

            m_Collider.SetParent( this );
            m_Collider.SetDimension2DPtr( &GetDimension2D() );
        }

        /**
         * Destructor. Finalize all class data.
         */
        Canvas :: ~Canvas( void )  {

        }

        /**
         * Set parent object.
         * @param pParent The new parent object of this object;
         */
        void Canvas :: SetParent( BaseCanvas *pParent )  {

            BaseCanvas :: SetParent( pParent );
            m_Collider.SetParent( pParent );
        }

        /**
         * Set pointer to a new parent dimension object passed as parameter;
         * @param pDimension Pointer to the new @link stDimension2D object
         * that will be used by this entity;
         */
        void Canvas :: SetDimension2DPtr( SunLight :: TileMap :: stDimension2D* pDimension )  {

            BaseCanvas :: SetDimension2DPtr( pDimension );
            m_Collider.SetDimension2DPtr( pDimension );
        }

        /**
         * Set the entity color.
         * @param color The RGB color to set;
         */
        void Canvas :: SetColor( SunLight :: Base :: stColor color )  {

            std :: memcpy( &m_Color, &color, sizeof( color ) );
        }

        /**
         * Return the reference to the internal color struct.
         */
        SunLight :: Base :: stColor& Canvas :: GetColor( void )  {

            return m_Color;
        }

        /**
         * Return the reference to the internal collider object.
         */
        SunLight :: Collision :: Collider& Canvas :: GetCollider( void )  {

            return m_Collider;
        }
    }
}
