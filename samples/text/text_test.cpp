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

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include "filesystem/filesystemfactory.h"
#include "input/defaultcontrollerbindings.h"
#include "renderer/tilemaprenderer.h"
#include "tilemap/itilemaplistener.h"
#include "../shared/zoomkeys.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // The viewport is 1000 x 800 px at (40, 40). The camera starts at (100, 100), so the
    // viewport shows the map from x = 100 to 1100 and y = 100 to 900 - the rectangle drawn
    // as "frame" in the map. Labels near those edges are cut by them.
    const int  __VIEWPORT_POS_X  = 40;
    const int  __VIEWPORT_POS_Y  = 40;
    const int  __VIEWPORT_WIDTH  = 1000;
    const int  __VIEWPORT_HEIGHT = 800;
    const int  __START_CAMERA_X  = 100;
    const int  __START_CAMERA_Y  = 100;

    // The HUD line under the viewport, in screen pixels (DrawText: no viewport or camera).
    const int            __HUD_X         = __VIEWPORT_POS_X;
    const int            __HUD_Y         = __VIEWPORT_POS_Y + __VIEWPORT_HEIGHT + 24;
    const int            __HUD_FONT_SIZE = 20;
    const unsigned char  __OPAQUE        = 0xFF;

    // The face the HUD shares with the map's Mono labels (I3 of the master plan).
    const char* const    __HUD_FACE      = "/shared/fonts/AnonymousPro-Regular.ttf";

    // Draws the HUD line every frame, inside the frame, with the active font (SetFont).
    struct stHud : public SunLight :: TileMap :: ITileMapListener  {

        TileMapRenderer  *pRenderer = nullptr;

        void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  {
            pRenderer -> DrawText( "HUD: the same Anonymous Pro the Mono labels use, loaded once (LoadFont, SetFont, RegisterFont)",
                                   __HUD_X, __HUD_Y, __HUD_FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );
        }

        void OnStop( void ) override  {}
    };
}

int main( int argc, char** argv )  {

    if( argc < 2 )  {
        fprintf( stderr, "Invalid command line arguments: text_test <sample directory>\n" );
        return EXIT_FAILURE;
    }

    // Resources are loaded by name relative to the sample's own directory, so enter it first.
    std :: error_code  errorCode;

    std :: filesystem :: current_path( argv[1], errorCode );

    if( errorCode )  {
        fprintf( stderr, "Cannot enter the sample directory [%s]: %s\n", argv[1], errorCode.message().c_str() );
        return EXIT_FAILURE;
    }

    // The fonts every sample shares live in samples/shared/fonts. Mounting anything turns off the
    // automatic mount of the working directory, so this sample's own folder is mounted too.
    SunLight :: FileSystem :: IFileSystem  &fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

    if( !fileSystem.Mount( ".", "/", true ) || !fileSystem.Mount( "../shared", "/shared", true ) )  {
        fprintf( stderr, "Cannot mount the sample resources\n" );
        return EXIT_FAILURE;
    }

    RendererConfig  config;

    config.fWidth   = 1260.0f;
    config.fHeight  = 920.0f;
    config.strTitle = "Text objects and clipping";
    config.nTargetFps = 60;
    config.viewport.emplace();
    config.viewport -> pos.x        = __VIEWPORT_POS_X;
    config.viewport -> pos.y        = __VIEWPORT_POS_Y;
    config.viewport -> size.nWidth  = __VIEWPORT_WIDTH;
    config.viewport -> size.nHeight = __VIEWPORT_HEIGHT;

    std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

    if( pRenderer == nullptr )  {
        fprintf( stderr, "Cannot create the renderer\n" );
        return EXIT_FAILURE;
    }

    if( !pRenderer -> Start() )  {
        fprintf( stderr, "Cannot start the renderer\n" );
        return EXIT_FAILURE;
    }

    if( !pRenderer -> LoadMap( "resources/map/text.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot load resources/map/text.tmx\n" );
        return EXIT_FAILURE;
    }

    // Sans: regular and bold, both bitmap fonts designed for Caravellius (no italic face).
    // Mono: all four faces of Anonymous Pro. Pixel: Press Start 2P (regular only).
    // "Sans italic" and "Serif" are deliberately not registered, so their labels show
    // ?????? (and a warning on stderr) - that is the behaviour under test.
    const struct  { const char* szFamily; const char* szFile; bool bBold; bool bItalic; }  faces[] = {
        { "Sans",  "/shared/fonts/caravellius8x8.fnt",           false, false },
        { "Sans",  "/shared/fonts/caravellius8x8_bold.fnt",      true,  false },
        { "Mono",  "/shared/fonts/AnonymousPro-Regular.ttf",     false, false },
        { "Mono",  "/shared/fonts/AnonymousPro-Bold.ttf",        true,  false },
        { "Mono",  "/shared/fonts/AnonymousPro-Italic.ttf",      false, true  },
        { "Mono",  "/shared/fonts/AnonymousPro-BoldItalic.ttf", true,  true  },
        { "Pixel", "/shared/fonts/pressstart2p-regular.ttf",    false, false },
    };

    // Each face is loaded once (LoadFont) and registered by handle. The Mono regular face is
    // also selected for the HUD line (SetFont): one font, two uses, no second load.
    for( const auto& face : faces )  {
        std :: shared_ptr<SunLight :: Font :: IFont>  pFont = pRenderer -> LoadFont( face.szFile );

        if( !pRenderer -> RegisterFont( face.szFamily, pFont, face.bBold, face.bItalic ) )
            fprintf( stderr, "Cannot load %s (%s)\n", face.szFile, face.szFamily );

        if( ( pFont != nullptr ) && ( std :: string( face.szFile ) == __HUD_FACE ) )
            pRenderer -> SetFont( pFont );
    }

    stHud  hud;

    hud.pRenderer = pRenderer.get();
    pRenderer -> AddTileMapListener( &hud );

    pRenderer -> SetCameraPosition( __START_CAMERA_X, __START_CAMERA_Y );

    // The arrow keys scroll and PageUp/PageDown/Home/End zoom and reset - the renderer no longer
    // installs these itself (F0b of the master plan), so a sample that wants them asks for them.
    SunLight :: Input :: DefaultControllerBindings :: Install( *pRenderer, pRenderer -> GetControllerManager() );

    // + zooms in, - zooms out and = resets the zoom, on the keyboard and on the keypad - the same in every
    // sample that zooms (samples/shared/zoomkeys.h), next to Page Up / Page Down / Home.
    Samples :: InstallZoomKeys( *pRenderer,
                                [&pRenderer]() { pRenderer -> ZoomIn(); },
                                [&pRenderer]() { pRenderer -> ZoomOut(); },
                                [&pRenderer]() { pRenderer -> ResetZoom(); } );

    pRenderer -> SetWindowBackgroundColor( DARK_RED_COLOR );   // around the viewport; the viewport keeps the map's dark green

    printf( "Arrows scroll the map. Watch each label cut at the edge it crosses.\n" );
    printf( "Esc quits.\n" );

    pRenderer -> Run();
    pRenderer -> Stop();

    return EXIT_SUCCESS;
}
