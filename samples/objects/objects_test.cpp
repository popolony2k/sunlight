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
#include "renderer/tilemaprenderer.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // The viewport is 1000 x 800 px at (40, 40). The camera starts at (100, 100), so the
    // viewport shows the map from x = 100 to 1100 and y = 100 to 900 - the rectangle drawn
    // as "frame" in the map. The tile objects near those edges are cut by them.
    const int  __VIEWPORT_POS_X  = 40;
    const int  __VIEWPORT_POS_Y  = 40;
    const int  __VIEWPORT_WIDTH  = 1000;
    const int  __VIEWPORT_HEIGHT = 800;
    const int  __START_CAMERA_X  = 100;
    const int  __START_CAMERA_Y  = 100;
}

int main( int argc, char **argv ) {

    if( argc < 2 )  {
        fprintf( stderr, "Invalid command line arguments: objects_test <sample directory>\n" );
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

    config.fWidth     = 1260.0f;
    config.fHeight    = 920.0f;
    config.strTitle   = "Tile objects: size, edges, animation and flip";
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

    if( !pRenderer -> LoadMap( "resources/map/objects.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot load resources/map/objects.tmx\n" );
        return EXIT_FAILURE;
    }

    // The labels are in "Sans", the regular face of Caravellius 8x8 (a bitmap font).
    if( !pRenderer -> RegisterFont( "Sans", "/shared/fonts/caravellius8x8.fnt", false, false ) )
        fprintf( stderr, "Cannot load the Sans font\n" );

    pRenderer -> SetCameraPosition( __START_CAMERA_X, __START_CAMERA_Y );

    printf( "Each numbered row is one tile object case. Arrows scroll the map, Esc quits.\n" );

    pRenderer -> Run();
    pRenderer -> Stop();

    return EXIT_SUCCESS;
}
