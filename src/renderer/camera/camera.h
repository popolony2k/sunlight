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

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <cstdint>
#include "base/primitives.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief One view's camera (S1e of the master plan): where it looks and how far one scroll
         * step goes. It knows nothing about TileMapRenderer or View: the map's size, the viewport
         * and the zoom are handed to each call that needs them.
         *
         * The position is stored as the NEGATED map point shown at the viewport's top-left (the
         * offset every map position gets: a map position p is drawn at (p + camera) x zoom +
         * viewport.pos). SetPosition/GetPosition take and give the plain map point.
         */
        class Camera  {

            SunLight :: Base :: stVector2D  m_Position;
            int                             m_nScrollStepWidth;
            int                             m_nScrollStepHeight;

            public:

            Camera( void );

            SunLight :: Base :: stVector2D Position( void ) const;
            void SetPosition( int nX, int nY );
            void GetPosition( int &nX, int &nY ) const;
            void Reset( void );

            void SetScrollStep( int nStepWidth, int nStepHeight );
            void GetScrollStep( int &nStepWidth, int &nStepHeight ) const;
            void ResolveScrollStep( const SunLight :: Base :: stSize2D &defaultStep );

            void MoveUp( int nViewportHeight, float fZoom, int nMapHeight );
            void MoveDown( void );
            void MoveLeft( int nViewportWidth, float fZoom, int nMapWidth );
            void MoveRight( void );

            void Align( int nAlignment, uint16_t nMapWidth, uint16_t nMapHeight,
                        const SunLight :: Base :: stDimension2D &viewport, float fZoomFactor );

            SunLight :: Base :: stVector2D ForParallax( double fFactorX, double fFactorY,
                                                        double fOriginX, double fOriginY ) const;
        };
    }
}

#endif  /* __CAMERA_H__ */
