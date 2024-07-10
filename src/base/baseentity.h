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