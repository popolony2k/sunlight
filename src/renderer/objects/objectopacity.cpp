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

#include "renderer/objects/objectopacity.h"
#include "renderer/shape/shapeobjects.h"
#include <cstdio>


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief An object's effective opacity: its own "opacity" property (0 to 1) when it has a
         * usable one, otherwise fLayerOpacity unchanged - the object has no opacity field of its
         * own in Tiled, so this is the only way to override it per object. An unusable property
         * (wrong type, or outside 0 to 1) warns once for that object and falls back to
         * fLayerOpacity too, the same as having none.
         * @param pObject The object, as libtmx parsed it;
         * @param fLayerOpacity Its layer's own opacity, used when the object has none of its own;
         * @return The effective opacity to draw this object with, 0 to 1;
         */
        float ObjectOpacity :: Of( tmx_object* pObject, float fLayerOpacity )  {

            double  fValue;

            switch( Shape :: ObjectOpacityOf( pObject, fValue ) )  {
                case Shape :: OpacityProperty :: VALID :
                    return ( float ) fValue;

                case Shape :: OpacityProperty :: INVALID :
                    WarnInvalid( pObject -> id );
                    return fLayerOpacity;

                case Shape :: OpacityProperty :: NONE :
                default :
                    return fLayerOpacity;
            }
        }

        /**
         * @brief Log, once per object id, that its own "opacity" property is not usable.
         */
        void ObjectOpacity :: WarnInvalid( unsigned int nObjectId )  {

            if( !m_WarnedObjects.insert( nObjectId ).second )
                return;

            fprintf( stderr, "Object %u's own \"opacity\" property is not a number in 0..1; using its layer's opacity instead.\n", nObjectId );
        }

        /**
         * @brief Forget the warnings already given, so an object warns again the next time
         * (TileMapRenderer calls this from Stop(), alongside FontRegistry::Clear).
         */
        void ObjectOpacity :: Reset( void )  {

            m_WarnedObjects.clear();
        }
    }
}
