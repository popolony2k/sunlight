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

            m_fInsetLeft   = 0.0f;
            m_fInsetTop    = 0.0f;
            m_fInsetRight  = 0.0f;
            m_fInsetBottom = 0.0f;
        }

        /**
         * Destructor. Finalize all class data.
         */
        Collider :: ~Collider( void )  {

        }

        /**
         * Compute the rectangle actually used for this collider's own side
         * of an overlap test - the parent's full dimension with SetInset's
         * percentages shaved off each edge. See SetInset's own doc comment
         * for why this is recomputed here rather than cached.
         * @param fX Receives the effective rectangle's X position;
         * @param fY Receives the effective rectangle's Y position;
         * @param fWidth Receives the effective rectangle's width;
         * @param fHeight Receives the effective rectangle's height;
         */
        void Collider :: GetEffectiveRect( float &fX, float &fY, float &fWidth, float &fHeight )  {

            SunLight :: TileMap :: stDimension2D&  self = GetDimension2D();
            float                                 fInsetLeftPx   = self.size.nWidth  * m_fInsetLeft;
            float                                 fInsetTopPx    = self.size.nHeight * m_fInsetTop;
            float                                 fInsetRightPx  = self.size.nWidth  * m_fInsetRight;
            float                                 fInsetBottomPx = self.size.nHeight * m_fInsetBottom;

            fX      = self.pos.x + fInsetLeftPx;
            fY      = self.pos.y + fInsetTopPx;
            fWidth  = self.size.nWidth  - fInsetLeftPx - fInsetRightPx;
            fHeight = self.size.nHeight - fInsetTopPx  - fInsetBottomPx;
        }

        /**
         * Check if collider object area has been hit by tile passed as parameter.
         * @param tile Reference to the tile struct containing all tile information;
         */
        bool Collider :: Hit( SunLight :: TileMap :: stTile &tile )  {

            SunLight :: TileMap :: stDimension2D&  viewport    = GetViewport().GetDimension2D();
            tmx_object                           *pCollision = tile.pTile -> collision;
            bool                                 bHit        = false;
            float                                fSelfX, fSelfY, fSelfWidth, fSelfHeight;

            GetEffectiveRect( fSelfX, fSelfY, fSelfWidth, fSelfHeight );

            while( pCollision )  {

                switch( pCollision -> obj_type )  {
                    case OT_SQUARE :
                        bHit = RectRect( fSelfX + ( float ) viewport.pos.x,
                                         fSelfY + ( float ) viewport.pos.y,
                                         fSelfWidth,
                                         fSelfHeight,
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

            float  fSelfX, fSelfY, fSelfWidth, fSelfHeight;

            GetEffectiveRect( fSelfX, fSelfY, fSelfWidth, fSelfHeight );

            return RectRect( fSelfX,
                             fSelfY,
                             fSelfWidth,
                             fSelfHeight,
                             ( float ) dimension.pos.x,
                             ( float ) dimension.pos.y,
                             ( float ) dimension.size.nWidth,
                             ( float ) dimension.size.nHeight );
        }

        /**
         * Check if this collider's own effective (SetInset-shrunk)
         * rectangle overlaps another collider's own effective rectangle -
         * unlike the stDimension2D overload above (which only ever applies
         * THIS side's inset against the other's raw, full area - by
         * design, for callers testing against an arbitrary rectangle that
         * isn't itself a Collider, e.g. a tile's collision shape), this
         * applies BOTH colliders' own SetInset shrink, so a collider-to-
         * collider overlap test correctly honors each side's own inset
         * independently. CollisionManager::Update() uses this one for it's
         * collider-to-collider rule checks - using the stDimension2D
         * overload there instead silently ignored the second collider's
         * own inset entirely, since only the first side's GetEffectiveRect
         * was ever consulted (found via Caravellius, 2026-08-11 - a
         * boss's own head hitbox never responded to SetInset at all,
         * however aggressively it was shrunk, because it was always the
         * second collider in it's own collision_add_rule pairing).
         * @param other The other collider to test against;
         */
        bool Collider :: Hit( Collider &other )  {

            float  fSelfX, fSelfY, fSelfWidth, fSelfHeight;
            float  fOtherX, fOtherY, fOtherWidth, fOtherHeight;

            GetEffectiveRect( fSelfX, fSelfY, fSelfWidth, fSelfHeight );
            other.GetEffectiveRect( fOtherX, fOtherY, fOtherWidth, fOtherHeight );

            return RectRect( fSelfX,
                             fSelfY,
                             fSelfWidth,
                             fSelfHeight,
                             fOtherX,
                             fOtherY,
                             fOtherWidth,
                             fOtherHeight );
        }

        /**
         * Set the inset (as fractions of the parent's own width/height)
         * shrinking the rectangle this collider's Hit() checks use for it's
         * own side of the test. See the header's own doc comment for the
         * full explanation.
         * @param fLeftPct Fraction of width to shrink off the left edge;
         * @param fTopPct Fraction of height to shrink off the top edge;
         * @param fRightPct Fraction of width to shrink off the right edge;
         * @param fBottomPct Fraction of height to shrink off the bottom edge;
         */
        void Collider :: SetInset( float fLeftPct, float fTopPct, float fRightPct, float fBottomPct )  {

            m_fInsetLeft   = fLeftPct;
            m_fInsetTop    = fTopPct;
            m_fInsetRight  = fRightPct;
            m_fInsetBottom = fBottomPct;
        }
    }
}