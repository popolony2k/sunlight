/*
 * collider.h
 *
 *  Created on: Jul 2, 2021
 *      Author: popolony2k
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
