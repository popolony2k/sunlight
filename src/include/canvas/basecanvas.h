/*
 * basecanvas.h
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#ifndef __BASECANVAS_H__
#define __BASECANVAS_H__

#include "base/baseentity.h"
#include "base/viewport.h"


namespace SunLight {
    namespace Canvas  {
        /**
         * @brief Base canvas implementation.
         * 
         */
        class BaseCanvas : public SunLight :: Base :: BaseEntity  {

            BaseCanvas                    *m_pParent;
            SunLight :: Base :: Viewport  *m_pViewport;
            SunLight :: Base :: Viewport  m_Viewport;


            public:

            BaseCanvas( void );
            virtual ~BaseCanvas( void );

            virtual void SetParent( BaseCanvas *pParent );
            BaseCanvas* GetParent( void );

            virtual void SetVisible( bool bVisible );
            virtual bool GetVisible( void );

            void SetViewport( SunLight :: Base :: Viewport *pViewport );
            SunLight :: Base :: Viewport& GetViewport( void );
        };
    }
}

#endif /* __BASECANVAS_H__ */
