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

            /**
             * A frame is two halves: STATE (advance animation) and DRAWING
             * (put the current state on screen). Advance() must run once per
             * frame; Draw() changes no state, so it may run several times for
             * the same frame (e.g. once per view). Children with animation
             * state implement both; the defaults do nothing.
             */
            virtual void Advance( void )  {};
            virtual void Draw( void )  {};

            /**
             * Whether the viewport test that gates Advance() and Draw()
             * passes right now, in the view being drawn (visible, and its
             * rectangle not entirely past the viewport's far edges). Advance()
             * only steps a canvas while this is true, so
             * whoever runs several passes for one frame asks it to advance in
             * the first pass that shows it. True by default.
             */
            virtual bool IsOnScreen( void )  { return true; }

            /**
             * Must be implemented by children objects to provide
             * it's own draw behavior. The classic single-call frame step:
             * Advance() followed by Draw().
             */
            virtual void Update( void )  {};
        };
    }
}

#endif /* __CANVAS_H__ */
