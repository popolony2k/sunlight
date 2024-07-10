/*
 * canvas.h
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "canvas/basecanvas.h"
#include "collision/collider.h"
#include "base/color.h"


namespace SunLight {
    namespace Canvas  {
        /**
         * @brief Canvas class implementation.
         * 
         */
        class Canvas : public SunLight :: Canvas :: BaseCanvas {

            SunLight :: Collision :: Collider  m_Collider;
            SunLight :: Base :: stColor        m_Color;


            public:

            Canvas( void );
            virtual ~Canvas( void );

            void SetParent( BaseCanvas *pParent );
            void SetDimension2DPtr( SunLight :: TileMap :: stDimension2D* pDimension );

            void SetColor( SunLight :: Base :: stColor color );
            SunLight :: Base :: stColor& GetColor( void );

            SunLight :: Collision :: Collider& GetCollider( void );
        };
    }
}

#endif /* __CANVAS_H__ */
