/*
 * canvas.cpp
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
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
