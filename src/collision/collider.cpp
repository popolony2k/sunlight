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

#include "collider.h"
#include <cstring>

// Disable data loss warning on windows
// #if defined( _WIN32 ) || defined( _WIN64 )
// #pragma warning(disable:4244)
// #endif


namespace SunLight {
    namespace Collision  {

        /**
         * Check if two rectangle areas are colliding;
         * @param fRect1X X coordinate of first rectangle;
         * @param fRect1Y Y coordinate of first rectangle;
         * @param fRect1Width Width of first rectangle;
         * @param fRect1Height Height of first rectangle;
         * @param fRect2X X coordinate of second rectangle;
         * @param fRect2Y Y coordinate of second rectangle;
         * @param fRect2Width Width of second rectangle;
         * @param fRect2Height Height of second rectangle;
         */
        bool Collider :: RectRect( float fRect1X,
                                float fRect1Y,
                                float fRect1Width,
                                float fRect1Height,
                                float fRect2X,
                                float fRect2Y,
                                float fRect2Width,
                                float fRect2Height )  {

            if( ( ( fRect1X + fRect1Width ) >= fRect2X )  &&    // r1 right edge past r2 left
                ( fRect1X <= ( fRect2X + fRect2Width ) )  &&    // r1 left edge past r2 right
                ( ( fRect1Y + fRect1Height ) >= fRect2Y ) &&    // r1 top edge past r2 bottom
                ( fRect1Y <= ( fRect2Y + fRect2Height ) ) ) {   // r1 bottom edge past r2 top

                return true;
            }

            return false;
        }

        /**
         * Constructor. Initialize all class data.
         */
        Collider :: Collider( void )  {

        }

        /**
         * Destructor. Finalize all class data.
         */
        Collider :: ~Collider( void )  {

        }

        /**
         * Check if collider object area has been hit by tile passed as parameter.
         * @param tile Reference to the tile struct containing all tile information;
         */
        bool Collider :: Hit( SunLight :: TileMap :: stTile &tile )  {

            SunLight :: TileMap :: stDimension2D&  viewport    = GetViewport().GetDimension2D();
            SunLight :: TileMap :: stDimension2D&  dimension   = GetDimension2D();
            tmx_object                           *pCollision = tile.pTile -> collision;
            bool                                 bHit        = false;

            while( pCollision )  {

                switch( pCollision -> obj_type )  {
                    case OT_SQUARE :
                        bHit = RectRect( ( float ) ( dimension.pos.x + viewport.pos.x ),
                                         ( float ) ( dimension.pos.y + viewport.pos.y ),
                                         ( float ) dimension.size.nWidth,
                                         ( float ) dimension.size.nHeight,
                                         ( float ) ( tile.dimension.pos.x + pCollision -> x ),
                                         ( float ) ( tile.dimension.pos.y + pCollision -> y ),
                                         ( float ) pCollision -> width,
                                         ( float ) pCollision -> height );
                        break;
                    case OT_TILE :
                        /* Still not supported */
                        break;
                    case OT_POINT :
                        /* Still not supported */
                        break;
                    case OT_POLYGON :
                        /* Still not supported */
                        break;
                    case OT_POLYLINE :
                        /* Still not supported */
                        break;
                    case OT_ELLIPSE :
                        /* Still not supported */
                        break;
                    case OT_NONE :
                        /* Still not supported */
                        break;
                    case OT_TEXT :
                        /* Still not supported */
                        break;
                }

                if( bHit )
                    return true;
                else
                    pCollision = tile.pTile -> collision -> next;
            }

            return bHit;
        }

        /**
         * Check if collider object area has been hit by a draw entity passed as
         * parameter.
         * @param dimension Reference to a struct containing the area to be checked;
         */
        bool Collider :: Hit( SunLight :: TileMap :: stDimension2D &dimension )  {

            SunLight :: TileMap :: stDimension2D&  self = GetDimension2D();

            return RectRect( ( float ) self.pos.x,
                             ( float ) self.pos.y,
                             ( float ) self.size.nWidth,
                             ( float ) self.size.nHeight,
                             ( float ) dimension.pos.x,
                             ( float ) dimension.pos.y,
                             ( float ) dimension.size.nWidth,
                             ( float ) dimension.size.nHeight );
        }
    }
}