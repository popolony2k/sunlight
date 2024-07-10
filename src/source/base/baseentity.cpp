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

#include "baseentity.h"
#include <cstring>


namespace SunLight  {
    namespace Base  {

        /**
         * Constructor. Initialize all class data.
         */
        BaseEntity :: BaseEntity( void )  {

            m_bVisible   = false;
            m_pDimension = &m_Dimension;
            std :: memset( &m_Dimension, 0, sizeof( m_Dimension ) );
        }

        /**
         * Destructor. Finalize all class data.
         */
        BaseEntity :: ~BaseEntity( void )  {

        }

        /**
         * Set the visible status of this entity.
         * @param bVisible The new visible status;
         */
        void BaseEntity :: SetVisible( bool bVisible )  {

            m_bVisible = bVisible;
        }

        /**
         * Get the visible status.
         */
        bool BaseEntity :: GetVisible( void )  {

            return m_bVisible;
        }

        /**
         * Set pointer to a new parent dimension object passed as parameter;
         * @param pDimension Pointer to the new @link stDimension2D object
         * that will be used by this entity;
         */
        void BaseEntity :: SetDimension2DPtr( SunLight :: TileMap :: stDimension2D* pDimension )  {

            m_pDimension = pDimension;
        }

        /**
         * Set the dimension of this entity.
         * @param dimension The new dimension of this entity;
         */
        void BaseEntity :: SetDimension2D( SunLight :: TileMap :: stDimension2D dimension )  {

            *m_pDimension = dimension;
        }

        /**
         * Get the reference of this entity object.
         */
        SunLight :: TileMap :: stDimension2D& BaseEntity :: GetDimension2D( void )  {

            return *m_pDimension;
        }
    }
}