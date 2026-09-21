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

#include "viewport.h"
#include <algorithm>


/*
 * Zoom defaults (the scale itself - step, positions, default position - is
 * the public ZOOM_* constants in viewport.h).
 */
#define __DEFAULT_USER_ZOOM_STATUS      true


namespace SunLight  {
    namespace Base  {
        /**
         * Initialize the zoom engine.
         */
        void Viewport :: InitializeZoomEngine( void )  {

            float   fZoomStep = 0.0;

            /*
            * Fill all zoom factor list.
            */
            for( unsigned nCount = 0; nCount < ZOOM_POS_COUNT; nCount++ )  {
                m_vZoomFactorList.push_back(fZoomStep+=ZOOM_STEP );
            }

            m_pProps = &m_Props;
            m_ZoomBorderLimits.first     = ZOOM_POS_MIN;
            m_ZoomBorderLimits.second    = ZOOM_POS_COUNT;

            SetPreferredZoom( ZOOM_POS_DEFAULT );

            m_pProps -> bEnabledUserZoom  = __DEFAULT_USER_ZOOM_STATUS;
            m_pProps -> nCurrentZoomPos   = m_pProps -> nPreferredZoomPos;
            m_pProps -> fZoomFactor       = m_vZoomFactorList[m_pProps -> nPreferredZoomPos];
        }

        /**
         * Constructor. Initialize all class data.
         */
        Viewport :: Viewport( void )  {

            InitializeZoomEngine();
        }

        /**
         * Destructor. Finalize all class data.
         */
        Viewport :: ~Viewport( void )  {

        }

        /**
         * Enable or disable the user zoom mode (false disables user zoom by
         * mouse, keyboard, joystick, touch, ...);
         * @param bEnabled The new user zoom mode;
         */
        void Viewport :: SetEnableUserZoom( bool bEnabled )  {

            m_pProps -> bEnabledUserZoom = bEnabled;
        }

        /**
         * Query whether user zoom is enabled (see @see SetEnableUserZoom).
         */
        bool Viewport :: GetEnableUserZoom( void )  {

            return m_pProps -> bEnabledUserZoom;
        }

        /**
         * Pull the current and preferred zoom positions (and the current
         * factor) back inside the configured [min, max] limits, after they
         * changed - a limit change must never leave the viewport sitting on
         * a position outside its own allowed range.
         */
        void Viewport :: ClampZoomToLimits( void )  {

            unsigned  nMin = m_ZoomBorderLimits.first;
            unsigned  nMax = m_ZoomBorderLimits.second - 1;   // second is the exclusive bound

            if( m_pProps -> nPreferredZoomPos < nMin )
                m_pProps -> nPreferredZoomPos = nMin;
            else if( m_pProps -> nPreferredZoomPos > nMax )
                m_pProps -> nPreferredZoomPos = nMax;

            if( m_pProps -> nCurrentZoomPos < nMin )
                m_pProps -> nCurrentZoomPos = nMin;
            else if( m_pProps -> nCurrentZoomPos > nMax )
                m_pProps -> nCurrentZoomPos = nMax;

            m_pProps -> fZoomFactor = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
        }

        /**
         * Set the preferred zoom to be used when engine apply
         * reset operations.
         * @param nZoomPos The new preferred zoom;
         */
        void Viewport :: SetPreferredZoom( unsigned nZoomPos )  {

            if( ( nZoomPos >= m_ZoomBorderLimits.first ) &&  ( nZoomPos < m_ZoomBorderLimits.second ) )
                m_pProps -> nPreferredZoomPos = nZoomPos;
        }

        /**
         * Set the minimum zoom border limit: the lowest zoom POSITION the
         * viewport may be at (inclusive). Validated against the ABSOLUTE
         * scale [ZOOM_POS_MIN, ZOOM_POS_MAX] - not against the current
         * limits - so a limit can be widened again as well as narrowed. A
         * request that is off the scale, or that would put the minimum
         * above the current maximum, is rejected and changes nothing. The
         * current and preferred positions are clamped into the new range.
         * @param nMinPos The new minimum zoom position;
         */
        void Viewport :: SetMinZoom( unsigned nMinPos )  {

            // second is the exclusive bound, so "nMinPos <= max" is "< second".
            if( ( nMinPos >= ZOOM_POS_MIN ) && ( nMinPos <= ZOOM_POS_MAX ) &&
                ( nMinPos < m_ZoomBorderLimits.second ) )  {
                m_ZoomBorderLimits.first = nMinPos;
                ClampZoomToLimits();
            }
        }

        /**
         * Set the maximum zoom border limit: the highest zoom POSITION the
         * viewport may be at (INCLUSIVE - SetMaxZoom( ZOOM_POS_MAX ) leaves
         * the whole scale usable, consistent with SetZoom and the other
         * position-taking calls). Validated against the ABSOLUTE scale
         * [ZOOM_POS_MIN, ZOOM_POS_MAX], so a limit can be widened again as
         * well as narrowed; a request that is off the scale, or that would
         * put the maximum below the current minimum, is rejected and
         * changes nothing. The current and preferred positions are clamped
         * into the new range.
         * @param nMaxPos The new maximum zoom position;
         */
        void Viewport :: SetMaxZoom( unsigned nMaxPos )  {

            if( ( nMaxPos <= ZOOM_POS_MAX ) && ( nMaxPos >= m_ZoomBorderLimits.first ) )  {
                m_ZoomBorderLimits.second = nMaxPos + 1;    // stored as the exclusive bound
                ClampZoomToLimits();
            }
        }

        /**
         * Set zoom programatically.
         * @param nZoomPos The zoom to be applied;
         */
        void Viewport :: SetZoom( unsigned nZoomPos )  {

            if( ( nZoomPos >= m_ZoomBorderLimits.first ) &&  ( nZoomPos < m_ZoomBorderLimits.second ) )  {
                m_pProps -> nCurrentZoomPos = nZoomPos;
                m_pProps -> fZoomFactor = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
            }
        }

        /**
         * Reset zoom to it's default state.
         */
        void Viewport :: ResetZoom( void )  {

            m_pProps -> nCurrentZoomPos = m_pProps -> nPreferredZoomPos;
            m_pProps -> fZoomFactor     = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
        }

        /**
         * Performs Zoom In effect.
         */
        void Viewport :: ZoomIn( void )  {

            if( ( m_pProps -> nCurrentZoomPos < m_ZoomBorderLimits.second ) &&
                m_pProps -> bEnabledUserZoom )  {
                m_pProps -> nCurrentZoomPos++;

                if( m_pProps -> nCurrentZoomPos == m_ZoomBorderLimits.second )
                    m_pProps -> nCurrentZoomPos--;

                m_pProps -> fZoomFactor = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
            }
        }

        /**
         * Performs Zoom Out effect.
         */
        void Viewport :: ZoomOut( void )  {

            if( ( m_pProps -> nCurrentZoomPos > m_ZoomBorderLimits.first ) &&
                m_pProps -> bEnabledUserZoom )  {
                m_pProps -> nCurrentZoomPos--;
                m_pProps -> fZoomFactor = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
            }
        }

        /**
         * @brief Get the Zoom Factor based on position passed as parameter.
         * Read-only: it reports the factor of the position asked for, and
         * never changes the viewport's own current zoom.
         *
         * @param nZoomPos The zoom position to be retrieved;
         * @return float the zoom factor of that position, or - if the
         * position is outside the allowed range (see SetMinZoom/
         * SetMaxZoom - both bounds are inclusive; by default the whole
         * scale, ZOOM_POS_MIN..ZOOM_POS_MAX) - the factor of the preferred
         * position (see SetPreferredZoom);
         */
        float Viewport :: GetZoomFactor( unsigned int nZoomPos )  {

            if( ( nZoomPos >= m_ZoomBorderLimits.first ) &&  ( nZoomPos < m_ZoomBorderLimits.second ) )  {
                return m_vZoomFactorList[nZoomPos];
            }

            // Outside the allowed range: the preferred position's factor.
            return m_vZoomFactorList[m_pProps -> nPreferredZoomPos];
        }

        /**
         * Set a new pointer to a new parent zoom properties to be used by this
         * ZoomEngine object;
         * @param pProps Pointer to the parent zoom properties to be set;
         */
        void Viewport :: SetZoomPropertiesPtr( stZoomProperties *pProps )  {

            m_pProps = pProps;
        }

        /**
         * Return the reference to internal @link stZoomProperties struct;
         */
        stZoomProperties& Viewport :: GetZoomProperties( void )  {

            return *m_pProps;
        }

        /**
         * Calculates the clipped rectangle area based on position 
         * and viewport boundaries.
         * This bounday is an relative coordinate to viewport starting
         * at (0, 0);
         * @param src Source coordinates;
         * @param dst Reference to destination clipped area;
         */
        bool Viewport :: GetClippedRect( SunLight :: TileMap :: stDimension2D src,
                                        SunLight :: TileMap :: stDimension2D& dst ) {

            float                                fClippingX;
            float                                fClippingY;
            int32_t                              nPos;
            SunLight :: TileMap :: stDimension2D&  vp = GetDimension2D();

            if( ( src.pos.x > vp.size.nWidth ) || ( src.pos.y > vp.size.nHeight) ||
                ( src.pos.x < 0 ) || ( src.pos.y < 0 ) ) {

                if( src.pos.x < 0 ) {
                    nPos = std :: abs( src.pos.x );
                    src.size.nWidth -= ( nPos < src.size.nWidth ? nPos : 
                                        src.size.nWidth );
                    src.pos.x = 0;
                }

                if( src.pos.y < 0 ) {
                    nPos = std :: abs( src.pos.y );
                    src.size.nHeight -= ( nPos < src.size.nHeight ? nPos :
                                        src.size.nHeight );
                    src.pos.y = 0;
                }       
            }

            dst.pos.x        = ( int ) ( ( src.pos.x * m_pProps -> fZoomFactor ) + vp.pos.x );
            dst.pos.y        = ( int ) ( ( src.pos.y * m_pProps -> fZoomFactor ) + vp.pos.y );
            dst.size.nWidth  = ( int ) ( src.size.nWidth * m_pProps -> fZoomFactor );
            dst.size.nHeight = ( int ) ( src.size.nHeight * m_pProps -> fZoomFactor );
            fClippingX       = ( float ) ( dst.pos.x + dst.size.nWidth );
            fClippingY       = ( float ) ( dst.pos.y + dst.size.nHeight );

            if( fClippingX > vp.size.nWidth )  {
                fClippingX-=vp.size.nWidth;

                if( fClippingX > dst.size.nWidth )
                    return false;

                dst.size.nWidth-=( int ) fClippingX;
            }

            if( fClippingY > vp.size.nHeight )  {
                fClippingY-=vp.size.nHeight;

                if( fClippingY > dst.size.nHeight )
                    return false;

                dst.size.nHeight-= ( int ) fClippingY;
            }

            return true;
        }
    }
}
