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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include "filesystem/filesystemfactory.h"
#include "renderer/tilemaprenderer.h"
#include "base/viewport.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // The viewport is 1000 x 800 px at (40, 40). The camera starts at (0, 0), so the
    // viewport shows the map from x = 100 to 1100 and y = 100 to 900 - the rectangle drawn
    // as "frame" in the map. The tile objects near those edges are cut by them.
    const int  __VIEWPORT_POS_X  = 40;
    const int  __VIEWPORT_POS_Y  = 40;
    const int  __VIEWPORT_WIDTH  = 1000;
    const int  __VIEWPORT_HEIGHT = 800;
    const int  __START_CAMERA_X  = 0;
    const int  __START_CAMERA_Y  = 0;

    // Command line: shapes_test <sample directory> [map file] [frames] [fps] [zoom]
    const int          __ARG_DIRECTORY = 1;
    const int          __ARG_MAP       = 2;
    const int          __ARG_FRAMES    = 3;
    const int          __ARG_FPS       = 4;
    const int          __ARG_ZOOM      = 5;
    const char * const __DEFAULT_MAP   = "shapes.tmx";
    const char * const __MAP_FOLDER    = "resources/map/";
    const int          __DEFAULT_FPS   = 60;
}

/**
 * Shows the shapes map. With a frame count it benchmarks instead: runs that many frames and
 * prints the average time per frame. Pass fps 0 to lift the frame cap, so the time measures
 * the work rather than the pacing.
 */
int main( int argc, char **argv ) {

    if( argc <= __ARG_DIRECTORY )  {
        fprintf( stderr, "Invalid command line arguments: shapes_test <sample directory> [map file] [frames] [fps] [zoom]\n" );
        return EXIT_FAILURE;
    }

    // Resources are loaded by name relative to the sample's own directory, so enter it first.
    std :: error_code  errorCode;

    std :: string  mapFile = ( argc > __ARG_MAP ) ? argv[__ARG_MAP] : __DEFAULT_MAP;
    int            frames  = ( argc > __ARG_FRAMES ) ? atoi( argv[__ARG_FRAMES] ) : 0;
    int            fps     = ( argc > __ARG_FPS ) ? atoi( argv[__ARG_FPS] ) : __DEFAULT_FPS;
    double         fZoom   = ( argc > __ARG_ZOOM ) ? atof( argv[__ARG_ZOOM] ) : 1.0;

    mapFile = __MAP_FOLDER + mapFile;

    std :: filesystem :: current_path( argv[__ARG_DIRECTORY], errorCode );

    if( errorCode )  {
        fprintf( stderr, "Cannot enter the sample directory [%s]: %s\n", argv[__ARG_DIRECTORY], errorCode.message().c_str() );
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
    config.strTitle   = "Shapes: rectangle, ellipse, polyline, polygon";
    config.nTargetFps = fps;
    config.nMaxFrames = ( unsigned ) frames;
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

    if( !pRenderer -> LoadMap( mapFile.c_str(), ITM :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot load %s\n", mapFile.c_str() );
        return EXIT_FAILURE;
    }

    // The labels are in "Sans", the regular face of Caravellius 8x8 (a bitmap font).
    if( !pRenderer -> RegisterFont( "Sans", "/shared/fonts/caravellius8x8.fnt", false, false ) )
        fprintf( stderr, "Cannot load the Sans font\n" );

    pRenderer -> SetCameraPosition( __START_CAMERA_X, __START_CAMERA_Y );

    // The zoom argument is a factor, a multiple of 1/16 (ZOOM_STEP): factor = (position + 1) x 1/16. Page Up and
    // Page Down still change it from there.
    if( fZoom != 1.0 )
        pRenderer -> GetViewport().SetZoom( ( unsigned ) ( fZoom / SunLight :: Base :: ZOOM_STEP + 0.5 ) - 1u );

    printf( "zoom %.4f\n", pRenderer -> GetViewport().GetZoomProperties().fZoomFactor );

    // = zooms in and - zooms out, so the widths can be checked at several zoom levels.
    pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_EQUAL,
                                         [&pRenderer]( SunLight :: Input :: ControllerType, int ) { pRenderer -> ZoomIn(); } );
    pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_MINUS,
                                         [&pRenderer]( SunLight :: Input :: ControllerType, int ) { pRenderer -> ZoomOut(); } );

    printf( "Each numbered shape is drawn by the backend. Arrows scroll the map, = and - zoom, Esc quits.\n" );

    if( frames > 0 )  {
        auto  start = std :: chrono :: steady_clock :: now();

        pRenderer -> Run();

        double  fMillis = std :: chrono :: duration<double, std :: milli>( std :: chrono :: steady_clock :: now() - start ).count();

        printf( "bench map=%s frames=%d ms_per_frame=%.4f\n", mapFile.c_str(), frames, fMillis / frames );
    }
    else
        pRenderer -> Run();

    pRenderer -> Stop();

    return EXIT_SUCCESS;
}
