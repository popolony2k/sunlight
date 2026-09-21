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

#ifndef __MOCK_FILESYSTEM_H__
#define __MOCK_FILESYSTEM_H__

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "filesystem/filesystemfactory.h"

typedef std :: vector<unsigned char>  Bytes;

/**
 * @brief In-memory IFileSystem: a map of virtual path -> bytes.
 */
class MemoryFileSystem : public SunLight :: FileSystem :: IFileSystem  {

    public:

    std :: map<std :: string, Bytes>  files;

    void SetReadFilter( ReadFilterCallback ) override {}
    bool Init( const char * ) override { return true; }
    void Shutdown( void ) override {}
    bool Mount( const std :: string &, const std :: string &, bool ) override { return true; }
    bool Exists( const std :: string &strPath ) override { return files.count( strPath ) > 0; }

    bool ReadFile( const std :: string &strPath, Bytes &out ) override {
        auto  it = files.find( strPath );

        if( it == files.end() )
            return false;

        out = it -> second;
        return true;
    }
};

/**
 * @brief Installs a MemoryFileSystem as the process filesystem for its lifetime.
 */
class MemoryFileSystemFixture  {

    public:

    MemoryFileSystem  fs;

    MemoryFileSystemFixture( void )  { SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( &fs ); }
    ~MemoryFileSystemFixture( void ) { SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( nullptr ); }
};

/**
 * @brief A tiny orthogonal TMX map as bytes: nTiles x nTiles tiles of nTileSize
 * pixels, all empty, in one tile layer ("ground") - plus, optionally, one
 * object layer holding one rectangle object.
 */
inline Bytes MakeSquareTmx( int nTiles, int nTileSize, bool bWithRectangleObject = false,
                            int nObjX = 0, int nObjY = 0, int nObjW = 0, int nObjH = 0 )  {

    std :: ostringstream  tmx;

    tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"" << nTiles
        << "\" height=\"" << nTiles << "\" tilewidth=\"" << nTileSize << "\" tileheight=\"" << nTileSize << "\">"
        << "<layer id=\"1\" name=\"ground\" width=\"" << nTiles << "\" height=\"" << nTiles << "\"><data encoding=\"csv\">";

    for( int nCount = 0; nCount < nTiles * nTiles; nCount++ )
        tmx << ( nCount ? ",0" : "0" );

    tmx << "</data></layer>";

    if( bWithRectangleObject )
        tmx << "<objectgroup id=\"2\" name=\"shapes\"><object id=\"1\" x=\"" << nObjX << "\" y=\"" << nObjY
            << "\" width=\"" << nObjW << "\" height=\"" << nObjH << "\"/></objectgroup>";

    tmx << "</map>";

    std :: string  str = tmx.str();

    return Bytes( str.begin(), str.end() );
}

#endif /* __MOCK_FILESYSTEM_H__ */
