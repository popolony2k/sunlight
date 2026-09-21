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

#include <vector>
#include "base/color.h"
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
         * Handles are std::shared_ptr<IView> (the default view's is also
         * reachable as a plain reference, ITileMap::GetDefaultView). A held
         * handle is always SAFE to use: when its view is removed
         * (ITileMap::RemoveView; the default view is never removed) or the
         * renderer is destroyed, the view becomes INERT - @see IsRemoved -
         * instead of dangling: it is no longer drawn, everything that would
         * act on the renderer (camera, zoom steps, scroll step, fitting,
         * converting coordinates) does nothing or answers "no", the things
         * that are just the view's own data (its Viewport, visibility, draw
         * order, background, layer mask) still work, and its Viewport stays
         * valid for as long as the handle is held. The one exception is
         * ITileMap::GetDefaultView, a reference: it lives as long as the
         * renderer, like the renderer's own members.
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
             * @brief Whether this view has been removed from its renderer
             * (ITileMap::RemoveView), or its renderer no longer exists. See
             * the class documentation for what an inert view does.
             */
            virtual bool IsRemoved( void ) = 0;

            /**
             * @brief Whether this view is drawn at all. A hidden view keeps
             * all its state (camera, zoom, mask, ...) and is simply skipped
             * by the frame; the sprites of a layer only shown by hidden
             * views are not advanced either (same rule as a hidden layer).
             * Every view is visible when created.
             */
            virtual void SetVisible( bool bVisible ) = 0;
            virtual bool GetVisible( void ) = 0;

            /**
             * @brief The order views are drawn in: ascending, so a view with
             * a higher order is drawn later - on top of - one with a lower
             * order (painter's order, all views share one render target);
             * views with the same order are drawn in ascending id order. The
             * default view starts at 0 and a view made by CreateView starts
             * at its own id, so by default extra views are on top of the
             * default one, later ones on top of earlier ones.
             */
            virtual void SetDrawOrder( int nOrder ) = 0;
            virtual int GetDrawOrder( void ) = 0;

            /**
             * @brief Whether the view's rectangle is filled with a
             * background color before its layers are drawn, and with which.
             * For a view made by CreateView this is what stops the scene
             * underneath from showing through (a minimap normally wants an
             * opaque backdrop; a translucent one is a color with alpha < 255,
             * blended over what is already there). On by default, with the
             * map's own background color (@see UseMapBackgroundColor).
             *
             * For the DEFAULT view this is the frame's background, cleared
             * once, whole, before any view is drawn - the very same setting
             * as TileMapRenderer::SetClearBackground, on by default, and
             * SetBackgroundColor overrides the map's/window's own color
             * there too.
             */
            virtual void SetClearBackground( bool bClear ) = 0;
            virtual bool GetClearBackground( void ) = 0;
            virtual void SetBackgroundColor( const SunLight :: Base :: stColor &color ) = 0;

            /**
             * @brief Go back to the default background color: the loaded
             * map's own background color, or the renderer's window
             * background color when no map is loaded.
             */
            virtual void UseMapBackgroundColor( void ) = 0;

            /**
             * @brief The LAYER MASK: which of the map's layers this view
             * shows. Every layer is shown by default. A layer is identified
             * by its id (or name) exactly as ITileMap::GetLayer and
             * AddSprite do; a layer that is masked out is not drawn in this
             * view - and, as with a hidden layer, neither are the sprites
             * registered against it (they belong to their layer). A group
             * layer that is masked out hides everything in the group,
             * whatever the children's own settings; a group that is shown
             * still applies the mask to each child. Masking never touches
             * the layer itself (its own visible flag, opacity, ...) or what
             * any other view shows.
             *
             * The mask is kept by id: it may be set before the map is loaded,
             * and an id that names no layer is simply never matched.
             */
            virtual void ShowLayer( int nLayerId, bool bShow ) = 0;

            /**
             * @brief Same, addressing the layer by name.
             * @return false (and nothing changed) when no layer of the loaded
             * map has that name, or no map is loaded.
             */
            virtual bool ShowLayer( const char *szLayerName, bool bShow ) = 0;

            /**
             * @brief Show ONLY the given layers (by id); every other layer -
             * including any added to the map later - is masked out.
             */
            virtual void ShowOnlyLayers( const std :: vector<int> &layerIds ) = 0;

            /**
             * @brief Clear the mask: every layer is shown again.
             */
            virtual void ShowAllLayers( void ) = 0;

            /**
             * @brief Whether this view currently shows the given layer (by
             * id): purely what the mask says - the layer's own visible flag
             * is a separate thing, and applies to every view.
             */
            virtual bool IsLayerShown( int nLayerId ) = 0;

            /**
             * @brief Zoom out (or in) to the largest zoom at which the whole
             * loaded map fits inside this view's viewport, and put the
             * camera at the map's top-left, so that the view shows the
             * entire map - the usual setup for a minimap. The zoom position
             * is clamped to the valid range, so a map too large to fit even
             * at the lowest zoom shows as much as that zoom allows.
             * Nothing else about the view changes.
             *
             * @return false (nothing changed) when no map is loaded.
             */
            virtual bool FitToMap( void ) = 0;

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
