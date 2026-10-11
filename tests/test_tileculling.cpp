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

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    const int  kTileSize = 16;   // every tile in these maps is 16 x 16, matching the map's own declared size

    // A map nTiles x nTiles, one tileset (gid 1, kTileSize square), every cell filled with gid 1.
    Bytes MakeFilledTmx( int nTiles )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"" << nTiles
            << "\" height=\"" << nTiles << "\" tilewidth=\"" << kTileSize << "\" tileheight=\"" << kTileSize << "\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"" << kTileSize << "\" tileheight=\"" << kTileSize
            << "\" tilecount=\"1\" columns=\"1\"><image source=\"tiles.png\" width=\"" << kTileSize << "\" height=\""
            << kTileSize << "\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"" << nTiles << "\" height=\"" << nTiles << "\"><data encoding=\"csv\">";

        for( int nCount = 0; nCount < nTiles * nTiles; nCount++ )
            tmx << ( nCount ? ",1" : "1" );

        tmx << "</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // A 5 x 5 map with two tilesets: gid 1 (16 x 16, matching the map's own declared size) everywhere except one
    // cell at (nSpecialRow, nSpecialCol), which uses gid 2 (nMismatchedTileSize square - mismatched on purpose).
    Bytes MakeMismatchedTmx( int nSpecialRow, int nSpecialCol, int nMismatchedTileSize )  {

        const int  nGridSize = 5;
        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"" << nGridSize
            << "\" height=\"" << nGridSize << "\" tilewidth=\"" << kTileSize << "\" tileheight=\"" << kTileSize << "\">"
            << "<tileset firstgid=\"1\" name=\"small\" tilewidth=\"" << kTileSize << "\" tileheight=\"" << kTileSize
            << "\" tilecount=\"1\" columns=\"1\"><image source=\"small.png\" width=\"" << kTileSize << "\" height=\""
            << kTileSize << "\"/></tileset>"
            << "<tileset firstgid=\"2\" name=\"big\" tilewidth=\"" << nMismatchedTileSize << "\" tileheight=\""
            << nMismatchedTileSize << "\" tilecount=\"1\" columns=\"1\"><image source=\"big.png\" width=\""
            << nMismatchedTileSize << "\" height=\"" << nMismatchedTileSize << "\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"" << nGridSize << "\" height=\"" << nGridSize << "\"><data encoding=\"csv\">";

        for( int nRow = 0; nRow < nGridSize; nRow++ )
            for( int nCol = 0; nCol < nGridSize; nCol++ )  {
                bool  bFirst = ( nRow == 0 ) && ( nCol == 0 );
                int   nGid   = ( ( nRow == nSpecialRow ) && ( nCol == nSpecialCol ) ) ? 2 : 1;

                tmx << ( bFirst ? "" : "," ) << nGid;
            }

        tmx << "</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }


    // A map declaring tile_width/tile_height nDeclared, 1 row by nTiles columns, every cell using ONE tileset
    // whose own tile size is nActual (may differ from the map's declared size).
    Bytes MakeOneRowTmx( int nTiles, int nDeclared, int nActual )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"" << nTiles
            << "\" height=\"1\" tilewidth=\"" << nDeclared << "\" tileheight=\"" << nDeclared << "\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"" << nActual << "\" tileheight=\"" << nActual
            << "\" tilecount=\"1\" columns=\"1\"><image source=\"tiles.png\" width=\"" << nActual << "\" height=\""
            << nActual << "\"/></tileset>"
            << "<layer id=\"1\" name=\"ground\" width=\"" << nTiles << "\" height=\"1\"><data encoding=\"csv\">";

        for( int nCount = 0; nCount < nTiles; nCount++ )
            tmx << ( nCount ? ",1" : "1" );

        tmx << "</data></layer></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // Renders one frame of the given map, with a viewport at (0, 0) of the given size, zoom 1, camera at the origin.
    struct Frame  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Frame( const Bytes &tmx, int nViewportWidth, int nViewportHeight )  {

            fsFixture.fs.files["maps/culling.tmx"] = tmx;

            RendererConfig  config;

            config.fWidth  = 1260.0f;
            config.fHeight = 920.0f;

            SunLight :: Base :: stDimension2D  viewport {};

            viewport.pos.x = 0;  viewport.pos.y = 0;  viewport.size.nWidth = nViewportWidth;  viewport.size.nHeight = nViewportHeight;
            config.viewport = viewport;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            windowFixture.window.nFramesUntilShouldClose = 1;
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/culling.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
            pRenderer -> Run();
        }

        ~Frame( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        int CountTiles( void ) const  {
            int  nCount = 0;

            for( const Event &evt : engineFixture.engine.events )
                if( evt.kind == Event :: TILE )
                    nCount++;

            return nCount;
        }
    };
}

TEST_SUITE( "Tile culling" )  {

    TEST_CASE( "A map far larger than the viewport draws a bounded number of tiles, not every cell" )  {

        // A 100 x 100 map (10000 cells) of 16 x 16 tiles, with a 160 x 128 viewport at zoom 1: padded by one tile
        // either side, the visible range is columns [0, 11) and rows [0, 9) - 11 x 9 = 99 tiles, not 10000.
        Frame  frame( MakeFilledTmx( 100 ), 160, 128 );

        CHECK( frame.CountTiles() == 99 );
    }

    TEST_CASE( "A map whose tilesets disagree on tile size falls back to visiting every cell" )  {

        // A 5 x 5 map (25 cells): every cell has a valid gid, and the far corner (4, 4) uses a tileset whose own
        // tile size (48, not the map's declared 16) also moves where DrawLayer draws it - at column 4, row 4 of
        // a 48 px step, that is pixel (192, 192), a pre-existing effect of DrawLayer using the resolved tile's
        // own tileset size, unrelated to E0. The viewport is large enough to show every cell at its own actual
        // position, so a count of 25 proves the fallback visited and drew all of them, mismatched size and all.
        Frame  frame( MakeMismatchedTmx( 4, 4, 48 ), 250, 250 );

        CHECK( frame.CountTiles() == 25 );
    }

    TEST_CASE( "A map whose tilesets agree on tile size is still exactly 25 tiles when every cell is visible" )  {

        // The same 5 x 5, 25-cell layout, but with the far tileset's own size matching the map's (16): culling is
        // safe here, and since the viewport covers the whole small map, it still finds and draws every cell.
        Frame  frame( MakeMismatchedTmx( 4, 4, 16 ), 200, 200 );

        CHECK( frame.CountTiles() == 25 );
    }

    TEST_CASE( "A mismatched tileset smaller than the map's declared size is not wrongly culled near the far edge" )  {

        // Map declares 48 x 48 tiles; the one tileset used is actually 16 x 16 - every cell is drawn at its own
        // (smaller) step, column K at pixel 16K, not the map's assumed 48K. A naive range using the map's 48 px
        // step for a 60 px viewport would stop at column 3 (3 x 48 = 144, past the view) - excluding column 3,
        // whose true position (48) is still inside the 60 px viewport. The fallback must still draw it.
        Frame  frame( MakeOneRowTmx( 10, 48, 16 ), 60, 60 );

        CHECK( frame.CountTiles() == 4 );
    }
}
