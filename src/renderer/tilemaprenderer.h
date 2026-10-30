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

#ifndef __TILEMAPRENDERER_H__
#define __TILEMAPRENDERER_H__

#include <string>
#include <queue>
#include <vector>
#include <array>
#include <map>
#include <memory>
#include <set>
#include "collision/collisionmanager.h"
#include "tilemap/itilemaplistener.h"
#include "drawsurface/idrawsurface.h"
#include "drawsurface/ishapesurface.h"
#include "font/ifont.h"
#include "renderer/rendererconfig.h"
#include "renderer/shape/shapeprimitives.h"
#include "renderer/shape/shaperenderer.h"
#include "renderer/drawcontext.h"
#include "renderer/text/fontregistry.h"
#include "renderer/objects/objectrenderer.h"
#include "renderer/map/mapdata.h"
#include "renderer/map/layerrenderer.h"
#include "renderer/sprite/spriteregistry.h"
#include "renderer/camera/camera.h"
#include "renderer/projection/imapprojection.h"
#include "input/iinputhandler.h"
#include "input/icontrollermanager.h"
#include "base/color.h"
#include "base/primitives.h"
#include "backends/null/nullbackend.h"
#include "renderer/view.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * Primitive structures definition.
         */
        typedef SunLight :: Base :: stVector2D    stVector;

        /**
         * @brief World renderer implementation to render TMX based maps.
         * Also implements IDrawSurface directly (see it's own header
         * comment) - the one concrete class both interfaces share, since
         * it's the object that actually owns the window/render context
         * both are ultimately backed by.
         */
        class TileMapRenderer : public SunLight :: TileMap :: ITileMap,
                                 public SunLight :: DrawSurface :: IDrawSurface,
                                 public SunLight :: DrawSurface :: IShapeSurface  {

            typedef std :: deque<SunLight :: TileMap :: ITileMapListener*> TileMapListenerList;

            // Non-null only for a RENDERER_BACKEND_NULL renderer: holds the
            // (shared, process-wide) null backend installed for as long as
            // this renderer lives. Deliberately the FIRST member, so it is
            // destroyed LAST - after everything below that might still call
            // into the engine/window/input while being torn down.
            std :: shared_ptr<SunLight :: Backends :: Null :: NullBackend>  m_pNullBackend;

            // Input dispatch/registration (F0a of the master plan): owns its own IInputHandler,
            // the registered key/gamepad handler lists and the gamepad list, dispatched once per
            // frame via Update() (see Run()). TileMapRenderer's own public input methods
            // (AddGamePad, SetUserKeyEventHandler, ...) delegate to this directly.
            std :: unique_ptr<SunLight :: Input :: IControllerManager>  m_pControllerManager;

            // The sprites registered per layer (AddSprite), every sprite this renderer is the PARENT of
            // (set by AddSprite, kept by RemoveSprite as it always was: the ones whose parent pointer
            // would dangle if this renderer went away, and that tell it when they are destroyed -
            // ChildRemoved), and the once-per-frame sprite update across the view passes (S1d).
            SpriteRegistry                             m_Sprites;
            TileMapListenerList                        m_TileMapListenerList;
            SunLight :: Collision :: CollisionManager  m_CollisionManager;
            ShapeRenderer                              m_ShapeRenderer;          // draws every shape (S1a): shape objects and IShapeSurface

            // The CURRENT layer's own effective camera (L1 of the master plan: per-layer
            // parallax) - the active camera scaled by that layer's own parallax factor (and its
            // group ancestors', multiplied together, matching Tiled's own nesting rule) and
            // anchored at the map's parallaxoriginx/y. Set once per layer, of any type, at
            // the top of DrawAllLayers's own loop body - before dispatching to
            // LayerRenderer/ObjectRenderer (through the DrawContext) and before HandleSpriteUpdate, so every one
            // of them (including a world-space sprite's own GetCameraOffset) reads the same,
            // already-correct value for whichever layer is currently being processed, with no
            // separate per-sprite layer tracking needed at all. A layer with no parallax
            // (factor 1,1 - the overwhelming default) gets this set to exactly the active camera,
            // byte-identical to this project's pre-L1 behavior.
            stVector                                   m_EffectiveLayerCameraPos;

            // I8 of the master plan: the opacity and tint the layer being drawn inherits from its
            // enclosing groups (DrawAllLayers), passed to the drawing classes through the DrawContext.
            // 1 and white outside any group, and again once the map is drawn.
            float                                      m_fGroupOpacity = 1.0f;
            SunLight :: Base :: stColor                m_GroupTint     = WHITE_COLOR;
            float                                      m_fWindowWidth;
            float                                      m_fWindowHeight;
            int                                        m_nTargetFps;
            SunLight :: Base :: stColor                m_WindowBackgroundColor;
            bool                                       m_bWindowBackgroundSet;   // SetWindowBackgroundColor was called: the window area is not the map's colour
            std :: string                              m_strTitle;
            float                                      m_fScreenFadeAlpha;

            // The loaded map and everything that belongs to it (S1c1: MapData - the libtmx map, its
            // Resource Manager, projection, pixel size and tile animation state); null when no map is
            // loaded. Replacing it with a newly loaded MapData frees the old map exactly once.
            std :: unique_ptr<MapData>                 m_pMap;
            bool                                       m_bClearBackground;
            bool                                       m_bIsStarted;
            bool                                       m_bExitRequested;
            SunLight :: Input :: KeyboardKey           m_ExitKey;
            bool                                       m_bWindowResizeable;
            bool                                       m_bStartFullscreen;
            SunLight :: Window :: FullscreenStrategy   m_StartFullscreenStrategy;
            bool                                       m_bDrawFPS;
            bool                                       m_bStretchToFill;
            unsigned                                   m_nMaxFrames;
            unsigned                                   m_nFramesRun;

            // Everything is rendered into this fixed-internal-resolution
            // offscreen render target (via IEngine), then Run() blits it
            // scaled+letterboxed to whatever the actual window/screen size
            // currently is - the one place both fullscreen and live
            // window-resizing are handled, since both just mean "the real
            // screen size differs from this".
            SunLight :: Base :: TextureHandle          m_pRenderTexture;

            // Views (see SunLight::TileMap::IView). The DEFAULT view (id 0)
            // owns what the renderer always had - camera, scroll step and
            // root viewport - and is the ACTIVE one whenever nothing else is.
            // Every view holds its own Camera (S1e); the renderer's camera code
            // works on the ACTIVE view's (ActiveCamera), and ActivateView (which
            // View's operations and the draw passes use) switches which view that
            // is, so all the camera/zoom code serves every view unchanged.
            std :: shared_ptr<View>                    m_pDefaultView;
            std :: vector<std :: shared_ptr<View>>     m_ExtraViews;
            View                                       *m_pActiveView;
            int                                        m_nNextViewId;

            FontRegistry                               m_Fonts;             // RegisterFont's fonts (S1b); released by ReleaseRegisteredFonts
            ObjectRenderer                             m_ObjectRenderer;    // object layers: shapes, text, the opacity rule (S1b)
            LayerRenderer                              m_LayerRenderer;     // tile and image layers, tile objects (S1c2)

            // Multi-pass frame (only used once an extra view exists - see RenderMap): the views of
            // this frame's passes in draw order.
            std :: vector<View*>                       m_PassViews;

            friend class View;

            View* ActivateView( View* pView );
            Camera& ActiveCamera( void ) const;      // the active view's camera (S1e)

            // The loaded map (S1c1): its libtmx map (null when none is loaded), its size in map pixels
            // (0 when none), and its layers by id or name (null when none, or no such layer).
            tmx_map* Tmx( void ) const;
            uint16_t MapPixelWidth( void ) const;
            uint16_t MapPixelHeight( void ) const;
            tmx_layer* GetLayer( int nLayerId );
            tmx_layer* GetLayer( const char* szLayerName );

            // Sprites miscelaneous
            void UnloadSprites( void );

            // Listener dispatch that tolerates a listener (un)registering listeners from it's own callback
            TileMapListenerList ListenersSnapshot( void ) const;
            bool IsListenerRegistered( SunLight :: TileMap :: ITileMapListener* pListener ) const;

            // Color control
            SunLight :: Base :: stColor IntToColor( uint32_t color );

            // The view and layer being drawn, for the drawing classes (S1a)
            DrawContext CurrentDrawContext( void );

            // M1 of the master plan: converts a caller-owned stVector2D array into the double**
            // shape the private DrawPolyline/DrawPolygon overloads (and libtmx itself) use, in
            // storage/ppRows only - nothing escapes this call. See IShapeSurface::DrawPolyline's
            // own doc comment for the full why.
            void ToTmxPointArray( const SunLight :: Base :: stVector2D* pPoints,
                                  int nPointsCount,
                                  std :: vector<double>& storage,
                                  std :: vector<double*>& ppRows );
            void ReleaseRegisteredFonts( void );
            void DrawAllLayers( tmx_layer* pLayer, double fParentParallaxX = 1.0, double fParentParallaxY = 1.0,
                                float fParentOpacity = 1.0f, SunLight :: Base :: stColor parentTint = WHITE_COLOR );
            void RenderMap( void );

            // L1 of the master plan: the effective camera for a layer whose own accumulated
            // parallax factor (its own parallaxx/y, already multiplied by every group ancestor's
            // own factor) is fFactorX/fFactorY - the active view's camera, the layer placed as Tiled
            // places it about that view's centre and the map's parallax origin (Camera::ForParallax);
            // exactly that camera, per axis, when that axis's factor is 1.0.
            stVector GetEffectiveCameraForFactor( double fFactorX, double fFactorY );

            // I7 of the master plan: a tile's rectangle in map space (see GetTile).
            SunLight :: Base :: stDimension2D TileMapRect( const SunLight :: TileMap :: stMatrixPosition& pos,
                                                           tmx_layer* pLayer,
                                                           tmx_tile* pTile );

            // User interaction handlers
            inline void HandleTileMapUpdate( void );
            inline void HandleUserCollisions( void );
            inline void HandleSpriteUpdate( int nLayerId );
            SunLight :: Base :: stColor BackgroundColorOf( View& view );
            void DrawViewPasses( void );
            void DrawPassContent( void );

            // Internal layer handlers
            void CopyLayerToTmx( tmx_layer* pTmxLayer, SunLight :: TileMap :: stLayer& layer );
            void CopyTmxToLayer( SunLight :: TileMap :: stLayer& layer, tmx_layer* pTmxLayer );


            public:

            TileMapRenderer( float fWidth,
                             float fHeight,
                             const char* szTitle,
                             int nTargetFps = -1 );
            explicit TileMapRenderer( const SunLight :: Renderer :: RendererConfig& config );
            virtual ~TileMapRenderer( void );

            // A sprite that is destroyed, or moved to another parent, calls this on the parent it is leaving.
            void ChildRemoved( SunLight :: Canvas :: BaseCanvas* pChild ) override;

            // The camera of the view being drawn (the active view's), for world-space sprites -
            // scaled by nLayerId's own parallax factor (L1 of the master plan) when it names a
            // real layer (-1, BaseCanvas's own sentinel, means none: a loose canvas, or one never
            // added via AddSprite, gets the true, unscaled camera). Computed fresh on every call,
            // not from any per-frame draw-time state, so it answers correctly whether or not a
            // render pass has run yet.
            void GetCameraOffset( float& fX, float& fY, int nLayerId = -1 ) override;

            // Checked creation: validates the config (see
            // RendererConfig::Validate) and returns nullptr, with a message
            // in *pError, instead of building a renderer from a config that
            // can't work (e.g. a backend not compiled into this build).
            static std :: unique_ptr<TileMapRenderer> Create( const SunLight :: Renderer :: RendererConfig& config,
                                                              std :: string* pError = nullptr );

            // World user listener interaction
            void AddTileMapListener( SunLight :: TileMap :: ITileMapListener* pListener );
            void RemoveTileMapListener( SunLight :: TileMap :: ITileMapListener* pListener );

            // Window behavior
            void SetExitKey( SunLight :: Input :: KeyboardKey key ) override;
            SunLight :: Input :: KeyboardKey GetExitKey( void ) override;
            void SetWindowResizeable( bool bResizeable ) override;
            bool GetWindowResizeable( void ) override;
            void RequestExit( void ) override;
            bool GetExitRequested( void ) override;
            void SetTargetFPS( int nTargetFps ) override;
            int  GetTargetFPS( void ) override;
            void SetWindowBackgroundColor( SunLight :: Base :: stColor color );
            void SetClearBackground( bool bStatus );
            bool GetClearBackground( void );
            void SetDrawFPS( bool bDrawFPS ) override;
            bool GetDrawFPS( void ) override;
            void SetFullscreen( bool bFullscreen,
                               SunLight :: Window :: FullscreenStrategy strategy =
                                   SunLight :: Window :: FULLSCREEN_STRATEGY_REAL ) override;
            bool GetFullscreen( void ) override;
            SunLight :: Window :: FullscreenStrategy GetFullscreenStrategy( void ) override;
            double GetElapsedTime( void ) override;
            void SetStretchToFill( bool bStretchToFill ) override;
            bool GetStretchToFill( void ) override;

            // Views
            SunLight :: TileMap :: IView& GetDefaultView( void ) override;
            std :: shared_ptr<SunLight :: TileMap :: IView> CreateView( const SunLight :: Base :: stDimension2D& rect ) override;
            std :: shared_ptr<SunLight :: TileMap :: IView> GetView( int nViewId ) override;
            bool RemoveView( int nViewId ) override;
            bool RemoveView( const std :: shared_ptr<SunLight :: TileMap :: IView>& pView ) override;
            int GetViewCount( void ) override;

            // View port control
            void SetViewControlMode( SunLight :: Renderer :: ViewControlMode mode );
            void SetScrollStepSize( int nStepWidth, int nStepHeight );
            void GetScrollStepSize( int& nStepWidth, int& nStepHeight );

            // GamePad handling
            void AddGamePad( int nGamePadId ) override;

            // User input handling           
            void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt, SunLight :: Input :: INPUT_EVENT_HANDLER handler ) override;
            void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt, SunLight :: Input :: INPUT_EVENT_HANDLER handler ) override;

            SunLight :: Input :: IInputHandler& GetInputHandler( void ) override;

            // The controller manager these input methods delegate to (F0a of the master plan),
            // for direct access - the same shape as GetCollisionManager().
            SunLight :: Input :: IControllerManager& GetControllerManager( void );

            // Zoom wrappers
            void ResetZoom( void ) override;
            void ZoomIn( void ) override;
            void ZoomOut( void ) override;

            // Camera management
            void ResetCamera( void ) override;
            void MoveCameraUp( void ) override;
            void MoveCameraDown( void ) override;
            void MoveCameraLeft( void ) override;
            void MoveCameraRight( void ) override;
            void SetCameraPosition( int nX, int nY ) override;
            void GetCameraPosition( int& nX, int& nY ) override;

            // Window management
            void SetWindowTitle( const std :: string& strTitle ) override;
            void SetScreenFade( float fAlpha ) override;
            void BeginBlendMode( SunLight :: Base :: BlendMode mode ) override;
            void EndBlendMode( void ) override;

            // Text rendering
            std :: shared_ptr<SunLight :: Font :: IFont> LoadFont( const char* szFilePath ) override;
            bool SetFont( std :: shared_ptr<SunLight :: Font :: IFont> pFont ) override;
            std :: shared_ptr<SunLight :: Font :: IFont> GetFont( void ) override;

            // Register a loaded font (LoadFont) for text objects: a text object
            // whose fontfamily and bold/italic flags match these draws with it.
            // The font is shared, so the same one can also be the active font
            // (SetFont) or serve other families. Registering the same family and
            // style again replaces the earlier font.
            bool RegisterFont( const char* szFamily, std :: shared_ptr<SunLight :: Font :: IFont> pFont, bool bBold, bool bItalic );

            // Let go of a registered font: in every family and style it serves (by the font itself),
            // or for one family and style. Text objects left without a font draw ??????.
            bool UnregisterFont( const std :: shared_ptr<SunLight :: Font :: IFont>& pFont );
            bool UnregisterFont( const char* szFamily, bool bBold, bool bItalic );
            void DrawText( const char* szText,
                           int nPosX,
                           int nPosY,
                           int nFontSize,
                           unsigned char nRed,
                           unsigned char nGreen,
                           unsigned char nBlue,
                           unsigned char nAlpha ) override;
            void DrawFilledRectangle( int nPosX,
                                      int nPosY,
                                      int nWidth,
                                      int nHeight,
                                      unsigned char nRed,
                                      unsigned char nGreen,
                                      unsigned char nBlue,
                                      unsigned char nAlpha ) override;
            int MeasureText( const char* szText, int nFontSize ) override;
            int GetWindowWidth( void ) override;
            int GetWindowHeight( void ) override;

            // IShapeSurface (M1 of the master plan): the same ShapeRenderer (S1a) Tiled shape
            // objects are drawn through, given a public, mapless door. Each override converts
            // its own map-unit thickness into ShapeRenderer's screen-pixel line width (and its
            // points into libtmx's point-array shape), then calls straight through - no drawing logic
            // is duplicated or reimplemented here.
            void DrawRectangle( double fX, double fY, double fWidth, double fHeight,
                                SunLight :: Base :: stColor color,
                                double fThickness = 1.0, double fRotation = 0.0 ) override;
            void DrawEllipse( double fX, double fY, double fWidth, double fHeight,
                              SunLight :: Base :: stColor color,
                              double fThickness = 1.0, double fRotation = 0.0 ) override;
            void DrawPolyline( double fX, double fY,
                               const SunLight :: Base :: stVector2D* pPoints, int nPointsCount,
                               SunLight :: Base :: stColor color,
                               double fThickness = 1.0, double fRotation = 0.0 ) override;
            void DrawPolygon( double fX, double fY,
                              const SunLight :: Base :: stVector2D* pPoints, int nPointsCount,
                              SunLight :: Base :: stColor color,
                              double fThickness = 1.0, double fRotation = 0.0 ) override;
            void DrawPoint( double fX, double fY, SunLight :: Base :: stColor color,
                            double fSize = 1.0 ) override;

            // Layer management
            bool SetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer ) override;
            bool SetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer ) override;
            bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer ) override;
            bool GetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer ) override;

            // L1 of the master plan: set a layer's own parallax factor in code, overriding
            // whatever the TMX file itself declared (fFactorX/fFactorY 1.0 = no parallax).
            // Writes straight into the tmx_layer, the same pattern SetLayer already uses for
            // opacity/offset/visible - so, like those, the original TMX-authored value is not
            // recoverable once overridden.
            bool SetLayerParallax( int nLayerId, float fFactorX, float fFactorY );
            bool SetLayerParallax( const char* szLayerName, float fFactorX, float fFactorY );

            // Tile management
            bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                          const SunLight :: TileMap :: stLayer& layer,
                          SunLight :: TileMap :: stTile& tile ) override;
            bool GetCollidingTiles( const SunLight :: Base :: stRectangle& rect,
                                    bool bWorldSpace,
                                    const SunLight :: TileMap :: stLayer& layer,
                                    std :: vector<SunLight :: TileMap :: stTile>& tiles ) override;

            // Coordinate conversion
            bool TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                      SunLight :: TileMap :: stMatrixPosition& pos ) override;

            // Map management
            bool LoadMap( const char* szTxMapFile, SunLight :: TileMap :: ITileMap :: MapAlignment alignment = MAP_ALIGNMENT_CENTER ) override;
            bool UnloadMap( void ) override;
            bool GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo ) override;

            // Object management
            bool GetObjectByName( const char* szObjectName, SunLight :: TileMap :: stObject& object ) override;
            bool GetTileFrame( int nLayerId, const SunLight :: TileMap :: stMatrixPosition& pos, unsigned int& nGID ) override;
            bool GetObjectTileFrame( const char* szObjectName, unsigned int& nGID ) override;

            // Collision management
            SunLight :: Collision :: ICollisionManager& GetCollisionManager( void ) override;

            // Sprite management
            bool AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) override;
            bool RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) override;

            // Renderer flow control.
            bool Start( void );
            void Stop( void );
            bool Run( void );
        };
    }
}

#endif /* __TILEMAPRENDERER_H__ */
