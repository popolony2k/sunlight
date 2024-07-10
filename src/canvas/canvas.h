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
