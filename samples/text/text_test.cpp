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
#include "renderer/tilemaprenderer.h"

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
    // Dark red around the viewport; the viewport itself keeps the map's dark green.
    const SunLight :: Base :: stColor  __WINDOW_BACKGROUND_COLOR { 0x30, 0x10, 0x10, 0xFF };
}

int main( int argc, char **argv ) {

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
    const struct  { const char *szFamily; const char *szFile; bool bBold; bool bItalic; }  faces[] = {
        { "Sans",  "resources/fonts/caravellius8x8.fnt",           false, false },
        { "Sans",  "resources/fonts/caravellius8x8_bold.fnt",      true,  false },
        { "Mono",  "resources/fonts/AnonymousPro-Regular.ttf",     false, false },
        { "Mono",  "resources/fonts/AnonymousPro-Bold.ttf",        true,  false },
        { "Mono",  "resources/fonts/AnonymousPro-Italic.ttf",      false, true  },
        { "Mono",  "resources/fonts/AnonymousPro-BoldItalic.ttf", true,  true  },
        { "Pixel", "resources/fonts/pressstart2p-regular.ttf",    false, false },
    };

    for( const auto &face : faces )  {
        if( !pRenderer -> RegisterFont( face.szFamily, face.szFile, face.bBold, face.bItalic ) )
            fprintf( stderr, "Cannot load %s (%s)\n", face.szFile, face.szFamily );
    }

    pRenderer -> SetCameraPosition( __START_CAMERA_X, __START_CAMERA_Y );

    pRenderer -> SetWindowBackgroundColor( __WINDOW_BACKGROUND_COLOR );

    printf( "Arrows scroll the map. Watch each label cut at the edge it crosses.\n" );
    printf( "Esc quits.\n" );

    pRenderer -> Run();
    pRenderer -> Stop();

    return EXIT_SUCCESS;
}
