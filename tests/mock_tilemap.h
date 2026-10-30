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

#ifndef __MOCK_TILEMAP_H__
#define __MOCK_TILEMAP_H__

#include "tilemap/itilemap.h"
#include <map>
#include <stdexcept>

/**
 * @brief Test double for SunLight::TileMap::ITileMap - the subset CollisionManager actually
 * calls (GetLayer, TileMapToTileMatrix, GetTile) has configurable results; the camera/zoom
 * methods (ResetZoom, ZoomIn, ZoomOut, ResetCamera, MoveCameraUp/Down/Left/Right) count their
 * own calls, for DefaultControllerBindings's tests; everything else is a trivial stub since
 * neither consumer reaches it. GetInputHandler()/GetCollisionManager() throw if called -
 * there's no sane mock value for either and neither consumer calls them on its parent.
 */
class MockTileMap : public SunLight :: TileMap :: ITileMap  {

    public:

    std :: map<int, SunLight :: TileMap :: stLayer>  layersById;

    bool bTileMapToTileMatrixResult = true;
    SunLight :: TileMap :: stMatrixPosition  tileMapToTileMatrixResult { 0, 0 };

    bool bGetTileResult = false;
    SunLight :: TileMap :: stTile  getTileResult {};

    int  nGetTileCalls              = 0;
    int  nTileMapToTileMatrixCalls  = 0;

    // GetCollidingTiles answers collidingTiles (for any rectangle) and records what it was asked.
    bool                                          bGetCollidingTilesResult = true;
    std :: vector<SunLight :: TileMap :: stTile>  collidingTiles;
    int                                           nGetCollidingTilesCalls  = 0;
    SunLight :: Base :: stRectangle               lastCollidingRect        { 0, 0, 0, 0 };
    bool                                          bLastCollidingWorldSpace = false;

    void AddGamePad( int )  {}
    void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey, SunLight :: Input :: INPUT_EVENT_HANDLER )  {}
    void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton, SunLight :: Input :: INPUT_EVENT_HANDLER )  {}

    SunLight :: Input :: IInputHandler& GetInputHandler( void )  {
        throw std :: logic_error( "MockTileMap::GetInputHandler not mocked" );
    }

    SunLight :: TileMap :: IView& GetDefaultView( void )  {
        throw std :: logic_error( "MockTileMap::GetDefaultView not mocked" );
    }
    std :: shared_ptr<SunLight :: TileMap :: IView> CreateView( const SunLight :: Base :: stDimension2D & )  { return nullptr; }
    std :: shared_ptr<SunLight :: TileMap :: IView> GetView( int )  { return nullptr; }
    bool RemoveView( int )  { return false; }
    bool RemoveView( const std :: shared_ptr<SunLight :: TileMap :: IView> & )  { return false; }
    int GetViewCount( void )  { return 1; }

    int  nResetZoomCalls      = 0;
    int  nZoomInCalls         = 0;
    int  nZoomOutCalls        = 0;
    int  nResetCameraCalls    = 0;
    int  nMoveCameraUpCalls   = 0;
    int  nMoveCameraDownCalls = 0;
    int  nMoveCameraLeftCalls = 0;
    int  nMoveCameraRightCalls = 0;

    void ResetZoom( void )  { nResetZoomCalls++; }
    void ZoomIn( void )  { nZoomInCalls++; }
    void ZoomOut( void )  { nZoomOutCalls++; }
    void ResetCamera( void )  { nResetCameraCalls++; }
    void MoveCameraUp( void )  { nMoveCameraUpCalls++; }
    void MoveCameraDown( void )  { nMoveCameraDownCalls++; }
    void MoveCameraLeft( void )  { nMoveCameraLeftCalls++; }
    void MoveCameraRight( void )  { nMoveCameraRightCalls++; }
    void SetCameraPosition( int, int )  {}

    void GetCameraPosition( int& nX, int& nY )  {
        nX = 0;
        nY = 0;
    }

    // SetWindowTitle/SetScreenFade/SetFullscreen/GetFullscreen/SetDrawFPS/
    // GetDrawFPS/SetWindowResizeable/GetWindowResizeable/SetStretchToFill/
    // GetStretchToFill/SetFont/DrawText/DrawFilledRectangle/MeasureText/
    // GetWindowWidth/GetWindowHeight moved off ITileMap onto the new
    // IDrawSurface (see it's own header comment) - no longer part of this
    // interface, so no longer stubbed here either.

    bool SetLayer( int, SunLight :: TileMap :: stLayer& )  {
        return false;
    }

    bool SetLayer( const char*, SunLight :: TileMap :: stLayer& )  {
        return false;
    }

    bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer )  {
        std :: map<int, SunLight :: TileMap :: stLayer> :: iterator itItem = layersById.find( nLayerId );

        if( itItem == layersById.end() )
            return false;

        layer = itItem -> second;
        return true;
    }

    bool GetLayer( const char*, SunLight :: TileMap :: stLayer& )  {
        return false;
    }

    bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                  const SunLight :: TileMap :: stLayer& layer,
                  SunLight :: TileMap :: stTile& tile )  {
        nGetTileCalls++;
        tile = getTileResult;
        return bGetTileResult;
    }

    bool GetCollidingTiles( const SunLight :: Base :: stRectangle& rect, bool bWorldSpace,
                            const SunLight :: TileMap :: stLayer& layer,
                            std :: vector<SunLight :: TileMap :: stTile>& tiles )  {
        nGetCollidingTilesCalls++;
        lastCollidingRect        = rect;
        bLastCollidingWorldSpace = bWorldSpace;
        tiles = collidingTiles;
        return bGetCollidingTilesResult;
    }

    bool TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D&,
                              SunLight :: TileMap :: stMatrixPosition& pos )  {
        nTileMapToTileMatrixCalls++;
        pos = tileMapToTileMatrixResult;
        return bTileMapToTileMatrixResult;
    }

    bool GetMapInfo( SunLight :: TileMap :: stMapInfo& )  {
        return false;
    }

    bool GetObjectByName( const char*, SunLight :: TileMap :: stObject& )  {
        return false;
    }

    SunLight :: Collision :: ICollisionManager& GetCollisionManager( void )  {
        throw std :: logic_error( "MockTileMap::GetCollisionManager not mocked" );
    }

    bool AddSprite( int, SunLight :: Sprite :: Sprite& )  {
        return false;
    }

    bool RemoveSprite( int, SunLight :: Sprite :: Sprite& )  {
        return false;
    }

    bool LoadMap( const char*, SunLight :: TileMap :: ITileMap :: MapAlignment )  {
        return false;
    }

    bool UnloadMap( void )  {
        return false;
    }
};

#endif /* __MOCK_TILEMAP_H__ */
