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
 * @brief Test double for SunLight::TileMap::ITileMap - only implements the
 * subset CollisionManager actually calls (GetLayer, TileMapToTileMatrix,
 * GetTile) with configurable results; everything else is a trivial stub
 * since CollisionManager never reaches it. GetInputHandler()/
 * GetCollisionManager() throw if called - there's no sane mock value for
 * either and CollisionManager doesn't call them on its parent.
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

    void AddGamePad( int )  {}
    void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey, SunLight :: Input :: INPUT_EVENT_HANDLER )  {}
    void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton, SunLight :: Input :: INPUT_EVENT_HANDLER )  {}

    SunLight :: Input :: IInputHandler& GetInputHandler( void )  {
        throw std :: logic_error( "MockTileMap::GetInputHandler not mocked" );
    }

    void ResetZoom( void )  {}
    void ZoomIn( void )  {}
    void ZoomOut( void )  {}
    void ResetCamera( void )  {}
    void MoveCameraUp( void )  {}
    void MoveCameraDown( void )  {}
    void MoveCameraLeft( void )  {}
    void MoveCameraRight( void )  {}
    void SetWindowTitle( const std :: string& )  {}

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

    bool TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D&,
                              SunLight :: TileMap :: stMatrixPosition& pos )  {
        nTileMapToTileMatrixCalls++;
        pos = tileMapToTileMatrixResult;
        return bTileMapToTileMatrixResult;
    }

    bool GetMapInfo( SunLight :: TileMap :: stMapInfo& )  {
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
