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
#include "collision/collisionmanager.h"
#include "tilemap/itilemaplistener.h"
#include "input/iinputhandler.h"


namespace SunLight {
    namespace Renderer  {

        /**
         * Primitive structures definition.
         */
        typedef Vector2    stVector;

        /**
         * View port control mode (active and reactive)
         * Active, the view port reacts to a single key pressing continuously;
         * Reactive, the view port reacts only for each key pressing;
         */
        enum ViewControlMode  {
            VIEW_CONTROL_MODE_ACTIVE,
            VIEW_CONTROL_MODE_REACTIVE
        };

        /**
         * @brief World renderer implementation to render TMX based maps.
         */
        class TileMapRenderer : public SunLight :: TileMap :: ITileMap  {

            /**
             * Tile animation information.
             */
            struct __stTileAnimInfo  {
                int64_t      nMillis;
                uint16_t     nCounter;
                tmx_tile     *pNextTile;
            };

            /**
             * @brief Input event data struct info
             */
            struct __stInputEventData  {
                int                                       nEvent;
                SunLight :: Input :: INPUT_EVENT_HANDLER  pHandler;
            };

            typedef std :: deque<__stTileAnimInfo*> __AnimInfoList;
            typedef std :: deque<SunLight :: TileMap :: ITileMapListener*> TileMapListenerList;
            typedef std :: deque<__stInputEventData*> InputEventHandlerList;
            typedef std :: deque<SunLight :: Sprite :: Sprite*> SpriteList;
            typedef std :: deque<int> GamePadList;

            SunLight :: Input :: IInputHandler         *m_pInputHandler;
            GamePadList                                m_GamePadList;
            SpriteList                                 m_SpriteList;
            TileMapListenerList                        m_TileMapListenerList;
            InputEventHandlerList                      m_KeyInputEventHandlerList;
            InputEventHandlerList                      m_GPadInputEventHandlerList;
            SunLight :: Input :: INPUT_EVENT_HANDLER   m_pNullInputEventHandler;
            SunLight :: Collision :: CollisionManager  m_CollisionManager;
            stVector                                   m_CameraPos;
            ViewControlMode                            m_ViewControlMode;
            uint16_t                                   m_nMapWidth;
            uint16_t                                   m_nMapHeight;
            int                                        m_nScrollStepWidth;
            int                                        m_nScrollStepHeight;
            float                                      m_fWindowWidth;
            float                                      m_fWindowHeight;
            int                                        m_nTargetFps;
            uint32_t                                   m_nWindowBackgroundColor;
            __AnimInfoList                             m_AnimInfoList;
            std :: string                              m_strTitle;
            tmx_map                                    *m_pTmxMap;
            bool                                       m_bClearBackground;
            bool                                       m_bIsStarted;
            bool                                       m_bWindowResizeable;
            bool                                       m_bDrawFPS;
            static bool                                m_bInitialized;

            // TmxLib overrides
            static void* TextureLoaderCallback( const char *szFileName );
            static void TextureFreeCallback( void *pTexture );

            // TmxLib miscellaneous
            tmx_tileset_list* GetTilesetList( tmx_map *pMap, tmx_tileset *pTilesetSearch );
            tmx_layer* GetLayer( int nLayerId );
            tmx_layer* GetLayer( const char *szLayerName );

            // Color control
            Color IntToColor( uint32_t color );

            // Graphics primitives miscellaneous
            void SetPixel( int nCoordX, int nCoordY, Color color );
            void MidPointEllipse( double fCoordX,
                                  double fCoordY,
                                  double fRadiusX,
                                  double fRadiusY,
                                  Color color );
            void LineBresenham( int nX0,
                                int nY0,
                                int nX1,
                                int nY1,
                                Color color );

            // Engine primitives
            void DrawPolyline( double fOffset_x,
                               double fOffset_y,
                               double **fPoints,
                               int nPointsCount,
                               Color color );
            void DrawPolygon( double fOffset_x,
                              double fOffset_y,
                              double **fPoints,
                              int nPointsCount,
                              Color color );
            void DrawRectangle( double offset_x,
                                double offset_y,
                                double width,
                                double height,
                                Color color );
            void DrawEllipse( double offset_x,
                              double offset_y,
                              double width,
                              double height,
                              Color color );
            void DrawTile( void *pImage,
                           int32_t nSourceX,
                           int32_t nSourceY,
                           int32_t nSourceW,
                           int32_t nSourceH,
                           int32_t nDestX,
                           int32_t nDestY,
                           float fOpacity );

            // High level primitive map handlers
            void DrawObjects( tmx_layer *pLayer );
            void DrawImageLayer( tmx_layer *pLayer );
            void DrawLayer( tmx_map *pMap, tmx_layer *pLayer );
            void DrawAllLayers( tmx_map *pMap, tmx_layer *pLayer );
            void RenderMap( void );

            // General engine handlers
            void InitalizeDefaultUserInputHandlers( void );

            // Default Input handlers callbacks
            void InputKeyUp( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyDown( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyLeft( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyRight( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyPageUp( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyPageDown( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyHome( SunLight :: Input :: ControllerType type, int nId );
            void InputKeyEnd( SunLight :: Input :: ControllerType type, int nId );

            // User interaction handlers
            inline void HandleKeyEvent( SunLight :: Input :: KeyboardKey key );
            inline void HandleGamePadEvent( SunLight :: Input :: GamepadButton key );
            inline void HandleUserInput( void );
            inline void HandleUserUpdate( void );
            inline void HandleUserCollisions( void );

            // Internal layer handlers
            void CopyLayerToTmx( tmx_layer *pTmxLayer, SunLight :: TileMap :: stLayer& layer );
            void CopyTmxToLayer( SunLight :: TileMap :: stLayer& layer, tmx_layer *pTmxLayer );


            public:

            TileMapRenderer( float fWidth,
                             float fHeight,
                             const char* szTitle,
                             int nTargetFps = -1,
                             bool bUseDefaultKeyHandler = true );
            virtual ~TileMapRenderer( void );

            // World user listener interaction
            void AddTileMapListener( SunLight :: TileMap :: ITileMapListener *pListener );
            void RemoveTileMapListener( SunLight :: TileMap :: ITileMapListener *pListener );

            // Window behavior
            void SetExitKey( KeyboardKey key );
            void SetWindowResizeable( bool bResizeable );
            void SetWindowBackgroundColor( uint32_t nWindowBkColor );
            void SetClearBackground( bool bStatus );
            void SetDrawFPS( bool bDrawFPS );

            // View port control
            void SetViewControlMode( SunLight :: Renderer :: ViewControlMode mode );
            void SetScrollStepSize( int nStepWidth, int nStepHeight );

            // GamePad handling
            void AddGamePad( int nGamePadId );

            // User input handling           
            void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt, SunLight :: Input :: INPUT_EVENT_HANDLER handler );
            void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt, SunLight :: Input :: INPUT_EVENT_HANDLER handler );

            SunLight :: Input :: IInputHandler& GetInputHandler( void );
       
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

            // Layer management
            bool SetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer );
            bool SetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer );
            bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer &layer );
            bool GetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer );

            // Tile management
            bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                          const SunLight :: TileMap :: stLayer& layer,
                          SunLight :: TileMap :: stTile& tile );

            // Coordinate conversion
            bool TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                      SunLight :: TileMap :: stMatrixPosition& pos );

            // Map management
            bool LoadMap( const char *szTxMapFile, SunLight :: TileMap :: ITileMap :: MapAlignment alignment = MAP_ALIGNMENT_CENTER );
            bool UnloadMap( void );
            bool GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo );

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
