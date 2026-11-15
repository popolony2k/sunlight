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

#include "world.h"
#include "filesystem/filesystemfactory.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#define __NO_MAP_ARGUMENT  "nomap"


int main( int argc, char** argv )  {

    // Check command line arguments: the sample's directory, and "nomap" to draw with no map loaded.
    if( argc < 2 )  {
        fprintf( stderr, "Usage: %s <sample directory> [%s]\n", argv[0], __NO_MAP_ARGUMENT );
        return EXIT_FAILURE;
    }

    bool  bUseMap = !( ( argc > 2 ) && ( strcmp( argv[2], __NO_MAP_ARGUMENT ) == 0 ) );

    // Every resource is loaded by name relative to the sample's own directory, through
    // SunLight::FileSystem - which reads the working directory (mounted automatically) - so make
    // the sample's directory the working directory.
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

    World  world( bUseMap );

    return world.Run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
