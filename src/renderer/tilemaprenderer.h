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
#include "font/ifont.h"
#include "renderer/rendererconfig.h"
#include "renderer/shape/shapeprimitives.h"
#include "renderer/projection/imapprojection.h"
#include "input/iinputhandler.h"
#include "input/icontrollermanager.h"
#include "base/color.h"
#include "base/primitives.h"


namespace SunLight {
    namespace Backends  {
        namespace Null  {
            class NullBackend;
        }
    }
}

namespace SunLight {
    namespace Renderer  {

        class View;

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
                                 public SunLight :: DrawSurface :: IDrawSurface  {

            /**
             * Tile animation information.
             */
            struct __stTileAnimInfo  {
                int64_t      nMillis;
                uint16_t     nCounter;
                tmx_tile     *pNextTile;
            };

            typedef std :: deque<std :: unique_ptr<__stTileAnimInfo>> __AnimInfoList;
            typedef std :: deque<SunLight :: TileMap :: ITileMapListener*> TileMapListenerList;
            typedef std :: deque<SunLight :: Sprite :: Sprite*> SpriteList;
            typedef std :: map<int, SpriteList*>                SpriteMap;

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
            SpriteMap                                  m_SpriteMap;

            // Every sprite this renderer is the PARENT of (set by AddSprite, kept by RemoveSprite as it
            // always was): the sprites whose parent pointer would dangle if this renderer went away, and
            // the ones that must tell it when they are destroyed (ChildRemoved). A superset of m_SpriteMap.
            std :: set<SunLight :: Sprite :: Sprite*>  m_ParentedSprites;
            TileMapListenerList                        m_TileMapListenerList;
            SunLight :: Collision :: CollisionManager  m_CollisionManager;
            stVector                                   m_CameraPos;

            // The CURRENT layer's own effective camera (L1 of the master plan: per-layer
            // parallax) - m_CameraPos scaled by that layer's own parallax factor (and its
            // group ancestors', multiplied together, matching Tiled's own nesting rule) and
            // anchored at the map's parallaxoriginx/y. Set once per layer, of any type, at
            // the top of DrawAllLayers's own loop body - before dispatching to
            // DrawLayer/DrawObjects/DrawImageLayer and before HandleSpriteUpdate, so every one
            // of them (including a world-space sprite's own GetCameraOffset) reads the same,
            // already-correct value for whichever layer is currently being processed, with no
            // separate per-sprite layer tracking needed at all. A layer with no parallax
            // (factor 1,1 - the overwhelming default) gets this set to exactly m_CameraPos,
            // byte-identical to this project's pre-L1 behavior.
            stVector                                   m_EffectiveLayerCameraPos;
            uint16_t                                   m_nMapWidth;
            uint16_t                                   m_nMapHeight;
            std :: unique_ptr<Projection :: IMapProjection>  m_pProjection;
            // Whether every tileset the map uses declares the same tile size as the map itself - only then is
            // DrawLayer's culled loop exact (its visible-range math assumes one consistent grid step). A map
            // that mixes tileset sizes falls back to visiting every cell, same as before E0.
            bool                                       m_bUniformTileGrid;
            int                                        m_nScrollStepWidth;
            int                                        m_nScrollStepHeight;
            float                                      m_fWindowWidth;
            float                                      m_fWindowHeight;
            int                                        m_nTargetFps;
            SunLight :: Base :: stColor                m_WindowBackgroundColor;
            bool                                       m_bWindowBackgroundSet;   // SetWindowBackgroundColor was called: the window area is not the map's colour
            __AnimInfoList                             m_AnimInfoList;
            std :: string                              m_strTitle;
            float                                      m_fScreenFadeAlpha;
            tmx_map                                    *m_pTmxMap;

            // Owns the map's EXTERNAL tilesets (.tsx) and object templates
            // (.tx) - read through SunLight::FileSystem and handed to libtmx
            // as buffers (see LoadMap). nullptr when the map has none. Must
            // outlive m_pTmxMap and be freed after it (see UnloadMap).
            tmx_resource_manager                       *m_pTmxRcMgr;
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
            static bool                                m_bInitialized;

            // Everything is rendered into this fixed-internal-resolution
            // offscreen render target (via IEngine), then Run() blits it
            // scaled+letterboxed to whatever the actual window/screen size
            // currently is - the one place both fullscreen and live
            // window-resizing are handled, since both just mean "the real
            // screen size differs from this".
            SunLight :: Base :: TextureHandle          m_pRenderTexture;

            // Views (see SunLight::TileMap::IView). The DEFAULT view (id 0)
            // owns what the renderer always had - camera, scroll step and
            // root viewport - and is the ACTIVE one whenever nothing else is:
            // m_CameraPos / m_nScrollStep* above are the ACTIVE view's
            // working state, swapped in and out by ActivateView (which
            // View's operations and the draw passes use), so all the existing
            // camera/zoom code serves every view unchanged.
            std :: shared_ptr<View>                    m_pDefaultView;
            std :: vector<std :: shared_ptr<View>>     m_ExtraViews;
            View                                       *m_pActiveView;
            int                                        m_nNextViewId;

            // Multi-pass frame (only used once an extra view exists - see RenderMap): the
            // sprites already advanced this frame (a sprite advances ONCE per frame, however
            // many views draw it), and the views of this frame's passes in draw order.
            struct __stRegisteredFont  {
                std :: string                                 strFamily;
                bool                                          bBold;
                bool                                          bItalic;
                std :: unique_ptr<SunLight :: Font :: IFont>  pFont;   // owned here; released by ReleaseRegisteredFonts
            };
            std :: vector<__stRegisteredFont>          m_RegisteredFonts;
            std :: set<std :: string>                  m_WarnedFonts;       // "family|bold|italic" keys already warned about
            std :: set<SunLight :: Sprite :: Sprite*>  m_FrameAdvancedSprites;
            std :: set<SunLight :: Sprite :: Sprite*>  m_FramePendingSprites;       // reached by a pass, but not on screen there (yet)
            std :: vector<View*>                       m_PassViews;

            friend class View;

            View* ActivateView( View *pView );

            // TmxLib overrides
            static void* TextureLoaderCallback( const char *szFileName );
            static void TextureFreeCallback( void *pTexture );

            // TmxLib miscellaneous
            tmx_tileset_list* GetTilesetList( tmx_tileset *pTilesetSearch );
            tmx_layer* GetLayer( int nLayerId );
            tmx_layer* GetLayer( const char *szLayerName );

            // Sprites miscelaneous
            void UnloadSprites( void );

            // Color control
            SunLight :: Base :: stColor IntToColor( uint32_t color );

            // Graphics primitives miscellaneous
            // One straight edge of a shape: the engine's own line (IEngine::DrawLine).
            void DrawEdge( int nX0, int nY0, int nX1, int nY1, int nWidth, SunLight :: Base :: stColor color );

            // Engine primitives
            void DrawPolyline( double fOffset_x,
                               double fOffset_y,
                               double **fPoints,
                               int nPointsCount,
                               double fRotation,
                               int nLineWidth,
                               SunLight :: Base :: stColor color );
            void DrawPolygon( double fOffset_x,
                              double fOffset_y,
                              double **fPoints,
                              int nPointsCount,
                              double fRotation,
                              int nLineWidth,
                              SunLight :: Base :: stColor color );
            std :: vector<Shape :: ScreenPoint> ScreenPointsOf( double fOriginX,
                                                                          double fOriginY,
                                                                          double **fPoints,
                                                                          int nPointsCount,
                                                                          double fZoom,
                                                                          double fRotation );
            void DrawRectangle( double offset_x,
                                double offset_y,
                                double width,
                                double height,
                                double fRotation,
                                int nLineWidth,
                                SunLight :: Base :: stColor color );
            void DrawPoint( double offset_x, double offset_y, double size, SunLight :: Base :: stColor color );
            void DrawEllipse( double offset_x,
                              double offset_y,
                              double width,
                              double height,
                              double fRotation,
                              int nLineWidth,
                              SunLight :: Base :: stColor color );
            void DrawTile( SunLight :: Base :: TextureHandle pImage,
                           int32_t nSourceX,
                           int32_t nSourceY,
                           int32_t nSourceW,
                           int32_t nSourceH,
                           int32_t nDestX,
                           int32_t nDestY,
                           float fOpacity,
                           SunLight :: Base :: stColor tintColor = { 0xFF, 0xFF, 0xFF, 0xFF } );

            // High level primitive map handlers
            void DrawObjects( tmx_layer *pLayer );

            // Draws one text object (B1). See DrawTextObject's own comment for
            // the placeholder used when its font is not registered.
            void DrawTextObject( tmx_object *pObject, tmx_layer *pLayer );

            // Draws one tile object (B2): the tile its gid names, at the object's position.
            void DrawTileObject( tmx_object *pObject, tmx_layer *pLayer );
            SunLight :: Font :: IFont* FindRegisteredFont( const char *szFamily, bool bBold, bool bItalic );
            void WarnMissingFont( const char *szFamily, bool bBold, bool bItalic );
            void ReleaseRegisteredFonts( void );
            void DrawImageLayer( tmx_layer *pLayer );
            void DrawLayer( tmx_layer *pLayer );
            tmx_tile* AnimatedFrameOf( tmx_tile *pBase, unsigned int nGID );
            void DrawAllLayers( tmx_layer *pLayer, double fParentParallaxX = 1.0, double fParentParallaxY = 1.0 );
            void RenderMap( void );

            // L1 of the master plan: the effective camera for a layer whose own accumulated
            // parallax factor (its own parallaxx/y, already multiplied by every group ancestor's
            // own factor) is fFactorX/fFactorY - m_CameraPos itself, anchored at the map's own
            // parallaxoriginx/y, scaled by the factor; exactly m_CameraPos, per axis, when that
            // axis's factor is 1.0 (byte-identical to this project's pre-L1 behavior).
            stVector GetEffectiveCameraForFactor( double fFactorX, double fFactorY ) const;

            // Object management internals
            bool FindObjectByName( tmx_layer *pLayer, const char *szObjectName, SunLight :: TileMap :: stObject& object );

            // User interaction handlers
            inline void HandleTileMapUpdate( void );
            inline void HandleUserCollisions( void );
            inline void HandleSpriteUpdate( int nLayerId );
            SunLight :: Base :: stColor BackgroundColorOf( View &view );
            void DrawViewPasses( void );

            // Internal layer handlers
            void CopyLayerToTmx( tmx_layer *pTmxLayer, SunLight :: TileMap :: stLayer& layer );
            void CopyTmxToLayer( SunLight :: TileMap :: stLayer& layer, tmx_layer *pTmxLayer );


            public:

            TileMapRenderer( float fWidth,
                             float fHeight,
                             const char* szTitle,
                             int nTargetFps = -1 );
            explicit TileMapRenderer( const SunLight :: Renderer :: RendererConfig &config );
            virtual ~TileMapRenderer( void );

            // A sprite that is destroyed, or moved to another parent, calls this on the parent it is leaving.
            void ChildRemoved( SunLight :: Canvas :: BaseCanvas *pChild );

            // The camera of the view being drawn (the active view's), for world-space sprites -
            // scaled by nLayerId's own parallax factor (L1 of the master plan) when it names a
            // real layer (-1, BaseCanvas's own sentinel, means none: a loose canvas, or one never
            // added via AddSprite, gets the true, unscaled camera). Computed fresh on every call,
            // not from any per-frame draw-time state, so it answers correctly whether or not a
            // render pass has run yet.
            void GetCameraOffset( float &fX, float &fY, int nLayerId = -1 ) override;

            // Checked creation: validates the config (see
            // RendererConfig::Validate) and returns nullptr, with a message
            // in *pError, instead of building a renderer from a config that
            // can't work (e.g. a backend not compiled into this build).
            static std :: unique_ptr<TileMapRenderer> Create( const SunLight :: Renderer :: RendererConfig &config,
                                                              std :: string *pError = nullptr );

            // World user listener interaction
            void AddTileMapListener( SunLight :: TileMap :: ITileMapListener *pListener );
            void RemoveTileMapListener( SunLight :: TileMap :: ITileMapListener *pListener );

            // Window behavior
            void SetExitKey( SunLight :: Input :: KeyboardKey key );
            SunLight :: Input :: KeyboardKey GetExitKey( void );
            void SetWindowResizeable( bool bResizeable );
            bool GetWindowResizeable( void );
            void RequestExit( void );
            bool GetExitRequested( void );
            void SetTargetFPS( int nTargetFps );
            int  GetTargetFPS( void );
            void SetWindowBackgroundColor( SunLight :: Base :: stColor color );
            void SetClearBackground( bool bStatus );
            bool GetClearBackground( void );
            void SetDrawFPS( bool bDrawFPS );
            bool GetDrawFPS( void );
            void SetFullscreen( bool bFullscreen,
                               SunLight :: Window :: FullscreenStrategy strategy =
                                   SunLight :: Window :: FULLSCREEN_STRATEGY_REAL );
            bool GetFullscreen( void );
            SunLight :: Window :: FullscreenStrategy GetFullscreenStrategy( void );
            double GetElapsedTime( void );
            void SetStretchToFill( bool bStretchToFill );
            bool GetStretchToFill( void );

            // Views
            SunLight :: TileMap :: IView& GetDefaultView( void );
            std :: shared_ptr<SunLight :: TileMap :: IView> CreateView( const SunLight :: Base :: stDimension2D& rect );
            std :: shared_ptr<SunLight :: TileMap :: IView> GetView( int nViewId );
            bool RemoveView( int nViewId );
            bool RemoveView( const std :: shared_ptr<SunLight :: TileMap :: IView> &pView );
            int GetViewCount( void );

            // View port control
            void SetViewControlMode( SunLight :: Renderer :: ViewControlMode mode );
            void SetScrollStepSize( int nStepWidth, int nStepHeight );
            void GetScrollStepSize( int &nStepWidth, int &nStepHeight );

            // GamePad handling
            void AddGamePad( int nGamePadId );

            // User input handling           
            void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt, SunLight :: Input :: INPUT_EVENT_HANDLER handler );
            void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt, SunLight :: Input :: INPUT_EVENT_HANDLER handler );

            SunLight :: Input :: IInputHandler& GetInputHandler( void );

            // The controller manager these input methods delegate to (F0a of the master plan),
            // for direct access - the same shape as GetCollisionManager().
            SunLight :: Input :: IControllerManager& GetControllerManager( void );

            // Zoom wrappers
            void ResetZoom( void );
            void ZoomIn( void );
            void ZoomOut( void );

            // Camera management
            void ResetCamera( void );
            void MoveCameraUp( void );
            void MoveCameraDown( void );
            void MoveCameraLeft( void );
            void MoveCameraRight( void );
            void SetCameraPosition( int nX, int nY );
            void GetCameraPosition( int &nX, int &nY );

            // Window management
            void SetWindowTitle( const std :: string &strTitle );
            void SetScreenFade( float fAlpha );

            // Text rendering
            bool SetFont( const char *szFilePath );

            // Register a font for text objects: a text object whose fontfamily
            // and bold/italic flags match these draws with it. Needs a started
            // renderer (a font needs the window's render context). Registering
            // the same family and style again replaces the earlier font.
            bool RegisterFont( const char *szFamily, const char *szFilePath, bool bBold, bool bItalic );
            void DrawText( const char *szText,
                           int nPosX,
                           int nPosY,
                           int nFontSize,
                           unsigned char nRed,
                           unsigned char nGreen,
                           unsigned char nBlue,
                           unsigned char nAlpha );
            void DrawFilledRectangle( int nPosX,
                                      int nPosY,
                                      int nWidth,
                                      int nHeight,
                                      unsigned char nRed,
                                      unsigned char nGreen,
                                      unsigned char nBlue,
                                      unsigned char nAlpha );
            int MeasureText( const char *szText, int nFontSize );
            int GetWindowWidth( void );
            int GetWindowHeight( void );

            // Layer management
            bool SetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer );
            bool SetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer );
            bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer &layer );
            bool GetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer );

            // L1 of the master plan: set a layer's own parallax factor in code, overriding
            // whatever the TMX file itself declared (fFactorX/fFactorY 1.0 = no parallax).
            // Writes straight into the tmx_layer, the same pattern SetLayer already uses for
            // opacity/offset/visible - so, like those, the original TMX-authored value is not
            // recoverable once overridden.
            bool SetLayerParallax( int nLayerId, float fFactorX, float fFactorY );
            bool SetLayerParallax( const char *szLayerName, float fFactorX, float fFactorY );

            // Tile management
            bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                          const SunLight :: TileMap :: stLayer& layer,
                          SunLight :: TileMap :: stTile& tile );

            // Coordinate conversion
            bool TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                      SunLight :: TileMap :: stMatrixPosition& pos );

            // Map management
            bool LoadMap( const char *szTxMapFile, SunLight :: TileMap :: ITileMap :: MapAlignment alignment = MAP_ALIGNMENT_CENTER );
            bool UnloadMap( void );
            bool GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo );

            // Object management
            bool GetObjectByName( const char *szObjectName, SunLight :: TileMap :: stObject& object );

            // Collision management
            SunLight :: Collision :: ICollisionManager& GetCollisionManager( void );

            // Sprite management
            bool AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite );
            bool RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite );

            // Renderer flow control.
            bool Start( void );
            void Stop( void );
            bool Run( void );
        };
    }
}

#endif /* __TILEMAPRENDERER_H__ */
