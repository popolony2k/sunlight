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
#include <utility>
#include <vector>

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
    std :: vector<std :: pair<int, int>>  setPixelPositions;          // every SetPixel( x, y ), in order
    int                                 nDrawTextureCalls          = 0;
    int                                 nDrawTextureTiledCalls     = 0;
    int                                 nDrawFilledRectangleCalls  = 0;
    int                                 nGetApplicationDirectoryCalls = 0;
    int                                 nClearBackgroundCalls      = 0;
    int                                 nDrawFPSCalls              = 0;
    int                                 nSetFontCalls              = 0;
    int                                 nDrawTextCalls             = 0;
    int                                 nMeasureTextCalls          = 0;
    int                                 nMeasureTextResult         = 0;
    int                                 nLoadRenderTargetCalls     = 0;
    int                                 nUnloadRenderTargetCalls   = 0;
    int                                 nBeginRenderTargetCalls    = 0;
    int                                 nEndRenderTargetCalls      = 0;
    int                                 nDrawTextureScaledCalls    = 0;

    // Every draw-ish call in order, for tests that care about WHICH pass drew what and in what order
    // (the multi-view frame): kind + the rectangle it was given (x, y, w, h; zeros where it has none).
    struct Event  {
        enum Kind  { CLEAR, FILL, TILE, FPS };

        Kind   kind;
        float  x, y, w, h;
        float  scale;
        float  srcX;                 // TILE only: the source rectangle's x
        void   *handle;              // TILE only: the texture drawn
        SunLight :: Base :: stColor  color;   // CLEAR and FILL only
    };
    std :: vector<Event>                events;

    std :: string                       strLastLoadTextureFileName;
    SunLight :: Base :: TextureHandle   hLastUnloadedTexture = nullptr;
    SunLight :: Base :: TextureHandle   hLastDrawnTexture    = nullptr;
    SunLight :: Base :: stRectangle     lastDrawTextureTiledSource { 0, 0, 0, 0 };
    SunLight :: Base :: stRectangle     lastDrawTextureTiledDest   { 0, 0, 0, 0 };
    float                               fLastDrawTextureTiledScale = 0.0f;
    int                                 nLastFilledRectangleX      = 0;
    int                                 nLastFilledRectangleY      = 0;
    int                                 nLastFilledRectangleWidth  = 0;
    int                                 nLastFilledRectangleHeight = 0;
    SunLight :: Base :: stColor         lastFilledRectangleColor   { 0, 0, 0, 0 };
    bool                                bSetFontResult             = true;
    std :: string                       strLastSetFontPath;
    std :: string                       strLastDrawnText;
    SunLight :: Base :: stColor         lastClearBackgroundColor   { 0, 0, 0, 0 };
    SunLight :: Base :: TextureHandle   hLoadRenderTargetResult    = ( SunLight :: Base :: TextureHandle )  0x2;
    SunLight :: Base :: TextureHandle   hLastUnloadedRenderTarget  = nullptr;
    SunLight :: Base :: TextureHandle   hLastBegunRenderTarget     = nullptr;
    SunLight :: Base :: stRectangle     lastDrawTextureScaledSource { 0, 0, 0, 0 };
    SunLight :: Base :: stRectangle     lastDrawTextureScaledDest   { 0, 0, 0, 0 };

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
        setPixelPositions.push_back( std :: make_pair( nPosX, nPosY ) );
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
        events.push_back( Event { Event :: TILE, dest.x, dest.y, dest.width, dest.height, scale, source.x, hTexture, SunLight :: Base :: stColor { 0, 0, 0, 0 } } );
        hLastDrawnTexture = hTexture;
        lastDrawTextureTiledSource = source;
        lastDrawTextureTiledDest   = dest;
        fLastDrawTextureTiledScale = scale;
    }

    void DrawFilledRectangle( int nPosX, int nPosY, int nWidth, int nHeight, SunLight :: Base :: stColor color )  {
        nDrawFilledRectangleCalls++;
        events.push_back( Event { Event :: FILL, ( float ) nPosX, ( float ) nPosY, ( float ) nWidth, ( float ) nHeight, 0.0f, 0.0f, nullptr, color } );
        nLastFilledRectangleX      = nPosX;
        nLastFilledRectangleY      = nPosY;
        nLastFilledRectangleWidth  = nWidth;
        nLastFilledRectangleHeight = nHeight;
        lastFilledRectangleColor   = color;
    }

    std :: string GetApplicationDirectory( void )  {
        nGetApplicationDirectoryCalls++;
        return strApplicationDirectoryResult;
    }

    bool SetFont( const char *szFilePath )  {
        nSetFontCalls++;
        strLastSetFontPath = szFilePath;
        return bSetFontResult;
    }

    void DrawText( const char *szText, int, int, int, SunLight :: Base :: stColor )  {
        nDrawTextCalls++;
        strLastDrawnText = szText;
    }

    int MeasureText( const char*, int )  {
        nMeasureTextCalls++;
        return nMeasureTextResult;
    }

    void ClearBackground( SunLight :: Base :: stColor color )  {
        nClearBackgroundCalls++;
        events.push_back( Event { Event :: CLEAR, 0, 0, 0, 0, 0.0f, 0.0f, nullptr, color } );
        lastClearBackgroundColor = color;
    }

    void DrawFPS( int, int )  {
        nDrawFPSCalls++;
        events.push_back( Event { Event :: FPS, 0, 0, 0, 0, 0.0f, 0.0f, nullptr, SunLight :: Base :: stColor { 0, 0, 0, 0 } } );
    }

    SunLight :: Base :: TextureHandle LoadRenderTarget( int nWidth, int nHeight )  {
        nLoadRenderTargetCalls++;
        return hLoadRenderTargetResult;
    }

    void UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget )  {
        nUnloadRenderTargetCalls++;
        hLastUnloadedRenderTarget = hRenderTarget;
    }

    void BeginRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget )  {
        nBeginRenderTargetCalls++;
        hLastBegunRenderTarget = hRenderTarget;
    }

    void EndRenderTarget( void )  {
        nEndRenderTargetCalls++;
    }

    SunLight :: Base :: TextureHandle GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget )  {
        return hRenderTarget;
    }

    void DrawTextureScaled( SunLight :: Base :: TextureHandle hTexture,
                            SunLight :: Base :: stRectangle source,
                            SunLight :: Base :: stRectangle dest,
                            SunLight :: Base :: stColor tint )  {
        nDrawTextureScaledCalls++;
        hLastDrawnTexture           = hTexture;
        lastDrawTextureScaledSource = source;
        lastDrawTextureScaledDest   = dest;
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
