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

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include <vector>
#include "base/graphicobject.h"


namespace SunLight  {
    namespace Base  {

        /*
         * The zoom scale, as public constants - the single source of truth
         * the viewport itself is built from, so a consumer that needs the
         * numbers (e.g. to validate a requested zoom) reads them here
         * instead of hardcoding a copy that can drift.
         *
         * A zoom POSITION p in [ZOOM_POS_MIN, ZOOM_POS_MAX] has the factor
         *
         *     factor( p ) = ( p + 1 ) x ZOOM_STEP
         *
         * so position 0 is ZOOM_FACTOR_MIN and position ZOOM_POS_MAX is
         * ZOOM_FACTOR_MAX. ZOOM_STEP is a power of two (1/16), so every
         * factor in the table is exactly representable in a float - factors
         * can be compared, and checked for being a multiple of the step,
         * with exact arithmetic.
         */
        constexpr float     ZOOM_STEP           = 0.0625f;
        constexpr unsigned  ZOOM_POS_MIN        = 0u;
        constexpr unsigned  ZOOM_POS_COUNT      = 256u;                      // number of positions; also the EXCLUSIVE upper bound of the valid range
        constexpr unsigned  ZOOM_POS_MAX        = ZOOM_POS_COUNT - 1u;       // the last VALID position (255)
        constexpr unsigned  ZOOM_POS_DEFAULT    = ( unsigned ) ( 1.0f / ZOOM_STEP ) - 1u;   // the default preferred position (15), whose factor is 1.0
        constexpr float     ZOOM_FACTOR_MIN     = ZOOM_STEP;                 // factor of position ZOOM_POS_MIN (0.0625)
        constexpr float     ZOOM_FACTOR_MAX     = ZOOM_POS_COUNT * ZOOM_STEP;   // factor of position ZOOM_POS_MAX (16.0)

        /**
         * Internal zoom properties.
         */
        struct stZoomProperties {
            unsigned       nCurrentZoomPos;
            unsigned       nPreferredZoomPos;
            float          fZoomFactor;
            bool           bEnabledUserZoom;
        };

        /**
         * Abstract class used to manage zoom engine for all
         * interface graphic components.
         *
         * A viewport is also a RECTANGLE in the render target's
         * coordinates, set with SetDimension2D and read with
         * GetDimension2D: `pos` is its top-left corner and `size` its
         * width and height - the visible area is [pos, pos + size), so
         * (10, 10, 1240, 900) shows x in [10, 1250) and y in [10, 910).
         * `size` is a width/height, never the coordinate of the far edge.
         *
         * (Before v0.29.0 the clipping treated `size` as the far-edge
         * coordinate whenever pos was not (0, 0): a legacy viewport of
         * (10, 10, 1250, 910) is the same visible area as (10, 10, 1240,
         * 900) now. Origin-anchored viewports, pos (0, 0), mean the same
         * in both.)
         */
        class Viewport : public SunLight :: Base :: GraphicObject {

            typedef std :: vector<float> ZoomFactorList;
            typedef std :: pair<unsigned, unsigned> ZoomBorderLimits;

            stZoomProperties           m_Props;


            void InitializeZoomEngine( void );
            void ClampZoomToLimits( void );

            public:

            Viewport( void );
            virtual ~Viewport( void );

            virtual void SetEnableUserZoom( bool bEnabled );
            virtual bool GetEnableUserZoom( void );
            virtual void SetPreferredZoom( unsigned nZoomPos );
            virtual void SetMinZoom( unsigned nMinPos );
            virtual void SetMaxZoom( unsigned nMaxPos );
            virtual void GetZoomLimits( unsigned &nMinPos, unsigned &nMaxPos );
            virtual void SetZoom( unsigned nZoomPos );
            virtual void ResetZoom( void );
            virtual void ZoomIn( void );
            virtual void ZoomOut( void );
            virtual float GetZoomFactor( unsigned int nZoomPos );

            void SetZoomPropertiesPtr( stZoomProperties *pProps );
            stZoomProperties& GetZoomProperties( void );

            virtual bool GetClippedRect( SunLight :: TileMap :: stDimension2D src,
                                         SunLight :: TileMap :: stDimension2D& dst );

            protected:

            ZoomBorderLimits           m_ZoomBorderLimits;
            ZoomFactorList             m_vZoomFactorList;
            stZoomProperties           *m_pProps;
        };
    }
}

#endif /* __VIEWPORT_H__ */
