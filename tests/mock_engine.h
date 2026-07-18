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

#ifndef __MOCK_ENGINE_H__
#define __MOCK_ENGINE_H__

#include "engines/iengine.h"
#include "engines/enginefactory.h"
#include <string>

/**
 * @brief Test double for SunLight::Engines::IEngine - records call counts/
 * arguments and returns per-instance, configurable results, with no real
 * window/render context involved.
 */
class MockEngine : public SunLight :: Engines :: IEngine  {

    public:

    SunLight :: Base :: TextureHandle  hLoadTextureResult = ( SunLight :: Base :: TextureHandle )  0x1;
    int                                 nLoadTextureWidth  = 0;
    int                                 nLoadTextureHeight = 0;
    std :: string                       strApplicationDirectoryResult;

    int                                 nLoadTextureCalls          = 0;
    int                                 nUnloadTextureCalls        = 0;
    int                                 nSetPixelCalls             = 0;
    int                                 nDrawTextureCalls          = 0;
    int                                 nDrawTextureTiledCalls     = 0;
    int                                 nGetApplicationDirectoryCalls = 0;

    std :: string                       strLastLoadTextureFileName;
    SunLight :: Base :: TextureHandle   hLastUnloadedTexture = nullptr;
    SunLight :: Base :: TextureHandle   hLastDrawnTexture    = nullptr;
    SunLight :: Base :: stRectangle     lastDrawTextureTiledSource { 0, 0, 0, 0 };
    SunLight :: Base :: stRectangle     lastDrawTextureTiledDest   { 0, 0, 0, 0 };
    float                               fLastDrawTextureTiledScale = 0.0f;

    SunLight :: Base :: TextureHandle LoadTexture( const char *szFileName, int& nWidth, int& nHeight )  {
        nLoadTextureCalls++;
        strLastLoadTextureFileName = szFileName;
        nWidth  = nLoadTextureWidth;
        nHeight = nLoadTextureHeight;
        return hLoadTextureResult;
    }

    void UnloadTexture( SunLight :: Base :: TextureHandle hTexture )  {
        nUnloadTextureCalls++;
        hLastUnloadedTexture = hTexture;
    }

    void SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color )  {
        nSetPixelCalls++;
    }

    void DrawTexture( SunLight :: Base :: TextureHandle hTexture, int nPosX, int nPosY, SunLight :: Base :: stColor tint )  {
        nDrawTextureCalls++;
        hLastDrawnTexture = hTexture;
    }

    void DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                            SunLight :: Base :: stRectangle source,
                            SunLight :: Base :: stRectangle dest,
                            SunLight :: Base :: stVector2D origin,
                            float rotation,
                            float scale,
                            SunLight :: Base :: stColor tint )  {
        nDrawTextureTiledCalls++;
        hLastDrawnTexture = hTexture;
        lastDrawTextureTiledSource = source;
        lastDrawTextureTiledDest   = dest;
        fLastDrawTextureTiledScale = scale;
    }

    std :: string GetApplicationDirectory( void )  {
        nGetApplicationDirectoryCalls++;
        return strApplicationDirectoryResult;
    }
};

/**
 * @brief RAII fixture that points EngineFactory :: GetEngine() at a single
 * MockEngine instance for the fixture's lifetime, restoring the default
 * (real) backend on destruction.
 */
class MockEngineFixture  {

    public:

    MockEngine  engine;

    MockEngineFixture( void )  {
        SunLight :: Engines :: EngineFactory :: SetEngine( &engine );
    }

    ~MockEngineFixture( void )  {
        SunLight :: Engines :: EngineFactory :: SetEngine( nullptr );
    }
};

#endif /* __MOCK_ENGINE_H__ */
