/*
 * viewport.h
 *
 *  Created on: Jul 13, 2021
 *      Author: popolony2k
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
