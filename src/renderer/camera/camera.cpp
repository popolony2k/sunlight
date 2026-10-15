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

#include "renderer/camera/camera.h"
#include "tilemap/itilemap.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>


#define __CAMERA_ORIGIN                0.0f    // the camera at the map's own unscrolled top-left corner
#define __UNRESOLVED_SCROLL_STEP       -1      // "the map's tile size, once a map is loaded"
#define __ALIGNMENT_MASK               0xFF    // the alignment bits of a MapAlignment value


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief A camera at the origin whose scroll step is still unresolved (see ResolveScrollStep).
         */
        Camera :: Camera( void )  {

            m_Position.x        = __CAMERA_ORIGIN;
            m_Position.y        = __CAMERA_ORIGIN;
            m_nScrollStepWidth  = __UNRESOLVED_SCROLL_STEP;
            m_nScrollStepHeight = __UNRESOLVED_SCROLL_STEP;
        }

        /**
         * @brief The stored position: the negated map point at the viewport's top-left, the offset
         * every map position gets when drawn (see the class comment).
         */
        SunLight :: Base :: stVector2D Camera :: Position( void ) const  {

            return m_Position;
        }

        /**
         * @brief Jump the camera so the given world-space coordinate is
         * shown at the top-left of the viewport. m_Position itself is the
         * negated world coordinate currently shown at the viewport's
         * top-left (confirmed via every Move call and the alignment code,
         * Camera::Align, all of which treat it that way) - callers pass plain,
         * positive world-space coordinates (the same space stObject's
         * x/y and TileMapToTileMatrix's input already use), not this
         * internal negated convention.
         *
         * Unlike MoveCameraUp/Down/Left/Right, this does not clamp to map
         * boundaries - same unconditional-application spirit as
         * ResetCamera(). Move* re-validates every call because it's nudged
         * one small step at a time, every frame; this is a deliberate jump
         * to a caller-supplied target (e.g. an authored map marker via
         * GetObjectByName()) that's assumed already valid - clamping it
         * would work against exactly what it's for, landing precisely on
         * that target instead of wherever the nearest in-bounds point is.
         * The caller is responsible for passing a valid target.
         *
         * @param nX World-space x coordinate (pixels) to show at the
         * viewport's left edge.
         * @param nY World-space y coordinate (pixels) to show at the
         * viewport's top edge.
         */
        void Camera :: SetPosition( int nX, int nY )  {

            m_Position.x = ( float ) -nX;
            m_Position.y = ( float ) -nY;
        }

        /**
         * @brief Read the world-space coordinate currently shown at the
         * top-left of the viewport - the exact inverse of
         * SetCameraPosition (m_Position itself stores the negated
         * coordinate, same convention that method's own comment covers).
         *
         * @param nX Receives the world-space x coordinate (pixels)
         * currently shown at the viewport's left edge.
         * @param nY Receives the world-space y coordinate (pixels)
         * currently shown at the viewport's top edge.
         */
        void Camera :: GetPosition( int& nX, int& nY ) const  {

            nX = ( int ) -m_Position.x;
            nY = ( int ) -m_Position.y;
        }

        /**
         * Reset the camera position to the origin.
         */
        void Camera :: Reset( void )  {

            m_Position.x = 0;
            m_Position.y = 0;
        }

        /**
         * Set the scroll step size.
         * @param nStepWidth The new scroll step size Width
         * (-1 uses the map tile size width, see ResolveScrollStep);
         * @param nStepHeight The new scroll step size Height
         * (-1 uses the map tile size height);
         */
        void Camera :: SetScrollStep( int nStepWidth, int nStepHeight )  {

            m_nScrollStepWidth  = nStepWidth;
            m_nScrollStepHeight = nStepHeight;
        }

        /**
         * Read the scroll step size back: what SetScrollStep stored, or the map tile size once
         * ResolveScrollStep resolved a -1; -1 while it is still unresolved.
         */
        void Camera :: GetScrollStep( int& nStepWidth, int& nStepHeight ) const  {

            nStepWidth  = m_nScrollStepWidth;
            nStepHeight = m_nScrollStepHeight;
        }

        /**
         * @brief A map was loaded: a scroll step still unresolved (-1, each axis on its own) becomes
         * that map's default step (its projection's, MapData::DefaultScrollStep). A step set
         * explicitly is kept.
         * @param defaultStep The loaded map's default step;
         */
        void Camera :: ResolveScrollStep( const SunLight :: Base :: stSize2D& defaultStep )  {

            if( m_nScrollStepWidth < 0 )
                m_nScrollStepWidth = defaultStep.nWidth;

            if( m_nScrollStepHeight < 0 )
                m_nScrollStepHeight = defaultStep.nHeight;
        }

        /**
         * @brief The camera value at which the map's far edge (its bottom or right edge) lands on
         * the viewport's far edge: the furthest MoveUp/MoveLeft may scroll.
         *
         * A map position p is drawn at (p + camera) x zoom + viewport.pos, so the map's far edge
         * (p = nMapSize) reaches the viewport's far edge (viewport.pos + nViewportSize) exactly
         * when camera = nViewportSize / zoom - nMapSize. That is rounded UP to a whole map pixel:
         * the camera stays whole (SetCameraPosition/GetCameraPosition and the scroll steps are
         * whole), and rounding up keeps the map's edge at or past the viewport's - never a gap
         * of the clear colour, at the cost of under one map pixel left hidden at a non-integer
         * zoom. A map smaller than the viewport gives a limit above the origin: no scrolling.
         * @param nViewportSize The viewport's height or width, in screen pixels;
         * @param fZoom The zoom factor;
         * @param nMapSize The map's height or width, in map pixels;
         */
        static float FarCameraLimit( int nViewportSize, float fZoom, int nMapSize )  {

            return std :: ceil( ( ( float ) nViewportSize / fZoom ) - ( float ) nMapSize );
        }

        /**
         * Move the camera up: one scroll step, or less when less is left - the last step stops
         * with the map's bottom edge on the viewport's bottom edge (FarCameraLimit). It used to
         * refuse any step that went past that point, so a map whose height is not a whole number
         * of steps past the viewport's kept its last strip hidden for good (found in the
         * staggered and hexagonal samples: 18 to 55 map pixels at their edges).
         * @param nViewportHeight The viewport's height, in screen pixels;
         * @param fZoom The zoom factor;
         * @param nMapHeight The map's height, in map pixels;
         */
        void Camera :: MoveUp( int nViewportHeight, float fZoom, int nMapHeight )  {

            float  fLimit = FarCameraLimit( nViewportHeight, fZoom, nMapHeight );

            if( fLimit < m_Position.y )
                m_Position.y = std :: max( m_Position.y - m_nScrollStepHeight, fLimit );
        }

        /**
         * Move the camera down: one scroll step, or less when less is left - the last step stops
         * at the origin, the map's own unscrolled alignment (camera.y == 0). It used to refuse a
         * step that went past the origin, which left a camera that MoveUp had stopped short
         * of a whole step (FarCameraLimit) unable to get back there.
         */
        void Camera :: MoveDown( void )  {

            if( m_Position.y < __CAMERA_ORIGIN )
                m_Position.y = std :: min( m_Position.y + m_nScrollStepHeight, __CAMERA_ORIGIN );
        }

        /**
         * Move the camera left: one scroll step, or less when less is left - the last step stops
         * with the map's right edge on the viewport's right edge (see MoveUp).
         * @param nViewportWidth The viewport's width, in screen pixels;
         * @param fZoom The zoom factor;
         * @param nMapWidth The map's width, in map pixels;
         */
        void Camera :: MoveLeft( int nViewportWidth, float fZoom, int nMapWidth )  {

            float  fLimit = FarCameraLimit( nViewportWidth, fZoom, nMapWidth );

            if( fLimit < m_Position.x )
                m_Position.x = std :: max( m_Position.x - m_nScrollStepWidth, fLimit );
        }

        /**
         * Move the camera right: one scroll step, or less when less is left - the last step
         * stops at the origin (see MoveDown).
         */
        void Camera :: MoveRight( void )  {

            if( m_Position.x < __CAMERA_ORIGIN )
                m_Position.x = std :: min( m_Position.x + m_nScrollStepWidth, __CAMERA_ORIGIN );
        }

        /**
         * @brief Place the camera for a map just loaded with the given alignment (moved from
         * TileMapRenderer::LoadMap as it was). Only the axes the alignment names are set:
         * MAP_ALIGNMENT_TOP_LEFT, for one, leaves the position as it is.
         * @param nAlignment A SunLight::TileMap::ITileMap::MapAlignment value;
         * @param nMapWidth The map's width, in map pixels;
         * @param nMapHeight The map's height, in map pixels;
         * @param viewport The viewport's rectangle;
         * @param fZoomFactor The zoom factor;
         */
        void Camera :: Align( int nAlignment, uint16_t nMapWidth, uint16_t nMapHeight,
                              const SunLight :: Base :: stDimension2D& viewport, float fZoomFactor )  {

            const SunLight :: Base :: stDimension2D  &vp = viewport;

            // The viewport's right/bottom EDGES (pos + size). The
            // alignment maths below aligns to the visible rectangle
            // [pos, pos + size) using them - every "- nVpRight + vp.pos.x"
            // is "minus the visible width" - so a viewport keeps the
            // exact alignment it always produced for the same visible area.
            int                                  nVpRight    = vp.pos.x + vp.size.nWidth;
            int                                  nVpBottom   = vp.pos.y + vp.size.nHeight;
            int                                  nVpHeight   = ( nVpBottom > 0 ?
                                                                 std :: abs( ( ( nMapHeight / nVpBottom ) * nVpBottom ) - nMapHeight ) : 0 );


            // Setup alignment
            switch( nAlignment & __ALIGNMENT_MASK )  {
                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_RIGHT :
                    m_Position.x = ( float ) -( nMapWidth - nVpRight + vp.pos.x );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT : // Nothing to do
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_BOTTOM_RIGHT :
                    m_Position.x = ( float ) -( nMapWidth - nVpRight + vp.pos.x );
                    m_Position.y = ( float ) -( nMapHeight - nVpHeight + vp.pos.y );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_BOTTOM_LEFT :
                    m_Position.y = ( float ) -( nMapHeight - nVpBottom + vp.pos.y );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM :
                    m_Position.x = -round( ( ( ( ( nMapWidth * fZoomFactor ) -
                                                nVpRight + vp.pos.x ) / 2 ) / fZoomFactor ) );
                    m_Position.y = ( float ) -( nMapHeight - nVpHeight + vp.pos.y );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_WIDTH_TOP :
                    m_Position.x = -round( ( ( ( ( nMapWidth * fZoomFactor ) -
                                                nVpRight + vp.pos.x ) / 2 ) / fZoomFactor ) );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT :
                    m_Position.y = -round( ( ( ( ( nMapHeight * fZoomFactor ) -
                                                nVpBottom + vp.pos.y ) / 2 ) / fZoomFactor ) );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT :
                    m_Position.x = ( float ) -( nMapWidth - nVpRight + vp.pos.x );
                    m_Position.y = -round( ( ( ( ( nMapHeight * fZoomFactor ) -
                                                nVpBottom + vp.pos.y ) / 2 ) / fZoomFactor ) );
                    break;

                case SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER :
                    m_Position.x = -round( ( ( ( ( nMapWidth * fZoomFactor ) -
                                                nVpRight + vp.pos.x ) / 2 ) / fZoomFactor ) );
                    m_Position.y = -round( ( ( ( ( nMapHeight * fZoomFactor ) -
                                                nVpBottom + vp.pos.y ) / 2 ) / fZoomFactor ) );
                    break;
            }
        }

        /**
         * @brief The camera for a layer whose accumulated parallax factor is fFactorX/fFactorY (L1 of
         * the master plan), placed as Tiled places it; exactly this camera, per axis, when that axis's
         * factor is 1.0.
         *
         * Tiled shifts a parallax layer by (1 - factor) x (view centre + parallax origin), in map pixels
         * (`MapScene::parallaxOffset`, src/tiled/mapscene.cpp: `viewCenter += map->parallaxOrigin()`,
         * then `(1.0 - parallaxFactor) * viewCenter`; the origin is read from the map as it is,
         * `MapReaderPrivate::readMap`). Its manual describes the origin as subtracted ("the distance
         * between the parallax origin and the center of the view"), but the editor draws with the
         * code above, and a map is authored by what the editor shows. This camera is the negated
         * map point at the view's top-left, so the view centre is -Position + half the view, and a
         * map position p is drawn at p + Position + (1 - factor) x (-Position + half view + origin):
         * the returned camera is Position x factor + (1 - factor) x (half view + origin).
         *
         * Before, this anchored at the view's top-left and subtracted the origin
         * ((Position + origin) x factor - origin): the layers did not sit where Tiled shows them,
         * and since the top-left moves with every zoom about a fixed centre, they slid away from
         * the rest of the map while zooming (seen in samples/parallax).
         * @param fFactorX The layer's horizontal factor (its own and its groups', multiplied);
         * @param fFactorY The vertical factor;
         * @param fOriginX The map's parallaxoriginx (0 with no map);
         * @param fOriginY The map's parallaxoriginy;
         * @param fHalfViewWidth Half the view's visible width, in map pixels (its width / zoom / 2);
         * @param fHalfViewHeight Half the view's visible height, in map pixels;
         */
        SunLight :: Base :: stVector2D Camera :: ForParallax( double fFactorX, double fFactorY,
                                                              double fOriginX, double fOriginY,
                                                              double fHalfViewWidth, double fHalfViewHeight ) const  {

            float  fX = ( fFactorX == 1.0 ) ? m_Position.x :
                        ( float ) ( ( m_Position.x * fFactorX ) + ( ( 1.0 - fFactorX ) * ( fHalfViewWidth + fOriginX ) ) );
            float  fY = ( fFactorY == 1.0 ) ? m_Position.y :
                        ( float ) ( ( m_Position.y * fFactorY ) + ( ( 1.0 - fFactorY ) * ( fHalfViewHeight + fOriginY ) ) );

            return { fX, fY };
        }
    }
}
