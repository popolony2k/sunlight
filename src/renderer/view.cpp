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

        bool View :: TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                          SunLight :: TileMap :: stMatrixPosition& pos )  {

            bool  bResult = false;

            Run( [this, &coord, &pos, &bResult]() { bResult = m_pRenderer -> TileMapToTileMatrix( coord, pos ); } );

            return bResult;
        }
    }
}
