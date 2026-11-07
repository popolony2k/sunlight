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

#include "renderer/view.h"
#include "renderer/tilemaprenderer.h"
#include <algorithm>
#include <cmath>

#define __VIEW_OPACITY_MIN    0.0f    // a view's opacity: nothing shown
#define __VIEW_OPACITY_MAX    1.0f    // a view's opacity: drawn as always


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Constructor.
         * @param pRenderer The renderer this view belongs to;
         * @param nId The view's id;
         * @param pExistingViewport The renderer's own root viewport, for
         * the default view (which uses it in place - so the renderer's
         * GetViewport() and the default view's are one object); nullptr for
         * any other view, which then owns a fresh Viewport;
         */
        View :: View( TileMapRenderer* pRenderer, int nId, SunLight :: Base :: Viewport* pExistingViewport ) :
                      m_pRenderer( pRenderer ),
                      m_nId( nId )  {

            if( pExistingViewport )  {
                m_pViewport = pExistingViewport;
            }
            else  {
                m_pOwnedViewport = std :: make_unique<SunLight :: Base :: Viewport>();
                m_pViewport      = m_pOwnedViewport.get();
            }

            // The default view (the one wrapping the renderer's own viewport) is drawn first and
            // is not cleared per-view (the frame clear is its clear); the others start on top of it.
            m_bVisible            = true;
            m_nDrawOrder          = ( pExistingViewport ? 0 : nId );
            m_bClear              = true;
            m_bExplicitBackground = false;
            m_Background          = SunLight :: Base :: stColor { 0, 0, 0, 0xFF };
            m_bMaskIsWhitelist    = false;
            m_fOpacity            = __VIEW_OPACITY_MAX;
            m_hTarget             = nullptr;
        }

        /**
         * @brief Nothing to release here: the view's render target (I19a) is the renderer's to
         * release (TileMapRenderer::ReleaseViewTarget, on RemoveView, Stop and its destructor), as
         * only it knows the engine is still drawing.
         */
        View :: ~View( void )  {
        }

        /**
         * Run one operation of the renderer's own with THIS view active:
         * swap the renderer's working state to this view's, call it, swap
         * back. A no-op swap when this view already is the active one.
         */
        template<class Function>
        void View :: Run( Function function )  {

            if( !m_pRenderer )
                return;

            View  *pPrevious = m_pRenderer -> ActivateView( this );

            function();

            m_pRenderer -> ActivateView( pPrevious );
        }

        /**
         * Cut this view loose from its renderer (see @see IView::IsRemoved for what an
         * inert view does). Idempotent.
         *
         * The view's camera is its own (m_Camera), so it keeps its last position and scroll
         * step as it is. The default view also uses the renderer's OWN root Viewport, a member of the
         * renderer's base class that dies with it: so it is replaced by a private copy,
         * built through Viewport's public interface (rectangle, zoom position, preferred
         * zoom, zoom limits, user-zoom flag), which then belongs to the view. Viewport
         * cannot simply be copy-constructed: it points at its own zoom properties, so a
         * copy would keep pointing into the original.
         */
        void View :: Detach( void )  {

            if( !m_pRenderer )
                return;

            if( this == m_pRenderer -> m_pDefaultView.get() )  {
                std :: unique_ptr<SunLight :: Base :: Viewport>  pCopy = std :: make_unique<SunLight :: Base :: Viewport>();
                SunLight :: Base :: stZoomProperties            &props = m_pViewport -> GetZoomProperties();
                unsigned                                         nMinZoom = 0;
                unsigned                                         nMaxZoom = 0;

                m_pViewport -> GetZoomLimits( nMinZoom, nMaxZoom );
                pCopy -> SetDimension2D( m_pViewport -> GetDimension2D() );
                pCopy -> SetMinZoom( nMinZoom );
                pCopy -> SetMaxZoom( nMaxZoom );
                pCopy -> SetEnableUserZoom( props.bEnabledUserZoom );
                pCopy -> SetPreferredZoom( props.nPreferredZoomPos );
                pCopy -> SetZoom( props.nCurrentZoomPos );

                m_bClear          = m_pRenderer -> m_bClearBackground;
                m_pOwnedViewport  = std :: move( pCopy );
                m_pViewport       = m_pOwnedViewport.get();
            }

            m_pRenderer = nullptr;
        }

        /**
         * @brief Whether the view was cut loose from its renderer (see Detach).
         * @return true once RemoveView removed it or its renderer was destroyed;
         */
        bool View :: IsRemoved( void )  {

            return ( m_pRenderer == nullptr );
        }

        /**
         * @brief The id the renderer gave the view.
         * @return 0 for the default view, otherwise the id CreateView returned;
         */
        int View :: GetId( void )  {

            return m_nId;
        }

        /**
         * @brief The view's Viewport: the renderer's own root one for the default view, the view's
         * own (m_pOwnedViewport) for any other or once detached.
         * @return The viewport, valid for as long as the view is;
         */
        SunLight :: Base :: Viewport& View :: GetViewport( void )  {

            return *m_pViewport;
        }

        /**
         * @brief The renderer's own ResetZoom, run with this view active (see Run); nothing once
         * detached.
         */
        void View :: ResetZoom( void )  {

            Run( [this]() { m_pRenderer -> ResetZoom(); } );
        }

        /**
         * @brief The renderer's own ZoomIn, run with this view active; nothing once detached.
         */
        void View :: ZoomIn( void )  {

            Run( [this]() { m_pRenderer -> ZoomIn(); } );
        }

        /**
         * @brief The renderer's own ZoomOut, run with this view active; nothing once detached.
         */
        void View :: ZoomOut( void )  {

            Run( [this]() { m_pRenderer -> ZoomOut(); } );
        }

        /**
         * @brief The renderer's own ResetCamera, run with this view active; nothing once detached.
         */
        void View :: ResetCamera( void )  {

            Run( [this]() { m_pRenderer -> ResetCamera(); } );
        }

        /**
         * @brief The renderer's own MoveCameraUp (one scroll step, clamped), run with this view
         * active; nothing once detached.
         */
        void View :: MoveCameraUp( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraUp(); } );
        }

        /**
         * @brief The renderer's own MoveCameraDown, run with this view active; nothing once detached.
         */
        void View :: MoveCameraDown( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraDown(); } );
        }

        /**
         * @brief The renderer's own MoveCameraLeft, run with this view active; nothing once detached.
         */
        void View :: MoveCameraLeft( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraLeft(); } );
        }

        /**
         * @brief The renderer's own MoveCameraRight, run with this view active; nothing once detached.
         */
        void View :: MoveCameraRight( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraRight(); } );
        }

        /**
         * @brief The renderer's own SetCameraPosition, run with this view active; nothing once
         * detached.
         * @param nX The map x shown at the viewport's left edge;
         * @param nY The map y shown at its top edge;
         */
        void View :: SetCameraPosition( int nX, int nY )  {

            Run( [this, nX, nY]() { m_pRenderer -> SetCameraPosition( nX, nY ); } );
        }

        /**
         * @brief The renderer's own GetCameraPosition, run with this view active; once detached,
         * the last position its camera had.
         * @param nX Receives the x;
         * @param nY Receives the y;
         */
        void View :: GetCameraPosition( int& nX, int& nY )  {

            // Detached: the last position it had.
            if( !m_pRenderer )  {
                m_Camera.GetPosition( nX, nY );

                return;
            }

            Run( [this, &nX, &nY]() { m_pRenderer -> GetCameraPosition( nX, nY ); } );
        }

        /**
         * @brief The renderer's own SetScrollStepSize, run with this view active; nothing once
         * detached.
         * @param nStepWidth The horizontal step, or -1 for the map's tile width;
         * @param nStepHeight The vertical step, or -1 for the map's tile height;
         */
        void View :: SetScrollStepSize( int nStepWidth, int nStepHeight )  {

            Run( [this, nStepWidth, nStepHeight]() { m_pRenderer -> SetScrollStepSize( nStepWidth, nStepHeight ); } );
        }

        /**
         * @brief The renderer's own GetScrollStepSize, run with this view active; once detached,
         * the step its camera kept.
         * @param nStepWidth Receives the horizontal step;
         * @param nStepHeight Receives the vertical step;
         */
        void View :: GetScrollStepSize( int& nStepWidth, int& nStepHeight )  {

            if( !m_pRenderer )  {
                m_Camera.GetScrollStep( nStepWidth, nStepHeight );

                return;
            }

            Run( [this, &nStepWidth, &nStepHeight]() { m_pRenderer -> GetScrollStepSize( nStepWidth, nStepHeight ); } );
        }

        /**
         * @brief Whether the frame draws the view; only stored (the view's own data).
         * @param bVisible false skips the view's pass;
         */
        void View :: SetVisible( bool bVisible )  {

            m_bVisible = bVisible;
        }

        /**
         * @brief Whether the frame draws the view.
         * @return true by default;
         */
        bool View :: GetVisible( void )  {

            return m_bVisible;
        }

        /**
         * @brief The view's place among the passes (ascending, then by id); only stored.
         * @param nOrder The order;
         */
        void View :: SetDrawOrder( int nOrder )  {

            m_nDrawOrder = nOrder;
        }

        /**
         * @brief The view's place among the passes.
         * @return 0 for the default view, its id for another, until set;
         */
        int View :: GetDrawOrder( void )  {

            return m_nDrawOrder;
        }

        /**
         * @brief I19a of the master plan: the view's opacity, which the renderer's frame reads (see
         * TileMapRenderer::BeginViewTarget). Only stored here - the view's own data, so a removed
         * view keeps it too.
         * @param fOpacity 0 (nothing shown) to 1 (drawn as always), clamped; a NaN leaves the opacity as it was;
         */
        void View :: SetOpacity( float fOpacity )  {

            if( !std :: isnan( fOpacity ) )
                m_fOpacity = std :: clamp( fOpacity, __VIEW_OPACITY_MIN, __VIEW_OPACITY_MAX );
        }

        /**
         * @brief The view's opacity.
         * @return 0..1; 1 by default;
         */
        float View :: GetOpacity( void )  {

            return m_fOpacity;
        }

        /**
         * The default view's clear is the frame's own (TileMapRenderer's
         * m_bClearBackground, the very flag SetClearBackground sets), so it
         * is read/written there rather than kept twice.
         * @param bClear true clears the view's rectangle before its pass;
         */
        void View :: SetClearBackground( bool bClear )  {

            if( m_pRenderer && ( this == m_pRenderer -> m_pDefaultView.get() ) )
                m_pRenderer -> SetClearBackground( bClear );
            else
                m_bClear = bClear;
        }

        /**
         * @brief Whether the view's rectangle is cleared before its pass: the renderer's own
         * m_bClearBackground for the default view (see SetClearBackground), the view's flag otherwise.
         * @return true by default;
         */
        bool View :: GetClearBackground( void )  {

            if( m_pRenderer && ( this == m_pRenderer -> m_pDefaultView.get() ) )
                return m_pRenderer -> m_bClearBackground;

            return m_bClear;
        }

        /**
         * @brief The color the view's clear uses from now on, instead of the map's or the window's.
         * @param color The color, alpha included;
         */
        void View :: SetBackgroundColor( const SunLight :: Base :: stColor& color )  {

            m_Background          = color;
            m_bExplicitBackground = true;
        }

        /**
         * @brief The view's clear goes back to the map's background color (the window's with no map).
         */
        void View :: UseMapBackgroundColor( void )  {

            m_bExplicitBackground = false;
        }

        /**
         * @brief Show or hide one layer in this view, by id. m_MaskIds holds the hidden ids, or -
         * after ShowOnlyLayers - the shown ones, so the id is added or taken out accordingly.
         * @param nLayerId The layer's id; one that names no layer is kept and never matched;
         * @param bShow true shows it;
         */
        void View :: ShowLayer( int nLayerId, bool bShow )  {

            // The set holds the hidden ids, or - after ShowOnlyLayers - the shown ones.
            if( bShow == m_bMaskIsWhitelist )
                m_MaskIds.insert( nLayerId );
            else
                m_MaskIds.erase( nLayerId );
        }

        /**
         * @brief Same, by name: the name is looked up in the loaded map (the renderer's GetLayer)
         * and its id masked.
         * @param szLayerName The layer's name;
         * @param bShow true shows it;
         * @return false (nothing changed) with no name, no renderer, no map or no such layer;
         */
        bool View :: ShowLayer( const char* szLayerName, bool bShow )  {

            tmx_layer  *pLayer = ( szLayerName && m_pRenderer && m_pRenderer -> Tmx() ) ? m_pRenderer -> GetLayer( szLayerName ) : nullptr;

            if( !pLayer )
                return false;

            ShowLayer( ( int ) pLayer -> id, bShow );

            return true;
        }

        /**
         * @brief Show only these layers: the mask becomes a list of the shown ids.
         * @param layerIds The ids shown; every other layer, later ones included, is hidden;
         */
        void View :: ShowOnlyLayers( const std :: vector<int>& layerIds )  {

            m_bMaskIsWhitelist = true;
            m_MaskIds          = std :: set<int>( layerIds.begin(), layerIds.end() );
        }

        /**
         * @brief Clear the mask: back to a (now empty) list of hidden ids, so every layer shows.
         */
        void View :: ShowAllLayers( void )  {

            m_bMaskIsWhitelist = false;
            m_MaskIds.clear();
        }

        /**
         * @brief What the mask says about a layer: in the set means shown for a list of shown ids,
         * hidden for a list of hidden ones.
         * @param nLayerId The layer's id;
         * @return true when this view shows it;
         */
        bool View :: IsLayerShown( int nLayerId )  {

            bool  bInSet = ( m_MaskIds.find( nLayerId ) != m_MaskIds.end() );

            return ( bInSet == m_bMaskIsWhitelist );
        }

        /**
         * A zoom position p has the factor ( p + 1 ) x ZOOM_STEP, so the
         * largest p that fits is worked out in whole steps: the number of
         * ZOOM_STEPs the map may be scaled by is floor( viewport size /
         * map size / ZOOM_STEP ) on the tighter axis, computed as an integer
         * division (viewport x steps-per-unit / map size) rather than in
         * floating point, so a view that is exactly half/quarter/... of the
         * map fits at exactly that zoom instead of one step short.
         * @return false (nothing changed) with no map loaded or once detached;
         */
        bool View :: FitToMap( void )  {

            tmx_map  *pMap = ( m_pRenderer ? m_pRenderer -> Tmx() : nullptr );

            if( !pMap )
                return false;

            const long  nStepsPerUnit = ( long ) ( 1.0f / SunLight :: Base :: ZOOM_STEP );
            long        nMapW         = ( long ) pMap -> width  * ( long ) pMap -> tile_width;
            long        nMapH         = ( long ) pMap -> height * ( long ) pMap -> tile_height;

            if( ( nMapW <= 0 ) || ( nMapH <= 0 ) )
                return false;

            SunLight :: Base :: stDimension2D  &dim = m_pViewport -> GetDimension2D();
            long  nStepsW = ( ( long ) dim.size.nWidth  * nStepsPerUnit ) / nMapW;
            long  nStepsH = ( ( long ) dim.size.nHeight * nStepsPerUnit ) / nMapH;
            long  nSteps  = ( nStepsW < nStepsH ? nStepsW : nStepsH );

            if( nSteps < 1 )
                nSteps = 1;

            if( nSteps > ( long ) SunLight :: Base :: ZOOM_POS_COUNT )
                nSteps = ( long ) SunLight :: Base :: ZOOM_POS_COUNT;

            m_pViewport -> SetZoom( ( unsigned ) ( nSteps - 1 ) );
            ResetCamera();

            return true;
        }

        /**
         * @brief The renderer's own TileMapToTileMatrix (a map position has no view, so every view
         * answers the same), run with this view active.
         * @param coord The position, in map pixels;
         * @param pos Receives the cell's row and column;
         * @return Whether the position is in one of the map's cells; false once detached;
         */
        bool View :: TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                          SunLight :: TileMap :: stMatrixPosition& pos )  {

            bool  bResult = false;

            Run( [this, &coord, &pos, &bResult]() { bResult = m_pRenderer -> TileMapToTileMatrix( coord, pos ); } );

            return bResult;
        }
    }
}
