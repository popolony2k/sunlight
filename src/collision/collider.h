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

            float  m_fInsetLeft;
            float  m_fInsetTop;
            float  m_fInsetRight;
            float  m_fInsetBottom;

            bool RectRect( float fRect1X,
                        float fRect1Y,
                        float fRect1Width,
                        float fRect1Height,
                        float fRect2X,
                        float fRect2Y,
                        float fRect2Width,
                        float fRect2Height );

            void GetEffectiveRect( float &fX,
                                   float &fY,
                                   float &fWidth,
                                   float &fHeight );

            public:

            Collider( void );
            virtual ~Collider( void );

            bool Hit( SunLight :: TileMap :: stTile &tile );
            bool Hit( SunLight :: TileMap :: stDimension2D &dimension );
            bool Hit( Collider &other );

            /**
             * @brief Shrinks the rectangle actually used for this collider's
             * own overlap tests, relative to it's parent's full render
             * dimension - the dimension itself (position/size, tracked via
             * SetDimension2DPtr) is untouched, so this only affects the
             * area Hit() considers "self", not what's drawn or how the
             * parent's position is tracked. Every percentage is a fraction
             * (0.0-1.0) of the parent's own width/height, recomputed from
             * the current dimension on every Hit() call rather than cached
             * as pixels, so it stays correct even if the parent's own size
             * changes at runtime (e.g. a differently-sized active texture
             * sequence). Defaults to 0 on every side (full-size collision,
             * today's behavior) until explicitly set. Not clamped/validated -
             * opposing percentages summing past 1.0 on an axis (e.g.
             * fLeftPct=0.6, fRightPct=0.6) push that axis's effective size
             * negative; RectRect() doesn't crash or false-positive on that,
             * it just fails toward "never hits", but it's still the caller's
             * responsibility to pass sane values (same convention as
             * TileMapRenderer::SetCameraPosition's own unclamped position).
             * @param fLeftPct Fraction of width to shrink off the left edge;
             * @param fTopPct Fraction of height to shrink off the top edge;
             * @param fRightPct Fraction of width to shrink off the right edge;
             * @param fBottomPct Fraction of height to shrink off the bottom edge;
             */
            void SetInset( float fLeftPct,
                           float fTopPct,
                           float fRightPct,
                           float fBottomPct );
        };
    }
}

#endif /* __COLLIDER_H__ */
