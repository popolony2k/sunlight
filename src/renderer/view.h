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

#ifndef __VIEW_H__
#define __VIEW_H__

#include <memory>
#include <set>
#include "tilemap/iview.h"
#include "base/primitives.h"


namespace SunLight {
    namespace Renderer  {

        class TileMapRenderer;

        /**
         * @brief The concrete view (see @see SunLight::TileMap::IView).
         *
         * A view is a holder of state - camera position, scroll step and a
         * Viewport - and a handle whose operations RE-USE the renderer's own
         * camera/zoom logic rather than duplicating it: each one briefly
         * makes this view the ACTIVE one (the renderer's working camera,
         * scroll step and root viewport pointer are swapped to this view's
         * state), runs the renderer's own method, and swaps back. So a view
         * always behaves exactly as the renderer itself does, by
         * construction. The default view is the one that is active whenever
         * nothing else is - which is always, outside a draw pass and
         * outside one of these calls.
         *
         * Only TileMapRenderer creates views.
         */
        class View : public SunLight :: TileMap :: IView  {

            friend class TileMapRenderer;

            /**
             * The per-view state the renderer keeps as its "working" copy
             * while the view is active, and that is parked here while it is not.
             */
            struct State  {
                SunLight :: Base :: stVector2D   camera;
                int                              nScrollStepWidth;
                int                              nScrollStepHeight;
            };

            TileMapRenderer                        *m_pRenderer;
            int                                    m_nId;
            SunLight :: Base :: Viewport           *m_pViewport;
            std :: unique_ptr<SunLight :: Base :: Viewport>  m_pOwnedViewport;
            State                                  m_State;

            // Drawing state (see IView). m_bClear / m_bExplicitBackground /
            // m_Background are read for the default view only when it is asked;
            // the frame's own clear is the renderer's (m_bClearBackground).
            bool                                   m_bVisible;
            int                                    m_nDrawOrder;
            bool                                   m_bClear;
            bool                                   m_bExplicitBackground;
            SunLight :: Base :: stColor            m_Background;
            bool                                   m_bMaskIsWhitelist;    // false: m_MaskIds are the HIDDEN layers; true: the SHOWN ones
            std :: set<int>                        m_MaskIds;

            template<class Function>
            void Run( Function function );

            public:

            View( TileMapRenderer *pRenderer, int nId, SunLight :: Base :: Viewport *pExistingViewport );
            virtual ~View( void );

            int GetId( void ) override;
            SunLight :: Base :: Viewport& GetViewport( void ) override;

            void ResetZoom( void ) override;
            void ZoomIn( void ) override;
            void ZoomOut( void ) override;

            void ResetCamera( void ) override;
            void MoveCameraUp( void ) override;
            void MoveCameraDown( void ) override;
            void MoveCameraLeft( void ) override;
            void MoveCameraRight( void ) override;
            void SetCameraPosition( int nX, int nY ) override;
            void GetCameraPosition( int &nX, int &nY ) override;

            void SetScrollStepSize( int nStepWidth, int nStepHeight ) override;
            void GetScrollStepSize( int &nStepWidth, int &nStepHeight ) override;

            void SetVisible( bool bVisible ) override;
            bool GetVisible( void ) override;
            void SetDrawOrder( int nOrder ) override;
            int GetDrawOrder( void ) override;
            void SetClearBackground( bool bClear ) override;
            bool GetClearBackground( void ) override;
            void SetBackgroundColor( const SunLight :: Base :: stColor &color ) override;
            void UseMapBackgroundColor( void ) override;
            void ShowLayer( int nLayerId, bool bShow ) override;
            bool ShowLayer( const char *szLayerName, bool bShow ) override;
            void ShowOnlyLayers( const std :: vector<int> &layerIds ) override;
            void ShowAllLayers( void ) override;
            bool IsLayerShown( int nLayerId ) override;
            bool FitToMap( void ) override;

            bool TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                      SunLight :: TileMap :: stMatrixPosition& pos ) override;
        };
    }
}

#endif  /* __VIEW_H__ */
