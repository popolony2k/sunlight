/*
 * baseentity.cpp
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
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