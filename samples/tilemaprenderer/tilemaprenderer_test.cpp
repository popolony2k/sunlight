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
#include <string>
#include <cstdio>


int main( int argc, char **argv ) {
    std :: string   strBasePath;
    bool            bRet;

    // Check command line arguments
    if( argc < 2 )  {
        fprintf( stderr, "Invalid command line arguments\n" );
        return EXIT_FAILURE;
    }

    strBasePath = argv[1];

    World world( strBasePath );
    bRet = world.Run();

    if( !bRet ){
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
