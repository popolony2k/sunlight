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

#ifndef __COLLIDER_H__
#define __COLLIDER_H__

#include "canvas/basecanvas.h"


namespace SunLight {
    namespace Collision  {

        /**
         * @brief Collider implementation.
         * 
         */
        class Collider : public SunLight :: Canvas :: BaseCanvas {

            bool RectRect( float fRect1X,
                        float fRect1Y,
                        float fRect1Width,
                        float fRect1Height,
                        float fRect2X,
                        float fRect2Y,
                        float fRect2Width,
                        float fRect2Height );

            public:

            Collider( void );
            virtual ~Collider( void );

            bool Hit( SunLight :: TileMap :: stTile &tile );
            bool Hit( SunLight :: TileMap :: stDimension2D &dimension );
        };
    }
}

#endif /* __COLLIDER_H__ */
