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

#ifndef __IVIEW_H__
#define __IVIEW_H__

#include "base/viewport.h"
#include "tilemap/tilemapdefs.h"


namespace SunLight {
    namespace TileMap  {

        /**
         * @brief One VIEW of the game world: where a camera looks, how it is
         * zoomed, and the rectangle of the render target it is shown in - the
         * unit that lets the same map be shown more than once at the same
         * time (a minimap, picture-in-picture, split-screen).
         *
         * Every renderer has exactly one view to begin with, the DEFAULT
         * view (id 0): everything the renderer itself exposes for camera,
         * zoom and viewport (ITileMap::MoveCameraUp, GetViewport, ZoomIn,
         * SetCameraPosition, ...) is that view's, and keeps meaning exactly
         * what it always did. A view handle offers the same operations
         * addressed to one particular view. See ITileMap::GetDefaultView,
         * CreateView and GetView.
         *
         * What a view owns: its camera position, its Viewport (the visible
         * rectangle [pos, pos + size) and the zoom), and its scroll step.
         * What is shared by every view: the map, the sprites, the
         * collisions, the input - there is one world, seen through several
         * windows.
         *
         * A handle stays valid until its view is removed (the default view
         * is never removed) or the renderer is destroyed.
         */
        class IView  {

            public:

            virtual ~IView( void ) {}

            /**
             * @brief The view's id: 0 for the default view, otherwise the
             * value ITileMap::CreateView returned.
             */
            virtual int GetId( void ) = 0;

            /**
             * @brief This view's viewport: the rectangle of the render
             * target it is drawn into ( [pos, pos + size), see Viewport )
             * and its zoom (position, factor, limits). Set the rectangle
             * with GetViewport().SetDimension2D( ... ) and the zoom with
             * GetViewport().SetZoom( ... ) as for the renderer's own.
             */
            virtual SunLight :: Base :: Viewport& GetViewport( void ) = 0;

            /**
             * @brief Reset zoom to its default state.
             */
            virtual void ResetZoom( void ) = 0;

            /**
             * @brief Performs Zoom In effect.
             */
            virtual void ZoomIn( void ) = 0;

            /**
             * @brief Performs Zoom Out effect.
             */
            virtual void ZoomOut( void ) = 0;

            /**
             * @brief Reset the camera position.
             */
            virtual void ResetCamera( void ) = 0;

            /**
             * @brief Move the camera up / down / left / right by one scroll
             * step, clamped exactly as the renderer's own MoveCamera* are.
             */
            virtual void MoveCameraUp( void ) = 0;
            virtual void MoveCameraDown( void ) = 0;
            virtual void MoveCameraLeft( void ) = 0;
            virtual void MoveCameraRight( void ) = 0;

            /**
             * @brief Put the camera at an absolute position, and read it
             * back (same convention as ITileMap::SetCameraPosition and
             * GetCameraPosition: the map point shown at the viewport's
             * top-left).
             */
            virtual void SetCameraPosition( int nX, int nY ) = 0;
            virtual void GetCameraPosition( int &nX, int &nY ) = 0;

            /**
             * @brief Set this view's scroll step size (see
             * TileMapRenderer::SetScrollStepSize): how far one MoveCamera*
             * call moves it. -1 means "the map's tile size", resolved when a
             * map is loaded.
             */
            virtual void SetScrollStepSize( int nStepWidth, int nStepHeight ) = 0;

            /**
             * @brief Read this view's scroll step size back: what @see
             * SetScrollStepSize last stored, or the map's tile size once a
             * map has been loaded and the step was "-1". Before any map is
             * loaded a step that was never set (or set to -1) still reads
             * -1 - "the map's tile size, not known yet" - so the value is
             * exactly what the view would move by on the next MoveCamera*
             * call, or the -1 that means it is not resolved yet.
             *
             * @param nStepWidth Output parameter receiving the horizontal step;
             * @param nStepHeight Output parameter receiving the vertical step;
             */
            virtual void GetScrollStepSize( int &nStepWidth, int &nStepHeight ) = 0;

            /**
             * @brief Convert a coordinate in THIS view (screen space,
             * through its viewport, zoom and camera) to the tile matrix
             * position under it (see ITileMap::TileMapToTileMatrix, which
             * does the same for the default view).
             */
            virtual bool TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                              SunLight :: TileMap :: stMatrixPosition& pos ) = 0;
        };
    }
}

#endif  /* __IVIEW_H__ */
