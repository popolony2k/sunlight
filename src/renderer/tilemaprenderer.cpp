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

#include "renderer/tilemaprenderer.h"
#include "engines/enginefactory.h"
#include "window/windowfactory.h"
#include "backends/null/nullbackend.h"
#include "renderer/view.h"
#include "renderer/shape/shapeprimitives.h"
#include "renderer/projection/orthogonalprojection.h"
#include "renderer/projection/isometricprojection.h"
#include "renderer/projection/staggeredprojection.h"
#include "renderer/projection/hexagonalprojection.h"
#include "renderer/map/externalresources.h"
#include "renderer/shape/shapeobjects.h"
#include "renderer/map/maporientation.h"
#include "base/primitives.h"
#include "input/controllermanager.h"
#include "filesystem/filesystemfactory.h"
#include <memory.h>
#include <cstring>
#include "general/clock.h"
#include "general/helper.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

/*
 * Engine defaults (the per-renderer ones - window size, resizeable,
 * exit key, scroll steps, ... - live in RendererConfig).
 */
#define __DEFAULT_FPS                   30
#define __DEFAULT_VISIBLE_STATUS        true
#define __DEFAULT_CLEAR_BACKGROUND      true


/*
 * Engine limits.
 */
#define __MAX_OPACITY_LEVEL            0xFF



namespace SunLight  {
    namespace Renderer  {

        /*
         * Static class initialization.
         */

        /*
         * How many live renderers run on the build's own (non-null) backend.
         * The backend is process-global, so TileMapRenderer::Create uses this
         * to refuse installing the null backend underneath one of them.
         */
        static int  s_nLiveDefaultBackendRenderers = 0;

        /*
         * ViewControlMode (this renderer's own, construction-time config field) and
         * InputDispatchMode (IControllerManager's, F0a of the master plan) are the same two
         * strategies under two names - explicit, not a static_cast relying on the two enums'
         * ordinal values happening to agree.
         */
        static SunLight :: Input :: InputDispatchMode ToInputDispatchMode( SunLight :: Renderer :: ViewControlMode mode )  {

            switch( mode )  {
                case SunLight :: Renderer :: VIEW_CONTROL_MODE_REACTIVE:
                    return SunLight :: Input :: INPUT_DISPATCH_MODE_REACTIVE;

                case SunLight :: Renderer :: VIEW_CONTROL_MODE_ACTIVE:
                default:
                    return SunLight :: Input :: INPUT_DISPATCH_MODE_ACTIVE;
            }
        }


        /**
         * @brief The loaded map's libtmx map, or nullptr when no map is loaded (S1c1: it lives in
         * MapData). The renderer's "is a map loaded" checks read this.
         */
        tmx_map* TileMapRenderer :: Tmx( void ) const  {

            return m_pMap ? m_pMap -> Tmx() : nullptr;
        }

        /**
         * @brief The loaded map's width in map pixels (its projection's MapPixelSize); 0 when no
         * map is loaded.
         */
        uint16_t TileMapRenderer :: MapPixelWidth( void ) const  {

            return m_pMap ? m_pMap -> PixelWidth() : 0;
        }

        /**
         * @brief The loaded map's height in map pixels; 0 when no map is loaded.
         */
        uint16_t TileMapRenderer :: MapPixelHeight( void ) const  {

            return m_pMap ? m_pMap -> PixelHeight() : 0;
        }

        /**
         * Get a pointer to a loaded layer based on it's Id.
         * @param nLayerId The Layer Id to retrieve the layer;
         * @return The layer; nullptr when no map is loaded, or it has no such layer;
         */
        tmx_layer* TileMapRenderer :: GetLayer( int nLayerId )  {

            return m_pMap ? m_pMap -> FindLayer( nLayerId ) : nullptr;
        }

        /**
         * Get a pointer to a loaded layer based on it's name.
         * @param szLayerName The Layer name used to retrieve the layer;
         * @return The layer; nullptr when no map is loaded, or it has no such layer;
         */
        tmx_layer* TileMapRenderer :: GetLayer( const char* szLayerName )  {

            return m_pMap ? m_pMap -> FindLayer( szLayerName ) : nullptr;
        }

        /**
         * Unload all sprites from map and release all allocated data.
         */
        void TileMapRenderer :: UnloadSprites( void )  {

            m_Sprites.UnloadAll();
        }

        /**
         * Convert integer color representation to a SunLight::Base::stColor;
         */
        SunLight :: Base :: stColor TileMapRenderer :: IntToColor( uint32_t color )  {

            return Shape :: ColorOf( color );
        }

        /**
         * @brief The DrawContext of what is being drawn right now (S1a of the master plan): the
         * active view's rectangle and zoom (GetViewport, the view a pass activated) and the camera
         * offset of the layer being drawn (m_EffectiveLayerCameraPos, its parallax applied) - read
         * at the moment of the call, exactly what the shape code read from the renderer itself
         * before it moved into ShapeRenderer.
         */
        DrawContext TileMapRenderer :: CurrentDrawContext( void )  {

            return DrawContext { GetViewport().GetDimension2D(),
                                 GetViewport().GetZoomProperties().fZoomFactor,
                                 m_EffectiveLayerCameraPos };
        }

        /**
         * @brief IShapeSurface::DrawRectangle (M1 of the master plan). Converts fThickness (map
         * units) to a screen-pixel line width via the same Shape::ScreenLineWidth every TMX shape
         * object already uses, then draws through ShapeRenderer (S1a) with the current
         * DrawContext - the very path ObjectRenderer takes for a Tiled rectangle.
         */
        void TileMapRenderer :: DrawRectangle( double fX, double fY, double fWidth, double fHeight,
                                               SunLight :: Base :: stColor color,
                                               double fThickness, double fRotation )  {

            DrawContext  context      = CurrentDrawContext();
            int          nScreenWidth = Shape :: ScreenLineWidth( fThickness, context.fZoom );

            m_ShapeRenderer.DrawRectangle( context, fX, fY, fWidth, fHeight, fRotation, nScreenWidth, color );
        }

        /**
         * @brief IShapeSurface::DrawEllipse (M1). Same conversion-then-delegate shape as
         * DrawRectangle above.
         */
        void TileMapRenderer :: DrawEllipse( double fX, double fY, double fWidth, double fHeight,
                                             SunLight :: Base :: stColor color,
                                             double fThickness, double fRotation )  {

            DrawContext  context      = CurrentDrawContext();
            int          nScreenWidth = Shape :: ScreenLineWidth( fThickness, context.fZoom );

            m_ShapeRenderer.DrawEllipse( context, fX, fY, fWidth, fHeight, fRotation, nScreenWidth, color );
        }

        /**
         * @brief ShapeRenderer's path methods take a point array as libtmx itself stores one
         * (tmx_shape::points), double**, each entry a heap-owned {x, y} pair - not something a
         * mapless public caller should ever have to build by hand. This converts a flat,
         * caller-owned stVector2D array into that same double** shape, in local storage only
         * (nothing escapes this call), for IShapeSurface::DrawPolyline/DrawPolygon (M1) below to
         * then call straight through with.
         * @param pPoints The caller's own points;
         * @param nPointsCount How many;
         * @param storage Backing storage for the converted coordinates - kept alive by the
         * caller for exactly as long as ppRows, which points into it;
         * @param ppRows Receives nPointsCount pointers, each into one pair inside storage;
         */
        void TileMapRenderer :: ToTmxPointArray( const SunLight :: Base :: stVector2D* pPoints,
                                                 int nPointsCount,
                                                 std :: vector<double>& storage,
                                                 std :: vector<double*>& ppRows )  {

            storage.resize( ( size_t ) nPointsCount * 2 );
            ppRows.resize( ( size_t ) nPointsCount );

            for( int nIdx = 0; nIdx < nPointsCount; nIdx++ )  {
                storage[( nIdx * 2 )]     = ( double ) pPoints[nIdx].x;
                storage[( nIdx * 2 ) + 1] = ( double ) pPoints[nIdx].y;
                ppRows[nIdx]              = &storage[nIdx * 2];
            }
        }

        void TileMapRenderer :: DrawPolyline( double fX, double fY,
                                              const SunLight :: Base :: stVector2D* pPoints, int nPointsCount,
                                              SunLight :: Base :: stColor color,
                                              double fThickness, double fRotation )  {

            std :: vector<double>   storage;
            std :: vector<double*>  ppRows;

            ToTmxPointArray( pPoints, nPointsCount, storage, ppRows );

            DrawContext  context      = CurrentDrawContext();
            int          nScreenWidth = Shape :: ScreenLineWidth( fThickness, context.fZoom );

            m_ShapeRenderer.DrawPolyline( context, fX, fY, ppRows.data(), nPointsCount, fRotation, nScreenWidth, color );
        }

        void TileMapRenderer :: DrawPolygon( double fX, double fY,
                                             const SunLight :: Base :: stVector2D* pPoints, int nPointsCount,
                                             SunLight :: Base :: stColor color,
                                             double fThickness, double fRotation )  {

            std :: vector<double>   storage;
            std :: vector<double*>  ppRows;

            ToTmxPointArray( pPoints, nPointsCount, storage, ppRows );

            DrawContext  context      = CurrentDrawContext();
            int          nScreenWidth = Shape :: ScreenLineWidth( fThickness, context.fZoom );

            m_ShapeRenderer.DrawPolygon( context, fX, fY, ppRows.data(), nPointsCount, fRotation, nScreenWidth, color );
        }

        /**
         * @brief IShapeSurface::DrawPoint (M1). ShapeRenderer::DrawPoint already takes its size in
         * map units and converts it internally - this only reorders fX/fY/color/fSize into its
         * order and adds the current DrawContext.
         */
        void TileMapRenderer :: DrawPoint( double fX, double fY, SunLight :: Base :: stColor color, double fSize )  {

            m_ShapeRenderer.DrawPoint( CurrentDrawContext(), fX, fY, fSize, color );
        }

        /**
         * @brief Release every font RegisterFont loaded, and forget the
         * warnings already given. Called by Stop() while the window and its
         * render context still exist, and by the destructor.
         */
        void TileMapRenderer :: ReleaseRegisteredFonts( void )  {

            // Each font is destroyed here, which releases it while the window is open.
            m_Fonts.Clear();
            m_ObjectRenderer.ResetWarnings();   // the renderer's one existing "forget warnings" point
        }

        /**
         * The effective camera for a layer whose own accumulated parallax factor (its own
         * parallaxx/y, already multiplied by every group ancestor's own factor - see
         * DrawAllLayers) is fFactorX/fFactorY (L1 of the master plan).
         *
         * Verified against both Tiled's own documented parallax formula and this project's
         * own camera convention (Camera::Position() is the NEGATED map coordinate shown at the
         * viewport's top-left - GetCameraPosition's own doc comment): the layer's own screen
         * position should be unshifted relative to its authored position exactly when the
         * camera sits at the map's own parallax origin (Tiled's own definition of that origin -
         * "the point parallax is measured from"), and should move at fFactor times the rate
         * the real camera moves otherwise. That is exactly
         * `(Camera::Position() + origin) * factor - origin`, which reduces to Camera::Position() itself when
         * factor is 1 algebraically - except floating point does not guarantee
         * `(a + b) * 1 - b == a` bit-for-bit for arbitrary a/b, so factor 1 is special-cased per
         * axis to return Camera::Position() exactly, unchanged, rather than trust that identity (the
         * master plan's own first L1 test item: factor 1 is byte-identical to today).
         */
        stVector TileMapRenderer :: GetEffectiveCameraForFactor( double fFactorX, double fFactorY ) const  {

            tmx_map  *pTmxMap  = Tmx();
            double   fOriginX = pTmxMap ? pTmxMap -> parallaxoriginx : 0.0;
            double   fOriginY = pTmxMap ? pTmxMap -> parallaxoriginy : 0.0;

            return ActiveCamera().ForParallax( fFactorX, fFactorY, fOriginX, fOriginY );
        }

        /**
         * Draw all layers on screen canvas;
         * @param pLayer Array of layer objects to draw;
         * @param fParentParallaxX The accumulated parallax factor of every group ancestor of
         * pLayer, multiplied together (L1 of the master plan - Tiled's own documented nesting
         * rule: a layer's effective factor is its own, times every parent group's own);
         * @param fParentParallaxY Same as fParentParallaxX, for the vertical axis;
         */
        void TileMapRenderer :: DrawAllLayers( tmx_layer* pLayer, double fParentParallaxX, double fParentParallaxY )  {

            while( pLayer )  {

                /*
                 * The active view's layer mask (IView::ShowLayer): a layer it
                 * does not show is skipped whole - its content, its children
                 * if it is a group, and its sprites, exactly like a layer with
                 * visible="0" but for this view only. m_pActiveView is the
                 * view of the pass being drawn (RenderMap makes it so); an
                 * unmasked view shows every layer.
                 */
                if( pLayer -> visible && m_pActiveView -> IsLayerShown( ( int ) pLayer -> id ) )  {

                    // L1 of the master plan: this layer's own effective camera, set once here
                    // regardless of layer type - LayerRenderer/ObjectRenderer (through the DrawContext) and the
                    // HandleSpriteUpdate call below (for this exact layer, still in scope) all
                    // read m_EffectiveLayerCameraPos instead of the active camera directly, so a
                    // world-space sprite's own GetCameraOffset sees the right value with no
                    // separate per-sprite layer tracking at all.
                    double  fEffectiveParallaxX = pLayer -> parallaxx * fParentParallaxX;
                    double  fEffectiveParallaxY = pLayer -> parallaxy * fParentParallaxY;

                    m_EffectiveLayerCameraPos = GetEffectiveCameraForFactor( fEffectiveParallaxX, fEffectiveParallaxY );

                    switch( pLayer -> type )  {
                        case L_GROUP :
                            DrawAllLayers( pLayer -> content.group_head, fEffectiveParallaxX, fEffectiveParallaxY ); // recursive call
                            break;
                        case L_OBJGR :
                            // S1b: shapes and text through the ObjectRenderer; tile objects come back here.
                            m_ObjectRenderer.Draw( CurrentDrawContext(), pLayer, m_Fonts,
                                                   [this]( tmx_object* pObject, tmx_layer* pObjectLayer, float fOpacity )  {
                                                       m_LayerRenderer.DrawTileObject( CurrentDrawContext(), *m_pMap, pObject, pObjectLayer, fOpacity );
                                                   } );
                            break;
                        case L_IMAGE :
                            // L1 of the master plan: now camera/parallax/zoom/viewport-aware
                            // (see LayerRenderer::DrawImageLayer's own comment) - still default-view-only,
                            // a separate, pre-existing restriction this item does not lift.
                            if( m_pActiveView == m_pDefaultView.get() )
                                m_LayerRenderer.DrawImageLayer( CurrentDrawContext(), pLayer );
                            break;
                        case L_LAYER :
                            m_LayerRenderer.DrawTileLayer( CurrentDrawContext(), *m_pMap, pLayer );
                            break;
                        case L_NONE :
                            // libtmx never gives a layer this kind: it skips any element that is not
                            // a tile layer, object group, image layer or group. The case stays so the
                            // switch covers every enumerator.
                            break;
                    }

                    /*
                     * Runtime-added sprites (AddSprite/m_Sprites, keyed by
                     * this layer's own id) follow a parent-child visibility
                     * hierarchy with their container TMX layer, by design: if
                     * the layer itself is invisible, every sprite registered
                     * against it is hidden too, full stop - a sprite's own
                     * Sprite::SetVisible/GetVisible only ever gets a say once
                     * it's container layer is already visible, same as any
                     * scene-graph-style parent/child visibility. Previously
                     * this only ran for the L_LAYER case (from inside
                     * the old DrawLayer itself), so a sprite registered against an
                     * L_OBJGR layer id was silently never drawn at all, even
                     * when that layer WAS visible - moved here so every layer
                     * type honors the same hierarchy consistently, not just
                     * tile layers. A layer meant to host sprites but show no
                     * static content of it's own (Caravellius's own "(Empty)"
                     * placeholder layers - see that project's own
                     * MAP_AUTHORING.md) must therefore be authored as
                     * genuinely visible (omit `visible`, or `visible="1"`) -
                     * `visible="0"` now means exactly what it says, hiding
                     * that layer's sprites along with it's tiles/objects.
                     */
                    HandleSpriteUpdate( pLayer -> id );
                }

                pLayer = pLayer -> next;
            }
        }

        /**
         * The background color a view's clear uses: its explicit one, or the
         * loaded map's own, or the window background color when there is no map.
         */
        SunLight :: Base :: stColor TileMapRenderer :: BackgroundColorOf( View& view )  {

            if( view.m_bExplicitBackground )
                return view.m_Background;

            if( Tmx() )
                return IntToColor( Tmx() -> backgroundcolor );

            return m_WindowBackgroundColor;
        }

        /**
         * The multi-view part of the frame: one pass per visible view, in
         * draw order (ascending order, then ascending id - std::stable_sort
         * over the default view first and the others in creation order, i.e.
         * ascending id already). Each pass makes its view the active one
         * (so every existing draw routine - tile clipping, zoom, camera,
         * the sprites' own viewport lookup - just sees that view's
         * viewport and camera), optionally fills the view's rectangle with
         * its background (the default view does not: the frame clear done by
         * RenderMap already covers it), and draws the layers the view's mask
         * lets through. Everything goes into the one render target the
         * frame already draws into, so a later view paints over an earlier
         * one where they overlap. The default view is always made active
         * again at the end, ready for input handling and the rest of the
         * frame.
         */
        void TileMapRenderer :: DrawViewPasses( void )  {

            View  *pDefault = m_pDefaultView.get();

            m_Sprites.BeginFrame();
            m_PassViews.clear();

            if( pDefault -> m_bVisible )
                m_PassViews.push_back( pDefault );

            for( std :: shared_ptr<View>& pView : m_ExtraViews )  {
                if( pView -> m_bVisible )
                    m_PassViews.push_back( pView.get() );
            }

            std :: stable_sort( m_PassViews.begin(), m_PassViews.end(),
                                []( View* pLeft, View* pRight ) { return pLeft -> m_nDrawOrder < pRight -> m_nDrawOrder; } );

            for( View* pView : m_PassViews )  {
                ActivateView( pView );

                // The whole pass is cut to its view's rectangle (the engine's clip stack keeps any
                // clip a draw starts inside it), so nothing the pass draws lands outside its view.
                SunLight :: Base :: stDimension2D  &passRect = pView -> m_pViewport -> GetDimension2D();

                SunLight :: Engines :: EngineFactory :: GetEngine().BeginClip( SunLight :: Base :: stRectangle {
                    ( float ) passRect.pos.x, ( float ) passRect.pos.y,
                    ( float ) passRect.size.nWidth, ( float ) passRect.size.nHeight } );

                if( ( pView != pDefault ) && pView -> m_bClear )  {
                    SunLight :: Base :: stDimension2D  &rect = pView -> m_pViewport -> GetDimension2D();

                    SunLight :: Engines :: EngineFactory :: GetEngine().DrawFilledRectangle( rect.pos.x,
                                                                                              rect.pos.y,
                                                                                              rect.size.nWidth,
                                                                                              rect.size.nHeight,
                                                                                              BackgroundColorOf( *pView ) );
                }

                DrawPassContent();

                SunLight :: Engines :: EngineFactory :: GetEngine().EndClip();
            }

            ActivateView( pDefault );

            // Sprites some pass reached but no pass had on screen: advanced once, exactly as a
            // single view would (see SpriteRegistry::UpdateLayer). They draw nothing.
            m_Sprites.EndFrame();
        }

        /**
         * @brief What one pass draws (M2 of the master plan): the map's own layers - each with its
         * sprites, in the map's layer order - when a map is loaded; otherwise every layer id
         * sprites were registered under, in ascending id order (SpriteRegistry::LayerIds), as
         * the only layer order a mapless renderer has. The active view's layer mask applies the
         * same way in both cases (IView::ShowLayer keys on the same layer id), so a view can hide
         * a mapless sprite group exactly as it hides a map layer.
         */
        void TileMapRenderer :: DrawPassContent( void )  {

            if( Tmx() )  {
                DrawAllLayers( Tmx() -> ly_head );
                return;
            }

            for( int nLayerId : m_Sprites.LayerIds() )  {
                if( m_pActiveView -> IsLayerShown( nLayerId ) )
                    HandleSpriteUpdate( nLayerId );
            }
        }

        /**
         * Render all map objects.
         */
        void TileMapRenderer :: RenderMap( void )  {

            if( m_bClearBackground )  {
                SunLight :: Engines :: IEngine  &engine = SunLight :: Engines :: EngineFactory :: GetEngine();

                if( m_bWindowBackgroundSet && Tmx() && !m_pDefaultView -> m_bExplicitBackground )  {
                    // The window area takes the window colour; the viewport is then filled with the map's own.
                    SunLight :: Base :: stDimension2D  &vp = m_pDefaultView -> GetViewport().GetDimension2D();

                    engine.ClearBackground( m_WindowBackgroundColor );
                    engine.DrawFilledRectangle( ( int ) vp.pos.x, ( int ) vp.pos.y,
                                                vp.size.nWidth, vp.size.nHeight,
                                                BackgroundColorOf( *m_pDefaultView ) );
                }
                else
                    engine.ClearBackground( BackgroundColorOf( *m_pDefaultView ) );
            }

            // M2 of the master plan: a frame with no map loaded still draws the sprites registered
            // with AddSprite (which has always accepted a mapless caller) - before M2 this whole
            // block was skipped without a map, so such a sprite's collider was updated every frame
            // but the sprite itself was never drawn.
            if( Tmx() || !m_Sprites.Empty() )  {
                if( m_ExtraViews.empty() )  {
                    /*
                     * The single-view frame - every renderer that never calls CreateView -
                     * is this one call, as it always was (the default view's mask and
                     * visibility are the only new inputs, and default to "show all").
                     */
                    if( m_pDefaultView -> m_bVisible )  {
                        // Study: nothing cuts the single view's draws in software any more, so the viewport
                        // is the clip for the whole frame (the view passes set their own).
                        SunLight :: Engines :: EngineFactory :: GetEngine().BeginClip( SunLight :: Base :: stRectangle {
                            ( float ) m_pDefaultView -> GetViewport().GetDimension2D().pos.x,
                            ( float ) m_pDefaultView -> GetViewport().GetDimension2D().pos.y,
                            ( float ) m_pDefaultView -> GetViewport().GetDimension2D().size.nWidth,
                            ( float ) m_pDefaultView -> GetViewport().GetDimension2D().size.nHeight } );
                        DrawPassContent();
                        SunLight :: Engines :: EngineFactory :: GetEngine().EndClip();
                    }
                }
                else  {
                    DrawViewPasses();
                }
            }

            if( m_bDrawFPS )
                SunLight :: Engines :: EngineFactory :: GetEngine().DrawFPS( 0, 0 );
        }

        /**
         * Fire OnUpdate on every registered ITileMapListener.
         */
        void TileMapRenderer :: HandleTileMapUpdate( void )  {

            for( SunLight :: TileMap :: ITileMapListener* pListener : ListenersSnapshot() )  {
                if( IsListenerRegistered( pListener ) )
                    pListener -> OnUpdate( *this );
            }
        }

        /**
         * The listeners registered right now, copied: what HandleTileMapUpdate and Stop walk. A
         * listener may call AddTileMapListener/RemoveTileMapListener from OnUpdate/OnStop (eg. a
         * stage handing over to the next one), and both mutate m_TileMapListenerList, a std::deque:
         * push_back invalidates every iterator into it and erase can too, so the range-for loops
         * this replaced were undefined behaviour then. The same snapshot CollisionManager::Update
         * already takes of it's collider lists.
         */
        TileMapRenderer :: TileMapListenerList TileMapRenderer :: ListenersSnapshot( void ) const  {

            return m_TileMapListenerList;
        }

        /**
         * Whether pListener is still registered - checked right before each call over a snapshot,
         * so a listener removed earlier in the same walk (and maybe deleted by whoever removed it)
         * is never called. One added during the walk is not in the snapshot: it starts next frame.
         * @param pListener The listener to look for;
         */
        bool TileMapRenderer :: IsListenerRegistered( SunLight :: TileMap :: ITileMapListener* pListener ) const  {

            return std :: find( m_TileMapListenerList.begin(), m_TileMapListenerList.end(), pListener ) != m_TileMapListenerList.end();
        }

        /**
         * Handle user collisions;
         */
        void TileMapRenderer :: HandleUserCollisions( void )  {

            m_CollisionManager.Update();
        }

        /**
         * Handle sprites updates.
         * @param nLayerId The LayerId to update sprites;
         */
        void TileMapRenderer :: HandleSpriteUpdate( int nLayerId )  {

            m_Sprites.UpdateLayer( nLayerId, !m_ExtraViews.empty() );
        }

        /**
         * Copy user layer to tmx layer.
         * @param pTmxLayer Pointer to Tmx that data will be copied to;
         * @param layer User struct whose layer data will be copied from;
         */
        void TileMapRenderer :: CopyLayerToTmx( tmx_layer* pTmxLayer, SunLight :: TileMap :: stLayer& layer )  {

            // nOpacity is 0..__MAX_OPACITY_LEVEL (fully opaque), libtmx's own opacity 0.0..1.0 - the factor
            // LayerRenderer multiplies alpha by. It was "__MAX_OPACITY_LEVEL - nOpacity" here and "opacity +
            // __MAX_OPACITY_LEVEL" in CopyTmxToLayer: not inverses of each other, and neither in 0..1 - an
            // opaque layer read back as 256 and was written back as -1.0, and 255 made it invisible.
            pTmxLayer -> opacity   = ( double ) std :: clamp( layer.nOpacity, 0, __MAX_OPACITY_LEVEL ) / __MAX_OPACITY_LEVEL;
            pTmxLayer -> offsetx   = layer.offset.x;
            pTmxLayer -> offsety   = layer.offset.y;
            pTmxLayer -> visible   = layer.bVisible;
            pTmxLayer -> parallaxx = layer.fParallaxX;
            pTmxLayer -> parallaxy = layer.fParallaxY;
            layer.pLayer           = pTmxLayer;
        }

        /**
         * Copy tmx layer to user layer.
         * @param pTmxLayer Reference to user layer that data will be copied to;
         * @param layer Pointer whose tmx layer data will be copied from;
         */
        void TileMapRenderer :: CopyTmxToLayer( SunLight :: TileMap :: stLayer& layer, tmx_layer* pTmxLayer )  {

            layer.nOpacity   = ( int ) std :: lround( pTmxLayer -> opacity * __MAX_OPACITY_LEVEL );
            layer.offset.x   = pTmxLayer -> offsetx;
            layer.offset.y   = pTmxLayer -> offsety;
            layer.bVisible   = pTmxLayer -> visible;
            layer.fParallaxX = ( float ) pTmxLayer -> parallaxx;
            layer.fParallaxY = ( float ) pTmxLayer -> parallaxy;
            layer.pLayer     = pTmxLayer;
        }

        /**
         * Constructor. Initialize all class data.
         * Kept for source compatibility: builds the equivalent
         * RendererConfig and delegates to the RendererConfig constructor,
         * so both paths produce identical renderers. No default key/camera
         * handler is installed any more (F0b of the master plan) - a game
         * that wants those bindings calls
         * SunLight::Input::DefaultControllerBindings::Install() itself.
         * @param fWidth Screen renderer width;
         * @param fHeight Screen renderer height;
         * @param szTitle Screen renderer title;
         * @param nTargetFps Renderer desired FPS;
         */
        TileMapRenderer :: TileMapRenderer( float fWidth,
                                            float fHeight,
                                            const char* szTitle,
                                            int nTargetFps ) :
                                            TileMapRenderer( [&]( void )  {
                                                SunLight :: Renderer :: RendererConfig  config;

                                                config.fWidth                = fWidth;
                                                config.fHeight               = fHeight;
                                                config.strTitle              = szTitle;
                                                config.nTargetFps            = nTargetFps;

                                                return config;
                                            }() )  {
        }

        /**
         * Constructor. Initialize all class data from a RendererConfig.
         * Assumes a config that passed RendererConfig::Validate() - use
         * @see Create for the checked path. The backend field is not
         * consulted here: the backend in use is whatever the build-time
         * factories hand out (see RendererConfig::IsBackendAvailable for
         * what can be selected).
         * @param config The renderer configuration;
         */
        TileMapRenderer :: TileMapRenderer( const SunLight :: Renderer :: RendererConfig& config ) :
                                            m_CollisionManager( this )  {

            // Select the backend FIRST: everything below (the input
            // handler, anything that touches the engine) must already see
            // the right one. Only the null backend needs installing -
            // DEFAULT/RAYLIB are what the build's own factories already
            // hand out. See NullBackend for the install/restore rules.
            if( config.backend == SunLight :: Renderer :: RENDERER_BACKEND_NULL )
                m_pNullBackend = SunLight :: Backends :: Null :: NullBackend :: Acquire( config.framePacing == SunLight :: Renderer :: FRAME_PACING_REAL_TIME );
            else
                s_nLiveDefaultBackendRenderers++;

            GetDimension2D().size.nWidth  = ( int ) config.fWidth;
            GetDimension2D().size.nHeight = ( int ) config.fHeight;
            m_fWindowWidth                = config.fWidth;
            m_fWindowHeight               = config.fHeight;
            m_nTargetFps                  = config.nTargetFps;
            m_strTitle                    = config.strTitle;
            m_fScreenFadeAlpha            = 0.0f;

            // The default view (id 0) uses this renderer's own root viewport in place and
            // starts out active - see ActivateView.
            m_pDefaultView                = std :: make_shared<View>( this, 0, &GetViewport() );
            m_pActiveView                 = m_pDefaultView.get();
            m_nNextViewId                 = 1;
            m_pRenderTexture              = nullptr;
            m_bIsStarted                  = false;
            m_bExitRequested              = false;
            m_ExitKey                     = config.exitKey;
            m_bWindowResizeable           = config.bResizeable;
            m_bStartFullscreen            = config.bFullscreen;
            m_StartFullscreenStrategy     = config.fullscreenStrategy;
            m_bClearBackground            = __DEFAULT_CLEAR_BACKGROUND;
            m_bDrawFPS                    = config.bDrawFPS;
            m_bStretchToFill              = config.bStretchToFill;
            m_nMaxFrames                  = config.nMaxFrames;
            m_nFramesRun                  = 0;
            m_pDefaultView -> m_Camera.SetScrollStep( config.nScrollStepWidth, config.nScrollStepHeight );
            m_WindowBackgroundColor       = BLACK_COLOR;
            m_bWindowBackgroundSet        = false;
            m_pControllerManager          = std :: make_unique<SunLight :: Input :: ControllerManager>();
            m_pControllerManager -> SetInputDispatchMode( ToInputDispatchMode( config.viewControlMode ) );
            m_TileMapListenerList.clear();
            memset( &m_EffectiveLayerCameraPos, 0, sizeof( m_EffectiveLayerCameraPos ) );
            ResetZoom();
            SetVisible( __DEFAULT_VISIBLE_STATUS );

            // Same order the classic setup used: zoom position first,
            // then the viewport rectangle.
            if( config.nZoomPos )
                GetViewport().SetZoom( *config.nZoomPos );

            if( config.viewport )  {
                GetViewport().SetDimension2D( *config.viewport );
            }
            else  {
                // No viewport chosen: default to the whole render area. The
                // viewport is what LoadMap's alignment math divides by (and
                // what clips drawing), so leaving it zero-sized crashed
                // LoadMap with a divide-by-zero and would have drawn nothing.
                SunLight :: Base :: stDimension2D  fullArea {};

                fullArea.size.nWidth  = ( int ) config.fWidth;
                fullArea.size.nHeight = ( int ) config.fHeight;

                GetViewport().SetDimension2D( fullArea );
            }
        }

        /**
         * Checked creation: validate the config, then build the renderer.
         * @param config The renderer configuration;
         * @param pError Receives the reason on failure (may be null);
         * @return The new renderer, or nullptr if the config is invalid
         * (e.g. a backend that isn't compiled into this build);
         */
        std :: unique_ptr<TileMapRenderer> TileMapRenderer :: Create( const SunLight :: Renderer :: RendererConfig& config,
                                                                      std :: string* pError )  {

            if( !config.Validate( pError ) )
                return nullptr;

            // The backend is process-global (see NullBackend), so the two
            // kinds can't coexist: a renderer of one kind would silently run
            // on the other's engine/window/clock. Refuse, clearly.
            bool  bWantsNull = ( config.backend == SunLight :: Renderer :: RENDERER_BACKEND_NULL );

            if( !bWantsNull && SunLight :: Backends :: Null :: NullBackend :: IsActive() )  {
                if( pError )
                    *pError = "a null-backend renderer is already active in this process; only one backend can be in use at a time";

                return nullptr;
            }

            if( bWantsNull && ( s_nLiveDefaultBackendRenderers > 0 ) )  {
                if( pError )
                    *pError = "a renderer on the default backend is already active in this process; only one backend can be in use at a time";

                return nullptr;
            }

            return std :: make_unique<TileMapRenderer>( config );
        }

        /**
         * Destructor. Finalize all class data.
         */
        TileMapRenderer :: ~TileMapRenderer( void )  {

            /*
             * A view handle may outlive the renderer (it is a shared_ptr the
             * caller owns a reference to): detach every view - the default
             * one included, which is the one that needs its Viewport copied
             * out of the renderer's own base object before that is gone - so
             * that a held handle is inert afterwards instead of pointing
             * into freed memory.
             */
            for( std :: shared_ptr<View>& pView : m_ExtraViews )
                pView -> Detach();

            m_pDefaultView -> Detach();

            ReleaseRegisteredFonts();
            UnloadMap();
            UnloadSprites();   // see Stop(): a mapless renderer's sprite lists are not reached by UnloadMap

            /*
             * Every sprite that still has this renderer as its parent (registered or removed with
             * RemoveSprite - see m_Sprites) is let go: after this destructor its parent pointer
             * would point at a dead renderer, and so would that of its collider and of the canvases that
             * followed it (Sprite::SetParent moves them). A sprite that was destroyed earlier already
             * told us (ChildRemoved) and is not in the set. Iterated over a copy: SetParent calls back
             * into ChildRemoved, which erases from the set.
             */
            for( SunLight :: Sprite :: Sprite* pSprite : m_Sprites.ParentedSprites() )  {
                if( pSprite -> GetParent() == this )
                    pSprite -> SetParent( nullptr );
            }

            m_Sprites.ClearParented();

            if( !m_pNullBackend )
                s_nLiveDefaultBackendRenderers--;
            m_TileMapListenerList.clear();
        }

        /**
         * The camera of the view being drawn: what a world-space canvas adds to a map
         * position to get a position relative to that view - the same offset every map
         * tile gets (see LayerRenderer::DrawTile). nLayerId == -1 (BaseCanvas's own sentinel - a loose
         * canvas, or one never added via AddSprite) gets the true, unscaled Camera::Position(),
         * exactly as before L1 of the master plan. A real layer id gets that layer's own
         * parallax factor applied (GetEffectiveCameraForFactor) - computed fresh here, every
         * call, from the live tmx_layer, NOT from m_EffectiveLayerCameraPos: that member is
         * only ever valid synchronously during DrawAllLayers's own loop (the tile/object/image
         * draw path, which never outlives one call), but GetCameraOffset must answer correctly
         * at any time, including with no render pass having run yet (pinned by
         * tests/test_viewpasses.cpp's own "GetCameraOffset... with one [renderer]" case) - a
         * world-space sprite can ask for its camera offset from application code whenever it
         * likes. A screen-space sprite never calls this at all (TextureCanvas::GetScreenRect
         * only does for a world-space one), so it is entirely unaffected by any layer's
         * parallax factor, by construction. Like GetTile's own parallax handling, this uses
         * the layer's own factor only, not accumulated with any parent group's own - a sprite
         * registered against a layer nested inside a parallax GROUP is the same narrow,
         * documented exception.
         */
        void TileMapRenderer :: GetCameraOffset( float& fX, float& fY, int nLayerId )  {

            tmx_layer  *pTmxLayer = ( nLayerId >= 0 ) ? GetLayer( nLayerId ) : nullptr;
            stVector    offset    = pTmxLayer ? GetEffectiveCameraForFactor( pTmxLayer -> parallaxx, pTmxLayer -> parallaxy ) : ActiveCamera().Position();

            fX = offset.x;
            fY = offset.y;
        }

        /**
         * A child of this renderer - a sprite registered with AddSprite, or
         * removed with RemoveSprite but still parented to us - is being
         * destroyed or moved to another parent (Sprite::~Sprite and
         * Sprite::SetParent call this): forget it, so that neither the frame
         * loop (HandleSpriteUpdate), Stop (UnloadSprites) nor the collision
         * manager is left holding a pointer to it. Layers are searched by
         * pointer identity, so a sprite registered on several layers is
         * removed from all of them; its collider comes out of each layer's
         * collision list too (the manager holds it by handle, so even a
         * stale entry would resolve to nothing, but it is not left behind).
         * @param pChild The child leaving, as its BaseCanvas;
         */
        void TileMapRenderer :: ChildRemoved( SunLight :: Canvas :: BaseCanvas* pChild )  {

            std :: vector<int>              layerIds;
            SunLight :: Sprite :: Sprite  *pSprite = m_Sprites.Forget( pChild, layerIds );

            if( pSprite )  {
                for( int nLayerId : layerIds )
                    m_CollisionManager.RemoveCollider( nLayerId, &pSprite -> GetCollider() );
            }
        }

        /**
         * Add a TileMap listener to internal listener list renderer.
         * All listeners will be called for each update step;
         * @param pListener Pointer to the @ref SunLight::TileMap::ITileMapListener object to add;
         */
        void TileMapRenderer :: AddTileMapListener( SunLight :: TileMap :: ITileMapListener* pListener )  {

            TileMapListenerList :: iterator itItem = std :: find( m_TileMapListenerList.begin(),
                                                                  m_TileMapListenerList.end(),
                                                                  pListener );

            if( itItem == m_TileMapListenerList.end() )
                m_TileMapListenerList.push_back( pListener );
        }

        /**
         * Remove a TileMap listener from internal listener list renderer.
         * @param pListener Pointer to the @ref SunLight::TileMap::ITileMapListener object to remove;
         */
        void TileMapRenderer :: RemoveTileMapListener( SunLight :: TileMap :: ITileMapListener* pListener )  {

            TileMapListenerList :: iterator itItem = std :: find( m_TileMapListenerList.begin(),
                                                                  m_TileMapListenerList.end(),
                                                                  pListener );

            if( itItem != m_TileMapListenerList.end() )
                m_TileMapListenerList.erase( itItem );
        }

        /**
         * Set exit key to leave the renderer when it is in running state;
         * @param key The key code representing the exit key (see
         * SunLight::Input::KeyboardKey);
         * The default exit is ESC Key;
         */
        void TileMapRenderer :: SetExitKey( SunLight :: Input :: KeyboardKey key )  {

            m_ExitKey = key;

            SunLight :: Window :: WindowFactory :: GetWindow().SetExitKey( key );
        }

        /**
         * Query which key currently triggers quit-on-press (see @see
         * SetExitKey) - reads back the locally cached value set above,
         * since raylib's own SetExitKey has no matching getter.
         */
        SunLight :: Input :: KeyboardKey TileMapRenderer :: GetExitKey( void )  {

            return m_ExitKey;
        }

        /**
         * Set window resizeable status. Safe to call both before Start()
         * (the initial state, applied via SetConfigFlags right before
         * InitWindow - see Start()) and while the window is already
         * running (a genuine live toggle, in either direction, via
         * IWindow::SetWindowResizeable - SetConfigFlags itself only takes
         * effect pre-InitWindow, so it can't be reused for that case).
         * @param bResizeable The new resizeable status for window;
         */
        void TileMapRenderer :: SetWindowResizeable( bool bResizeable )  {

            m_bWindowResizeable = bResizeable;

            if( m_bIsStarted )
                SunLight :: Window :: WindowFactory :: GetWindow().SetWindowResizeable( bResizeable );
        }

        /**
         * Query whether the window is currently resizeable (see
         * @see SetWindowResizeable).
         */
        bool TileMapRenderer :: GetWindowResizeable( void )  {

            return m_bWindowResizeable;
        }

        /**
         * Requests the render loop exit on it's next check (see
         * @see IDrawSurface::RequestExit). Purely local state - unlike
         * most of IDrawSurface, there's no IEngine/IWindow call to route this
         * through, since raylib itself has no public way to set it's own
         * close flag, only read it via WindowShouldClose(). Consulted by
         * Run()'s own loop condition.
         */
        void TileMapRenderer :: RequestExit( void )  {

            m_bExitRequested = true;
        }

        /**
         * Query whether @see RequestExit has been called since the
         * renderer was last started.
         */
        bool TileMapRenderer :: GetExitRequested( void )  {

            return m_bExitRequested;
        }

        /**
         * Set the renderer's own target frame rate. Safe to call both
         * before Start() (the initial value - m_nTargetFps is applied via
         * IWindow::SetTargetFPS at the top of Start(), resolving the -1
         * constructor-default sentinel to __DEFAULT_FPS at that point,
         * same shape as SetWindowResizeable's own pre-Start() path) and
         * while the window is already running (a genuine live change, via
         * IWindow::SetTargetFPS - ::SetTargetFPS itself is a plain runtime
         * setter, not a SetConfigFlags-before-InitWindow one-time value).
         * @param nTargetFps The new target frame rate, in frames per second;
         */
        void TileMapRenderer :: SetTargetFPS( int nTargetFps )  {

            m_nTargetFps = nTargetFps;

            if( m_bIsStarted )
                SunLight :: Window :: WindowFactory :: GetWindow().SetTargetFPS( nTargetFps );
        }

        /**
         * Query the currently configured target FPS (see
         * @see SetTargetFPS) - m_nTargetFps itself is resolved from the
         * -1 constructor-default sentinel to it's real applied value
         * (__DEFAULT_FPS) the moment Start() runs, so this never leaks
         * that sentinel back to a caller once the window actually exists.
         */
        int TileMapRenderer :: GetTargetFPS( void )  {

            return m_nTargetFps;
        }

        /**
         * Set the window background color. This color is used when there's no color on any layer or
         * when there's no map loaded.
         * @param color The background color to set;
         */
        void TileMapRenderer :: SetWindowBackgroundColor( SunLight :: Base :: stColor color )  {

            m_WindowBackgroundColor = color;

            // Once set, the area outside the default viewport takes this colour and the
            // viewport takes the map's own (see RenderMap). Until then both use the map's
            // colour, as they always have, so games that never call this are unchanged.
            m_bWindowBackgroundSet   = true;
        }

        /**
         * Set the status of background cleaning for each
         * rendering cycle;
         * @param bStatus The new background clear status settings;
         */
        void TileMapRenderer :: SetClearBackground( bool bStatus )  {

            m_bClearBackground = bStatus;
        }

        /**
         * Whether the frame's whole-window background clear is enabled (see
         * @see SetClearBackground) - the default view's own "clear" flag.
         */
        bool TileMapRenderer :: GetClearBackground( void )  {

            return m_bClearBackground;
        }

        /**
         * Enable/disable draw FPS information on top left corner of screen
         * @param bDrawFPS The new draw FPS status;
         */
        void TileMapRenderer :: SetDrawFPS( bool bDrawFPS )  {

            m_bDrawFPS = bDrawFPS;
        }

        /**
         * Query whether the FPS counter is currently being drawn (see
         * @see SetDrawFPS).
         */
        bool TileMapRenderer :: GetDrawFPS( void )  {

            return m_bDrawFPS;
        }

        /**
         * Choose how the fixed-internal-resolution render target is
         * blitted onto the real window/screen when their sizes differ -
         * see IDrawSurface::SetStretchToFill's own doc comment for the full
         * behavior. A plain flag read every frame in Run()'s own blit
         * math, same as m_bDrawFPS - no immediate side effect needed here
         * (unlike m_bWindowResizeable, which pokes IWindow right away if
         * the window's already running).
         * @param bStretchToFill The new stretch-to-fill status;
         */
        void TileMapRenderer :: SetStretchToFill( bool bStretchToFill )  {

            m_bStretchToFill = bStretchToFill;
        }

        /**
         * Query whether the render target is currently being stretched to
         * fill (see @see SetStretchToFill).
         */
        bool TileMapRenderer :: GetStretchToFill( void )  {

            return m_bStretchToFill;
        }

        /**
         * Enter or leave fullscreen (see @see IDrawSurface::SetFullscreen).
         * Routed through IWindow rather than raylib directly, same as
         * every other window primitive TileMapRenderer uses.
         * @param bFullscreen true to enter fullscreen, false for windowed;
         * @param strategy Which fullscreen strategy to use when entering
         * fullscreen (ignored when bFullscreen is false);
         */
        void TileMapRenderer :: SetFullscreen( bool bFullscreen, SunLight :: Window :: FullscreenStrategy strategy )  {

            SunLight :: Window :: WindowFactory :: GetWindow().SetFullscreen( bFullscreen, strategy );
        }

        /**
         * Query whether the window is currently fullscreen (see
         * @see SetFullscreen).
         */
        bool TileMapRenderer :: GetFullscreen( void )  {

            return SunLight :: Window :: WindowFactory :: GetWindow().GetFullscreen();
        }

        /**
         * The fullscreen strategy in effect (see @see
         * IDrawSurface::GetFullscreenStrategy), straight from IWindow.
         */
        SunLight :: Window :: FullscreenStrategy TileMapRenderer :: GetFullscreenStrategy( void )  {

            return SunLight :: Window :: WindowFactory :: GetWindow().GetFullscreenStrategy();
        }

        /**
         * Query real, wall-clock elapsed time in seconds (see
         * @see IDrawSurface::GetElapsedTime). Forwarded straight to
         * IWindow, same as GetFullscreen above.
         */
        double TileMapRenderer :: GetElapsedTime( void )  {

            return SunLight :: Window :: WindowFactory :: GetWindow().GetElapsedTime();
        }

        /**
         * Make a view the ACTIVE one: park the current active view's working state (camera,
         * scroll step) in it, load the new view's into the renderer's working members, and point
         * the root viewport at the new view's - after which every piece of existing camera, zoom
         * and drawing code, and every sprite (which finds its viewport through this renderer, its
         * parent), works on that view. Returns the previously active view so the caller can put it
         * back; activating the already-active view changes nothing.
         * @param pView The view to activate;
         */
        View* TileMapRenderer :: ActivateView( View* pView )  {

            View  *pPrevious = m_pActiveView;

            if( ( pView == nullptr ) || ( pView == pPrevious ) )
                return pPrevious;

            SetViewport( pView -> m_pViewport );
            m_pActiveView = pView;

            return pPrevious;
        }

        /**
         * @brief The active view's camera (S1e of the master plan): the one every camera method of
         * the renderer reads and moves - the default view's, outside a draw pass and outside a
         * View's own operation (see ActivateView).
         */
        Camera& TileMapRenderer :: ActiveCamera( void ) const  {

            return m_pActiveView -> m_Camera;
        }

        /**
         * The default view (id 0) - see @see SunLight::TileMap::ITileMap::GetDefaultView.
         */
        SunLight :: TileMap :: IView& TileMapRenderer :: GetDefaultView( void )  {

            return *m_pDefaultView;
        }

        /**
         * Create an additional view over the same world. The renderer keeps
         * one reference to it (so it is drawn whatever the caller does with
         * the returned handle) and the caller gets the other: see
         * @see SunLight::TileMap::ITileMap::CreateView. It starts with its camera at
         * the origin, the default zoom, and a scroll step of the loaded map's tile size (or
         * "the tile size of whatever map is loaded next" if none is yet).
         * @param rect The rectangle of the render target it is shown in - [pos, pos + size);
         */
        std :: shared_ptr<SunLight :: TileMap :: IView> TileMapRenderer :: CreateView( const SunLight :: Base :: stDimension2D& rect )  {

            std :: shared_ptr<View>  pView = std :: make_shared<View>( this, m_nNextViewId, nullptr );

            pView -> m_pViewport -> SetDimension2D( rect );

            if( m_pMap )  {
                SunLight :: Base :: stSize2D  step = m_pMap -> DefaultScrollStep();

                pView -> m_Camera.SetScrollStep( step.nWidth, step.nHeight );
            }

            m_ExtraViews.push_back( pView );
            m_nNextViewId++;

            return pView;
        }

        /**
         * Get a view by id (0 = the default view), or an empty pointer.
         */
        std :: shared_ptr<SunLight :: TileMap :: IView> TileMapRenderer :: GetView( int nViewId )  {

            if( nViewId == 0 )
                return m_pDefaultView;

            for( std :: shared_ptr<View>& pView : m_ExtraViews )  {
                if( pView -> GetId() == nViewId )
                    return pView;
            }

            return nullptr;
        }

        /**
         * Remove a view created by CreateView. The default view can't be
         * removed. The view is taken out of the frame first and only then
         * detached (see View::Detach), so a caller still holding its handle
         * is left with an inert - never a dangling - view.
         */
        bool TileMapRenderer :: RemoveView( int nViewId )  {

            if( nViewId == 0 )
                return false;

            for( std :: vector<std :: shared_ptr<View>> :: iterator itView = m_ExtraViews.begin(); itView != m_ExtraViews.end(); itView++ )  {
                if( ( * itView ) -> GetId() == nViewId )  {
                    // Never leave a removed view as the active one.
                    if( m_pActiveView == itView -> get() )
                        ActivateView( m_pDefaultView.get() );

                    // Keep it alive across the erase: Detach is called on the (possibly last) reference.
                    std :: shared_ptr<View>  pRemoved = * itView;

                    m_ExtraViews.erase( itView );
                    pRemoved -> Detach();

                    return true;
                }
            }

            return false;
        }

        /**
         * Same, by handle. Compared by object, not just by id: ids are per
         * renderer, so another renderer's view 1 is not this renderer's view 1.
         */
        bool TileMapRenderer :: RemoveView( const std :: shared_ptr<SunLight :: TileMap :: IView>& pView )  {

            if( !pView )
                return false;

            std :: shared_ptr<SunLight :: TileMap :: IView>  pOwn = GetView( pView -> GetId() );

            if( !pOwn || ( pOwn.get() != pView.get() ) )
                return false;

            return RemoveView( pView -> GetId() );
        }

        /**
         * The number of views, the default view included.
         */
        int TileMapRenderer :: GetViewCount( void )  {

            return ( int ) m_ExtraViews.size() + 1;
        }

        /**
         * Set the view port control mode;
         * Viewport control mode (active and reactive)
         * Active, the view port reacts to a single key pressing continuously;
         * Reactive, the view port reacts only for each key pressing;
         * @param mode The new view control mode;
         */
        void TileMapRenderer :: SetViewControlMode( SunLight :: Renderer :: ViewControlMode mode )  {

            m_pControllerManager -> SetInputDispatchMode( ToInputDispatchMode( mode ) );
        }

        /**
         * Set the default scroll step size.
         * @param nStepWidth The new scroll step size Width
         * (-1 uses the map tile size width);
         * @param nStepHeight The new scroll step size Height
         * (-1 uses the map tile size height);
         */
        void TileMapRenderer :: SetScrollStepSize( int nStepWidth, int nStepHeight )  {

            ActiveCamera().SetScrollStep( nStepWidth, nStepHeight );
        }

        /**
         * Read the scroll step size back (the active view's - the renderer's
         * own for the default view).
         * @param nStepWidth Receives the horizontal step: what
         * @see SetScrollStepSize stored, or the map tile width once a map is
         * loaded and the step was -1 (LoadMap resolves it); -1 while it is
         * still unresolved;
         * @param nStepHeight Receives the vertical step, same rule;
         */
        void TileMapRenderer :: GetScrollStepSize( int& nStepWidth, int& nStepHeight )  {

            ActiveCamera().GetScrollStep( nStepWidth, nStepHeight );
        }

        /**
         * @brief Set the Game Pad Id that will be handled by this renderer;
         *
         * @param nGamePadId The game pad id to set;
         */
        void TileMapRenderer :: AddGamePad( int nGamePadId )  {

            m_pControllerManager -> AddGamePad( nGamePadId );
        }

        /**
         * @brief Set the User Key Event Handler callback routine.
         *
         * @param evt The event code to call related routine;
         * @param handler The callback routine that will be called for key event;
         */
        void TileMapRenderer :: SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt,
                                                        SunLight :: Input :: INPUT_EVENT_HANDLER handler )  {

            m_pControllerManager -> SetUserKeyEventHandler( evt, handler );
        }

        /**
          * @brief Set the User Game Pad button event Handler callback routine.
          *
          * @param evt The event code to call related routine;
          * @param handler The callback routine that will be called for key event;
          */
        void TileMapRenderer :: SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt,
                                                            SunLight :: Input :: INPUT_EVENT_HANDLER handler )  {

            m_pControllerManager -> SetUserGamePadEventHandler( evt, handler );
        }

        /**
         * @brief Get the Input Handler object
         *
         * @return The reference to IInputHandler object;
         */
        SunLight :: Input :: IInputHandler& TileMapRenderer :: GetInputHandler( void )  {

            return m_pControllerManager -> GetInputHandler();
        }

        /**
         * @brief Get the controller manager these input methods delegate to (F0a of the master
         * plan), for direct access - the same shape as GetCollisionManager().
         * @return A reference to the IControllerManager object;
         */
        SunLight :: Input :: IControllerManager& TileMapRenderer :: GetControllerManager( void )  {

            return *m_pControllerManager;
        }

        /**
         * Reset zoom to it's default state.
         */
        void TileMapRenderer :: ResetZoom( void )  {

            GetViewport().ResetZoom();
        }

        /**
         * Performs Zoom In effect.
         */
        void TileMapRenderer :: ZoomIn( void )  {

            GetViewport().ZoomIn();
        }

        /**
         * Performs Zoom Out effect.
         */
        void TileMapRenderer :: ZoomOut( void )  {

            GetViewport().ZoomOut();
        }

        /**
         * Reset the camera position.
         */
        void TileMapRenderer :: ResetCamera( void )  {

            ActiveCamera().Reset();
        }

        /**
         * Move view camera up: one scroll step, or less when less is left - the last step stops
         * with the map's bottom edge on the viewport's bottom edge (Camera::MoveUp). Does nothing
         * while no map is loaded: the limit is worked out from the map's extent.
         */
        void TileMapRenderer :: MoveCameraUp( void )  {

            if( !Tmx() )
                return;

            SunLight :: Base :: Viewport  &vp = GetViewport();

            ActiveCamera().MoveUp( vp.GetDimension2D().size.nHeight, vp.GetZoomProperties().fZoomFactor, MapPixelHeight() );
        }

        /**
         * Move view camera down: one scroll step, or less when less is left - the last step stops
         * at the origin (Camera::MoveDown).
         */
        void TileMapRenderer :: MoveCameraDown( void )  {

            ActiveCamera().MoveDown();
        }

        /**
         * Move view camera left: one scroll step, or less when less is left - the last step stops
         * with the map's right edge on the viewport's right edge (Camera::MoveLeft). Does nothing
         * while no map is loaded.
         */
        void TileMapRenderer :: MoveCameraLeft( void )  {

            if( !Tmx() )
                return;

            SunLight :: Base :: Viewport  &vp = GetViewport();

            ActiveCamera().MoveLeft( vp.GetDimension2D().size.nWidth, vp.GetZoomProperties().fZoomFactor, MapPixelWidth() );
        }

        /**
         * Move view camera right: one scroll step, or less when less is left - the last step
         * stops at the origin (Camera::MoveRight).
         */
        void TileMapRenderer :: MoveCameraRight( void )  {

            ActiveCamera().MoveRight();
        }

        /**
         * @brief Jump the camera so the given world-space coordinate is
         * shown at the top-left of the viewport. Camera::Position() itself is the
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
        void TileMapRenderer :: SetCameraPosition( int nX, int nY )  {

            ActiveCamera().SetPosition( nX, nY );
        }

        /**
         * @brief Read the world-space coordinate currently shown at the
         * top-left of the viewport - the exact inverse of
         * SetCameraPosition (Camera::Position() itself stores the negated
         * coordinate, same convention that method's own comment covers).
         *
         * @param nX Receives the world-space x coordinate (pixels)
         * currently shown at the viewport's left edge.
         * @param nY Receives the world-space y coordinate (pixels)
         * currently shown at the viewport's top edge.
         */
        void TileMapRenderer :: GetCameraPosition( int& nX, int& nY )  {

            ActiveCamera().GetPosition( nX, nY );
        }

        /**
         * @brief Set the application window's title, replacing whatever
         * title it was created with (see @see IDrawSurface::SetWindowTitle).
         * Routed through IWindow rather than raylib directly, same as
         * every other window primitive TileMapRenderer uses -
         * raylib's own SetWindowTitle acts on the live window handle, so
         * this only forwards the call once the window actually exists,
         * same guard as SetWindowResizeable.
         *
         * @param strTitle The new window title;
         */
        void TileMapRenderer :: SetWindowTitle( const std :: string& strTitle )  {

            m_strTitle = strTitle;

            if( m_bIsStarted )
                SunLight :: Window :: WindowFactory :: GetWindow().SetWindowTitle( m_strTitle.c_str() );
        }

        /**
         * @brief Set a whole-screen fade overlay. Stored here and drawn by
         * @see Run() as the very last thing each frame, on top of whatever
         * else was rendered - see Run()'s own comment for why.
         */
        void TileMapRenderer :: SetScreenFade( float fAlpha )  {

            m_fScreenFadeAlpha = fAlpha;
        }

        /**
         * @brief Load (or replace) the font used by @see DrawText (see
         * @see IDrawSurface::SetFont). A thin pass-through to IEngine - unlike
         * @see SetWindowResizeable, there's no pre-Start() path to support
         * here (loading a font needs the render context IEngine's backend
         * already requires), so this is always forwarded directly.
         */
        bool TileMapRenderer :: SetFont( const char* szFilePath )  {

            return SunLight :: Engines :: EngineFactory :: GetEngine().SetFont( szFilePath );
        }

        /**
         * @brief Register a font for text objects (see TextObjectRenderer): a
         * text object draws with the font whose family and bold/italic flags
         * match its own. Loads through the engine's LoadFont, which does not
         * change the active font (SetFont).
         *
         * Needs a started renderer: loading a font needs the window's render
         * context, which Start() creates. The font is released by Stop(),
         * before the window closes. Registering the same family and style
         * again replaces the earlier font, so a game can swap a font without
         * stopping the renderer.
         *
         * @param szFamily Family name, as Tiled's fontfamily spells it;
         * @param szFilePath Font file to load (same formats as SetFont);
         * @param bBold Whether this is the bold face of the family;
         * @param bItalic Whether this is the italic face of the family;
         * @return true if the font loaded and is registered, false otherwise
         * (nothing is registered, and an earlier font for the same family and
         * style is kept).
         */
        bool TileMapRenderer :: RegisterFont( const char* szFamily, const char* szFilePath, bool bBold, bool bItalic )  {

            // Loading a font needs the render context, which Start() creates.
            if( !m_bIsStarted )
                return false;

            return m_Fonts.Register( szFamily, szFilePath, bBold, bItalic );
        }

        /**
         * @brief Draw a line of text in screen space (see
         * @see IDrawSurface::DrawText). A thin pass-through to IEngine, same
         * shape as @see SetFont.
         */
        void TileMapRenderer :: DrawText( const char* szText,
                                          int nPosX,
                                          int nPosY,
                                          int nFontSize,
                                          unsigned char nRed,
                                          unsigned char nGreen,
                                          unsigned char nBlue,
                                          unsigned char nAlpha )  {

            SunLight :: Engines :: EngineFactory :: GetEngine().DrawText( szText, nPosX, nPosY, nFontSize,
                                          SunLight :: Base :: stColor { nRed, nGreen, nBlue, nAlpha } );
        }

        /**
         * @brief Draw a filled rectangle in screen space (see
         * @see IDrawSurface::DrawFilledRectangle). A thin pass-through to
         * IEngine, same shape as @see DrawText.
         */
        void TileMapRenderer :: DrawFilledRectangle( int nPosX,
                                                     int nPosY,
                                                     int nWidth,
                                                     int nHeight,
                                                     unsigned char nRed,
                                                     unsigned char nGreen,
                                                     unsigned char nBlue,
                                                     unsigned char nAlpha )  {

            SunLight :: Engines :: EngineFactory :: GetEngine().DrawFilledRectangle( nPosX, nPosY, nWidth, nHeight,
                                          SunLight :: Base :: stColor { nRed, nGreen, nBlue, nAlpha } );
        }

        /**
         * @brief Measure a line of text's rendered width (see
         * @see IDrawSurface::MeasureText). A thin pass-through to IEngine,
         * same shape as @see DrawText.
         */
        int TileMapRenderer :: MeasureText( const char* szText, int nFontSize )  {

            return SunLight :: Engines :: EngineFactory :: GetEngine().MeasureText( szText, nFontSize );
        }

        /**
         * @brief Return the engine's own fixed design/render resolution
         * width (see @see IDrawSurface::GetWindowWidth). Pure state - m_fWindowWidth
         * is set once, at construction, and never touched again (in
         * particular, never updated on a live window resize - see it's
         * own doc comment on ITileMap for why that's deliberate).
         */
        int TileMapRenderer :: GetWindowWidth( void )  {

            return ( int ) m_fWindowWidth;
        }

        /**
         * @brief Return the engine's own fixed design/render resolution
         * height (see @see GetWindowWidth).
         */
        int TileMapRenderer :: GetWindowHeight( void )  {

            return ( int ) m_fWindowHeight;
        }

        /**
         * Set layer parameters.
         * @param nLayerId The layer id to set layer parameters;
         * @param layer reference to layer parameters structure to set;
         */
        bool TileMapRenderer :: SetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer )  {

            tmx_layer* pTmxLayer = GetLayer( nLayerId );

            if( pTmxLayer )  {
                CopyLayerToTmx( pTmxLayer, layer );
                return true;
            }

            return false;
        }

        /**
         * Set layer parameters.
         * @param szLayerName The layer name to set layer parameters;
         * @param layer reference to layer parameters structure to set;
         */
        bool TileMapRenderer :: SetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer )  {

            tmx_layer* pTmxLayer = GetLayer( szLayerName );

            if( pTmxLayer )  {
                CopyLayerToTmx( pTmxLayer, layer );
                return true;
            }

            return false;
        }

        /**
         * Get the layer parameters.
         * @param nLayerId The layer id to get layer parameters;
         * @param layer reference to layer parameters structure to get;
         */
        bool TileMapRenderer :: GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer )  {

            tmx_layer* pTmxLayer = GetLayer( nLayerId );

            if( pTmxLayer )  {
                CopyTmxToLayer( layer, pTmxLayer );
                return true;
            }

            return false;
        }

        /**
         * Get the layer parameters.
         * @param szLayerName The layer name to get layer parameters;
         * @param layer reference to layer parameters structure to get;
         */
        bool TileMapRenderer :: GetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer )  {

            tmx_layer* pTmxLayer = GetLayer( szLayerName );

            if( pTmxLayer )  {
                CopyTmxToLayer( layer, pTmxLayer );
                return true;
            }

            return false;
        }

        /**
         * Set a layer's own parallax factor in code (L1 of the master plan), overriding
         * whatever the TMX file itself declared for it.
         * @param nLayerId The layer id to set the parallax factor for;
         * @param fFactorX The horizontal parallax factor (1.0 = no parallax);
         * @param fFactorY The vertical parallax factor (1.0 = no parallax);
         */
        bool TileMapRenderer :: SetLayerParallax( int nLayerId, float fFactorX, float fFactorY )  {

            tmx_layer* pTmxLayer = GetLayer( nLayerId );

            if( pTmxLayer )  {
                pTmxLayer -> parallaxx = fFactorX;
                pTmxLayer -> parallaxy = fFactorY;
                return true;
            }

            return false;
        }

        /**
         * Set a layer's own parallax factor in code (L1 of the master plan), overriding
         * whatever the TMX file itself declared for it.
         * @param szLayerName The layer name to set the parallax factor for;
         * @param fFactorX The horizontal parallax factor (1.0 = no parallax);
         * @param fFactorY The vertical parallax factor (1.0 = no parallax);
         */
        bool TileMapRenderer :: SetLayerParallax( const char* szLayerName, float fFactorX, float fFactorY )  {

            tmx_layer* pTmxLayer = GetLayer( szLayerName );

            if( pTmxLayer )  {
                pTmxLayer -> parallaxx = fFactorX;
                pTmxLayer -> parallaxy = fFactorY;
                return true;
            }

            return false;
        }

        /**
         * Get a tile based row and column on specified map layer;
         * @param pos The tile position on layer map.
         * @param layer Reference to the layer whose tile will be
         * retrieved;
         * @param tile reference to @ref SunLight::TileMap::stTile object to receive
         * tile information;
         */
        bool TileMapRenderer :: GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                                         const SunLight :: TileMap :: stLayer& layer,
                                         SunLight :: TileMap :: stTile& tile )  {

            // The gid and tile (S1c1: MapData::TileAt, which also refuses a position off the map - A6 - and
            // a gid past libtmx's tiles[] table); the screen rectangle below needs the view, so stays here.
            if( !m_pMap -> TileAt( pos, layer.pLayer, tile ) )
                return false;

            // L1 of the master plan: layer.pLayer's own parallax factor (not accumulated with
            // any parent group's own - this call has no ancestor chain to walk, only the one
            // tmx_layer pointer the caller passed), so a non-nested parallax layer's picked
            // tile position is correct; a tile layer nested inside a parallax-factored GROUP
            // is a narrow, documented exception (LayerRenderer::DrawTileLayer's render path does not depend
            // on this field at all - only external pick/collision callers read it).
            tile.dimension = m_pMap -> MapProjection().TileViewRect( pos, Tmx(), GetDimension2D(),
                                                            GetEffectiveCameraForFactor( layer.pLayer -> parallaxx, layer.pLayer -> parallaxy ) );

            return true;
        }

        /**
         * Convert world coordinate to tile matrix coordinate.
         * @param coord The World coordinate to translate to tile matrix coordinate;
         * @param pos Reference to struct @ref SunLight::TileMap::stMatrixPosition to receive the
         * tile matrix position based on world coordinate passed as parameter;
         */
        bool TileMapRenderer :: TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                                     SunLight :: TileMap :: stMatrixPosition& pos )  {

            if( m_pMap )  {
                SunLight :: Base :: stDimension2D&  vp = GetViewport().GetDimension2D();
                SunLight :: Base :: stZoomProperties&  zp = GetViewport().GetZoomProperties();

                if( m_pMap -> MapProjection().ViewToTileMatrix( coord, Tmx(), vp, ActiveCamera().Position(), zp.fZoomFactor,
                                                                MapPixelWidth(), MapPixelHeight(), pos ) )  {

                    // Both edges: GetTile indexes gids[] with these, and a
                    // row/column past the far edge would read past the array. Only
                    // the near edge used to be checked, so a sprite in the last
                    // viewport-origin-sized strip of the map (the origin is added
                    // above) got row/column == width/height (A6).
                    return ( ( pos.nTileCol >= 0 ) && ( pos.nTileRow >= 0 ) &&
                             ( pos.nTileCol < ( int ) Tmx() -> width ) &&
                             ( pos.nTileRow < ( int ) Tmx() -> height ) );
                }
            }

            return false;
        }

        /**
         * Load the specified map to renderer.
         * @param szTmxMapFile Renderer map file;
         * @param alignment Map alignment according @ref SunLight::TileMap::ITileMap::MapAlignment enumerator;
         */
        bool TileMapRenderer :: LoadMap( const char* szTmxMapFile, SunLight :: TileMap :: ITileMap :: MapAlignment alignment )  {

            if( m_bIsStarted )  {
                // S1c1: the map is read, parsed and given its projection by MapData::Load (which says
                // why on standard error when it cannot); a failed load leaves the loaded map as it is.
                std :: unique_ptr<MapData>  pNewMap = MapData :: Load( szTmxMapFile );

                if( !pNewMap )
                    return false;

                // The new map loaded: only now does the previous one (if any) go - destroying its
                // MapData frees it, its textures and its Resource Manager; the sprites registered with
                // AddSprite stay registered, as they always did across a LoadMap.
                m_pMap = std :: move( pNewMap );

                uint16_t  nMapWidth  = MapPixelWidth();
                uint16_t  nMapHeight = MapPixelHeight();

                // Setup alignment (Camera::Align): the active view's camera, for its viewport and zoom.
                ActiveCamera().Align( alignment, nMapWidth, nMapHeight, GetViewport().GetDimension2D(),
                                      GetViewport().GetZoomProperties().fZoomFactor );

                /*
                * Set scrolling properties.
                */                /*
                * Set scrolling properties.
                */
                SunLight :: Base :: stSize2D  defaultStep = m_pMap -> DefaultScrollStep();

                // For every view: "-1" means the tile size of the map they now show.
                m_pDefaultView -> m_Camera.ResolveScrollStep( defaultStep );

                for( std :: shared_ptr<View>& pView : m_ExtraViews )
                    pView -> m_Camera.ResolveScrollStep( defaultStep );

                return true;
            }

            return false;
        }

        /**
         * Unload a previously loaded map and it's related data (animations, sprites, etc...);
         */
        bool TileMapRenderer :: UnloadMap( void )  {

            if( m_pMap )  {
                UnloadSprites();
                m_pMap.reset();   // frees the map, its textures, its Resource Manager and its tile animations

                return true;
            }

            return false;
        }

        /**
         * Get the current map information data.
         * @param mapInfo Reference to the struct @ref SunLight::TileMap::stMapInfo that will
         * receive the map information.
         */
        bool TileMapRenderer :: GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo )  {

            tmx_map  *pTmxMap = Tmx();

            if( pTmxMap )  {
                mapInfo.mapSize.nWidth   = pTmxMap -> width;
                mapInfo.mapSize.nHeight  = pTmxMap -> height;
                mapInfo.tileSize.nWidth  = pTmxMap -> tile_width;
                mapInfo.tileSize.nHeight = pTmxMap -> tile_height;
                mapInfo.pMap = pTmxMap;

                return true;
            }

            return false;
        }

        /**
         * @brief Look up a level-design-authored object (a Tiled `<object>`
         * placed inside any `<objectgroup>` layer) by name - searches every
         * object-group layer in the loaded map, including ones nested
         * inside `<group>` layers (see MapData::FindObjectByName), since libtmx has
         * no tmx_find_object_by_name of it's own (only
         * tmx_find_object_by_id, no use here - objects are authored/named
         * by hand in the map editor, not looked up by their internal id).
         *
         * @param szObjectName The object's name, as set in the map editor.
         * @param object Reference to receive the object's data if found.
         * @return true if an object with this name was found, false
         * otherwise (including if no map is loaded).
         */
        bool TileMapRenderer :: GetObjectByName( const char* szObjectName, SunLight :: TileMap :: stObject& object )  {

            if( !m_pMap )
                return false;

            return m_pMap -> FindObjectByName( szObjectName, object );
        }

        /**
         * Return the reference to internal renderer collision manager.
         */
        SunLight :: Collision :: ICollisionManager& TileMapRenderer :: GetCollisionManager( void )  {

            return m_CollisionManager;
        }

        /**
         * Add a sprite to world on top of specified layer (see
         * @see ITileMap::AddSprite's own doc comment for the full "why" -
         * mapless-rendering support, added so a sprite can exist with no
         * tile map ever loaded at all). GetLayer(nLayerId) validates
         * nLayerId against the currently loaded map's own real layers
         * when one is loaded (Tmx() != nullptr) - unchanged from
         * before. When no map is loaded, there's nothing real to validate
         * nLayerId against, so it's skipped entirely and nLayerId is
         * accepted as a plain caller-chosen grouping key - the same role
         * it already plays for CollisionManager::AddCollider (a fixed-
         * size array indexed by integer id, with zero tmx dependency) and
         * Sprite::SetParent (accepts any BaseCanvas*, no tmx dependency
         * either). Nothing below this check treats nLayerId any
         * differently depending on which branch let it through.
         *
         * That said, AddCollider's own bounds check (nLayerId must be
         * less than MAX_COLLIDER_LAYERS) still applies either way, and is
         * now honored rather than ignored - a mapless caller is free to
         * pick any nLayerId as a grouping key, but one at or past
         * MAX_COLLIDER_LAYERS fails AddSprite outright instead of
         * silently succeeding with a sprite whose collider never actually
         * registered with the collision manager.
         * @param nLayerId Id of an existing Layer on tiled map whose sprite will be added
         * (or, with no map loaded, any caller-chosen grouping id, below
         * MAX_COLLIDER_LAYERS);
         * @param sprite Reference to the sprite that will be added;
         */
        bool TileMapRenderer :: AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite )  {

            if( m_bIsStarted && ( ( Tmx() == nullptr ) || GetLayer( nLayerId ) ) )  {
                // Opened before the checks below, as it always was (see SpriteRegistry::OpenLayer).
                m_Sprites.OpenLayer( nLayerId );

                if( !m_Sprites.Contains( nLayerId, &sprite ) &&
                    m_CollisionManager.AddCollider( nLayerId, &sprite.GetCollider() ) )  {

                    sprite.SetParent( this );
                    // L1 of the master plan: so a world-space sprite's own GetCameraOffset
                    // can later ask for THIS layer's parallax factor, at any time, with no
                    // dependency on whether a render pass has ever run.
                    sprite.SetParallaxLayerId( nLayerId );
                    m_Sprites.MarkParented( &sprite );
                    m_Sprites.Add( nLayerId, &sprite );
                    return true;
                }
            }

            return false;
        }

        /**
         * Remove a sprite from world.
         * @param nLayerId Id of an existing layer on tiled map whose sprite was previously added by 
         * @see AddSprite and now will be removed;
         * @param sprite Reference to the sprite that will be removed;
         */
        bool TileMapRenderer :: RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite )  {

            if( m_bIsStarted && m_Sprites.Contains( nLayerId, &sprite ) )  {
                m_CollisionManager.RemoveCollider( nLayerId, &sprite.GetCollider() );
                m_Sprites.Remove( nLayerId, &sprite );

                return true;
            }

            return false;
        }

        /**
         * Start engine renderer.
         */
        bool TileMapRenderer :: Start( void )  {

            SunLight :: Window :: IWindow  &window = SunLight :: Window :: WindowFactory :: GetWindow();

            // Born fullscreen when configured (RendererConfig::bFullscreen): the window backend applies it
            // as part of creating the window - the same state SetFullscreen would give, without a windowed phase.
            if( !window.Create( ( int ) m_fWindowWidth,
                                ( int ) m_fWindowHeight,
                                m_strTitle.c_str(),
                                m_bWindowResizeable,
                                m_bStartFullscreen,
                                m_StartFullscreenStrategy ) )  {
                ::tmx_perror( "Cannot create a window" );
                return false;
            }

            // Apply the configured exit key (ESC unless the caller chose
            // another - via RendererConfig or SetExitKey, before or after
            // an earlier Start()), so a choice made before Start() sticks
            // instead of being overwritten with the default.
            SetExitKey( m_ExitKey );

            // Reset any exit request left over from a previous Start()/
            // Run()/Stop() cycle - a stopped-then-restarted renderer
            // isn't stuck permanently exited (see RequestExit's own doc
            // comment).
            m_bExitRequested = false;
            m_nFramesRun     = 0;

            /*
             * Resolves the -1 constructor-default sentinel into
             * m_nTargetFps itself (so GetTargetFPS() never leaks it back
             * to a caller once the window exists), then applies it
             * straight through IWindow. Deliberately NOT through the
             * member SetTargetFPS(), which only forwards once
             * m_bIsStarted is true - that flag isn't set yet at this
             * point in Start().
             */
            m_nTargetFps = ( m_nTargetFps != -1 ) ? m_nTargetFps : __DEFAULT_FPS;
            window.SetTargetFPS( m_nTargetFps );

            /*
             * Everything renders into this fixed-size offscreen target
             * (Run() blits it scaled+letterboxed to the real window/screen
             * every frame) - game/camera/collision math never needs to
             * know the actual window size at all. If Start() is somehow
             * called again without an intervening Stop(), release
             * whatever render target already exists first so it isn't
             * leaked.
             */
            if( m_pRenderTexture != nullptr )  {
                SunLight :: Engines :: EngineFactory :: GetEngine().UnloadRenderTarget( m_pRenderTexture );
                m_pRenderTexture = nullptr;
            }

            m_pRenderTexture = SunLight :: Engines :: EngineFactory :: GetEngine().LoadRenderTarget(
                                                      ( int ) m_fWindowWidth,
                                                      ( int ) m_fWindowHeight );
            m_bIsStarted = true;

            return m_bIsStarted;
        }

        /**
         * Stop renderer freeing all allocated resources.
         */
        void TileMapRenderer :: Stop( void )  {

            for( SunLight :: TileMap :: ITileMapListener* pListener : ListenersSnapshot() )  {
                if( IsListenerRegistered( pListener ) )
                    pListener -> OnStop();
            }

            if( m_bIsStarted )  {
                UnloadMap();
                // UnloadMap only reaches UnloadSprites when a map is loaded; sprites registered
                // with no map ever loaded (M2) must be released here too, or their per-layer
                // lists leak and they stay registered across a restart.
                UnloadSprites();

                // Fonts go before the window closes: the engine's close handler
                // would release them too, but that is its own state, not ours.
                ReleaseRegisteredFonts();

                if( m_pRenderTexture != nullptr )  {
                    SunLight :: Engines :: EngineFactory :: GetEngine().UnloadRenderTarget( m_pRenderTexture );
                    m_pRenderTexture = nullptr;
                }

                // Closing the window fires its close handlers first (see
                // IWindow::AddCloseHandler), which is where a backend engine
                // releases any GPU-context-tied state it privately keeps
                // (e.g. a custom font loaded via SetFont) while that
                // context is still valid - nothing to call on the engine
                // from here.
                SunLight :: Window :: WindowFactory :: GetWindow().Close();
                m_bIsStarted = false;
            }
        }

        /**
         * Run renderer. Loops until the hardware exit key/window close
         * button fires (IWindow::ShouldClose()), the optional frame budget
         * (RendererConfig::nMaxFrames, counted since Start()) is spent, or a
         * caller/listener called @see RequestExit - checked once per iteration, not
         * immediately, so a RequestExit() call from inside this frame's
         * own HandleTileMapUpdate/etc. still lets the frame finish drawing
         * before the loop actually exits. Either way, this function only
         * returns - it never closes the window itself; that's still
         * @see Stop's job alone, unconditionally, regardless of which
         * condition broke the loop.
         */
        bool TileMapRenderer :: Run( void )  {

            if( m_bIsStarted )  {
                SunLight :: Window :: IWindow  &window = SunLight :: Window :: WindowFactory :: GetWindow();

                while( !window.ShouldClose() && !m_bExitRequested &&
                        ( ( m_nMaxFrames == 0 ) || ( m_nFramesRun < m_nMaxFrames ) ) )  {
                    SunLight :: Engines :: EngineFactory :: GetEngine().BeginRenderTarget( m_pRenderTexture );
                    if( GetVisible() )  {
                        RenderMap();
                        m_pControllerManager -> Update();
                        HandleTileMapUpdate();
                        HandleUserCollisions();

                        /*
                         * Screen fade overlay - drawn last, after
                         * everything else this frame, so it composites on
                         * top of the whole rendered scene rather than any
                         * single element. Deliberately inside the
                         * GetVisible() branch: if nothing was rendered this
                         * frame, there's nothing fresh to composite over -
                         * drawing it unconditionally would paint over
                         * whatever stale content is still in the frame
                         * buffer from the last visible frame instead.
                         * Routed through IEngine (not raylib's ::DrawRectangle/
                         * ::Fade directly) - full window bounds, deliberately
                         * bypassing any viewport/camera state.
                         */
                        if( m_fScreenFadeAlpha > 0.0f )  {
                            SunLight :: Base :: stColor  fadeColor = BLACK_COLOR;

                            fadeColor.nAlpha = ( unsigned char ) ( 255.0f * m_fScreenFadeAlpha );

                            SunLight :: Engines :: EngineFactory :: GetEngine().DrawFilledRectangle(
                                                                  0, 0,
                                                                  ( int ) m_fWindowWidth,
                                                                  ( int ) m_fWindowHeight,
                                                                  fadeColor );
                        }
                    }
                    SunLight :: Engines :: EngineFactory :: GetEngine().EndRenderTarget();

                    /*
                     * Blit the fixed-resolution render texture to the real
                     * window - recomputed from the actual current screen
                     * size every single frame, so both fullscreen
                     * (SetFullscreen) and live window resizing (if
                     * m_bWindowResizeable) fall out of this one path with
                     * no separate resize-event handling needed. Source
                     * height is negative because render targets are
                     * stored bottom-up (OpenGL convention) - this flips it
                     * back right-side up. The frame itself is bracketed by
                     * IWindow::BeginFrame/EndFrame; everything drawn inside
                     * it goes through IEngine, same as the offscreen pass
                     * above.
                     *
                     * m_bStretchToFill (see it's own doc comment on
                     * SetStretchToFill) picks between two different dest
                     * rectangles: the default letterboxed one uses a
                     * single uniform fScale (the smaller of the two axis
                     * ratios) so the whole render target fits inside the
                     * screen with it's own aspect ratio intact, centered
                     * with black bars filling whatever's left over on the
                     * non-fitting axis. Stretch-to-fill instead sizes dest
                     * to the full screen directly, with no centering
                     * offset needed - IEngine::DrawTextureScaled (raylib's
                     * own DrawTexturePro underneath) already maps an
                     * arbitrary source rect onto an arbitrary dest rect
                     * using independent X/Y scale factors on it's own, so
                     * this needs no change to that call itself, only to
                     * which dest rectangle gets computed here.
                     */
                    int    nScreenWidth  = window.GetScreenWidth();
                    int    nScreenHeight = window.GetScreenHeight();

                    SunLight :: Base :: stRectangle  source { 0.0f, 0.0f, m_fWindowWidth, -m_fWindowHeight };
                    SunLight :: Base :: stRectangle  dest;

                    if( m_bStretchToFill )  {
                        dest = SunLight :: Base :: stRectangle { 0.0f, 0.0f,
                                                                  ( float ) nScreenWidth,
                                                                  ( float ) nScreenHeight };
                    }
                    else  {
                        float  fScaleX = ( float ) nScreenWidth / m_fWindowWidth;
                        float  fScaleY = ( float ) nScreenHeight / m_fWindowHeight;
                        float  fScale  = ( fScaleX < fScaleY ) ? fScaleX : fScaleY;

                        dest = SunLight :: Base :: stRectangle { ( nScreenWidth - ( m_fWindowWidth * fScale ) ) * 0.5f,
                                                                  ( nScreenHeight - ( m_fWindowHeight * fScale ) ) * 0.5f,
                                                                  m_fWindowWidth * fScale,
                                                                  m_fWindowHeight * fScale };
                    }

                    window.BeginFrame();
                    SunLight :: Engines :: EngineFactory :: GetEngine().ClearBackground( BLACK_COLOR );
                    SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureScaled(
                                          SunLight :: Engines :: EngineFactory :: GetEngine().GetRenderTargetTexture( m_pRenderTexture ),
                                          source, dest, WHITE_COLOR );
                    window.EndFrame();
                    m_nFramesRun++;
                }

                return true;
            }

            return false;
        }
    }
}
