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


namespace SunLight {
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
        View :: View( TileMapRenderer *pRenderer, int nId, SunLight :: Base :: Viewport *pExistingViewport ) :
                      m_pRenderer( pRenderer ),
                      m_nId( nId )  {

            if( pExistingViewport )  {
                m_pViewport = pExistingViewport;
            }
            else  {
                m_pOwnedViewport = std :: make_unique<SunLight :: Base :: Viewport>();
                m_pViewport      = m_pOwnedViewport.get();
            }

            m_State.camera.x         = 0.0f;
            m_State.camera.y         = 0.0f;
            m_State.nScrollStepWidth  = -1;
            m_State.nScrollStepHeight = -1;

            // The default view (the one wrapping the renderer's own viewport) is drawn first and
            // is not cleared per-view (the frame clear is its clear); the others start on top of it.
            m_bVisible            = true;
            m_nDrawOrder          = ( pExistingViewport ? 0 : nId );
            m_bClear              = true;
            m_bExplicitBackground = false;
            m_Background          = SunLight :: Base :: stColor { 0, 0, 0, 0xFF };
            m_bMaskIsWhitelist    = false;
        }

        View :: ~View( void )  {
        }

        /**
         * Run one operation of the renderer's own with THIS view active:
         * swap the renderer's working state to this view's, call it, swap
         * back. A no-op swap when this view already is the active one.
         */
        template<class Function>
        void View :: Run( Function function )  {

            View  *pPrevious = m_pRenderer -> ActivateView( this );

            function();

            m_pRenderer -> ActivateView( pPrevious );
        }

        int View :: GetId( void )  {

            return m_nId;
        }

        SunLight :: Base :: Viewport& View :: GetViewport( void )  {

            return *m_pViewport;
        }

        void View :: ResetZoom( void )  {

            Run( [this]() { m_pRenderer -> ResetZoom(); } );
        }

        void View :: ZoomIn( void )  {

            Run( [this]() { m_pRenderer -> ZoomIn(); } );
        }

        void View :: ZoomOut( void )  {

            Run( [this]() { m_pRenderer -> ZoomOut(); } );
        }

        void View :: ResetCamera( void )  {

            Run( [this]() { m_pRenderer -> ResetCamera(); } );
        }

        void View :: MoveCameraUp( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraUp(); } );
        }

        void View :: MoveCameraDown( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraDown(); } );
        }

        void View :: MoveCameraLeft( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraLeft(); } );
        }

        void View :: MoveCameraRight( void )  {

            Run( [this]() { m_pRenderer -> MoveCameraRight(); } );
        }

        void View :: SetCameraPosition( int nX, int nY )  {

            Run( [this, nX, nY]() { m_pRenderer -> SetCameraPosition( nX, nY ); } );
        }

        void View :: GetCameraPosition( int &nX, int &nY )  {

            Run( [this, &nX, &nY]() { m_pRenderer -> GetCameraPosition( nX, nY ); } );
        }

        void View :: SetScrollStepSize( int nStepWidth, int nStepHeight )  {

            Run( [this, nStepWidth, nStepHeight]() { m_pRenderer -> SetScrollStepSize( nStepWidth, nStepHeight ); } );
        }

        void View :: GetScrollStepSize( int &nStepWidth, int &nStepHeight )  {

            Run( [this, &nStepWidth, &nStepHeight]() { m_pRenderer -> GetScrollStepSize( nStepWidth, nStepHeight ); } );
        }

        void View :: SetVisible( bool bVisible )  {

            m_bVisible = bVisible;
        }

        bool View :: GetVisible( void )  {

            return m_bVisible;
        }

        void View :: SetDrawOrder( int nOrder )  {

            m_nDrawOrder = nOrder;
        }

        int View :: GetDrawOrder( void )  {

            return m_nDrawOrder;
        }

        /**
         * The default view's clear is the frame's own (TileMapRenderer's
         * m_bClearBackground, the very flag SetClearBackground sets), so it
         * is read/written there rather than kept twice.
         */
        void View :: SetClearBackground( bool bClear )  {

            if( this == m_pRenderer -> m_pDefaultView.get() )
                m_pRenderer -> SetClearBackground( bClear );
            else
                m_bClear = bClear;
        }

        bool View :: GetClearBackground( void )  {

            if( this == m_pRenderer -> m_pDefaultView.get() )
                return m_pRenderer -> m_bClearBackground;

            return m_bClear;
        }

        void View :: SetBackgroundColor( const SunLight :: Base :: stColor &color )  {

            m_Background          = color;
            m_bExplicitBackground = true;
        }

        void View :: UseMapBackgroundColor( void )  {

            m_bExplicitBackground = false;
        }

        void View :: ShowLayer( int nLayerId, bool bShow )  {

            // The set holds the hidden ids, or - after ShowOnlyLayers - the shown ones.
            if( bShow == m_bMaskIsWhitelist )
                m_MaskIds.insert( nLayerId );
            else
                m_MaskIds.erase( nLayerId );
        }

        bool View :: ShowLayer( const char *szLayerName, bool bShow )  {

            tmx_layer  *pLayer = ( szLayerName && m_pRenderer -> m_pTmxMap ) ? m_pRenderer -> GetLayer( szLayerName ) : nullptr;

            if( !pLayer )
                return false;

            ShowLayer( ( int ) pLayer -> id, bShow );

            return true;
        }

        void View :: ShowOnlyLayers( const std :: vector<int> &layerIds )  {

            m_bMaskIsWhitelist = true;
            m_MaskIds          = std :: set<int>( layerIds.begin(), layerIds.end() );
        }

        void View :: ShowAllLayers( void )  {

            m_bMaskIsWhitelist = false;
            m_MaskIds.clear();
        }

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
         */
        bool View :: FitToMap( void )  {

            tmx_map  *pMap = m_pRenderer -> m_pTmxMap;

            if( !pMap )
                return false;

            const long  nStepsPerUnit = ( long ) ( 1.0f / SunLight :: Base :: ZOOM_STEP );
            long        nMapW         = ( long ) pMap -> width  * ( long ) pMap -> tile_width;
            long        nMapH         = ( long ) pMap -> height * ( long ) pMap -> tile_height;

            if( ( nMapW <= 0 ) || ( nMapH <= 0 ) )
                return false;

            SunLight :: TileMap :: stDimension2D  &dim = m_pViewport -> GetDimension2D();
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

        bool View :: TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                          SunLight :: TileMap :: stMatrixPosition& pos )  {

            bool  bResult = false;

            Run( [this, &coord, &pos, &bResult]() { bResult = m_pRenderer -> TileMapToTileMatrix( coord, pos ); } );

            return bResult;
        }
    }
}
