/*
 * baseentity.h
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#ifndef __BASEENTITY_H__
#define __BASEENTITY_H__

#include "tilemap/tilemapdefs.h"
#include "base/object.h"


namespace SunLight  {
    namespace Base  {
        class BaseEntity : public SunLight :: Base :: Object  {

            SunLight :: TileMap :: stDimension2D    *m_pDimension;
            SunLight :: TileMap :: stDimension2D    m_Dimension;
            bool                                  m_bVisible;


            public:

            BaseEntity( void );
            virtual ~BaseEntity( void );

            virtual void SetVisible( bool bVisible );
            virtual bool GetVisible( void );

            virtual void SetDimension2DPtr( SunLight :: TileMap :: stDimension2D* pDimension );
            void SetDimension2D( SunLight :: TileMap :: stDimension2D dimension );
            SunLight :: TileMap :: stDimension2D& GetDimension2D( void );
        };
    }
}

#endif /* __BASEENTITY_H__ */
