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
#include "base/baseentity.h"


namespace SunLight  {
    namespace Base  {
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
         */
        class Viewport : public SunLight :: Base :: BaseEntity {

            typedef std :: vector<float> ZoomFactorList;
            typedef std :: pair<unsigned, unsigned> ZoomBorderLimits;

            stZoomProperties           m_Props;


            void InitializeZoomEngine( void );

            public:

            Viewport( void );
            virtual ~Viewport( void );

            virtual void SetEnableUserZoom( bool bEnabled );
            virtual void SetPreferredZoom( unsigned nZoomPos );
            virtual void SetMinZoom( unsigned nMinPos );
            virtual void SetMaxZoom( unsigned nMaxPos );
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
