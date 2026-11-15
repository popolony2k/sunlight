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

/*
 * I20 of the master plan: Tiled writes staggeraxis="x", libtmx reads only "y" or "columns" - so
 * TmxWorkarounds::FixStaggerAxis rewrites the value in the root <map> tag before libtmx parses the map.
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
#include "renderer/map/tmxworkarounds.h"
#include "renderer/map/mapdata.h"
#include "mock_engine.h"
#include "mock_filesystem.h"

using SunLight :: Renderer :: TmxWorkarounds;
using SunLight :: Renderer :: MapData;

namespace  {

    std :: vector<unsigned char> ToBytes( const std :: string& str )  {
        return std :: vector<unsigned char>( str.begin(), str.end() );
    }

    std :: string Text( const std :: vector<unsigned char>& data )  {
        return std :: string( data.begin(), data.end() );
    }

    // A map as Tiled writes it: orientation, staggeraxis="x", 2 x 2 tiles of 32 x 16.
    std :: string TiledMap( const char* szOrientation, const char* szExtra )  {
        return std :: string( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<map version=\"1.10\" tiledversion=\"1.11.0\" orientation=\"" ) + szOrientation +
               "\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"32\" tileheight=\"16\" infinite=\"0\"" + szExtra +
               " staggeraxis=\"x\" staggerindex=\"odd\" nextlayerid=\"2\" nextobjectid=\"1\">\n"
               " <tileset firstgid=\"1\" name=\"t\" tilewidth=\"32\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"t.png\" width=\"32\" height=\"16\"/></tileset>\n"
               " <layer id=\"1\" name=\"ground\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,1,1,1</data></layer>\n"
               "</map>\n";
    }
}

TEST_SUITE( "renderer/tmxworkarounds" )  {

    TEST_CASE( "FixStaggerAxis rewrites Tiled's \"x\" as libtmx's \"columns\", whatever the quotes and spaces" )  {

        std :: vector<unsigned char>  data = ToBytes( "<?xml version=\"1.0\"?><map orientation=\"staggered\" staggeraxis=\"x\" staggerindex=\"odd\"></map>" );

        CHECK( TmxWorkarounds :: FixStaggerAxis( data ) == true );
        CHECK( Text( data ) == "<?xml version=\"1.0\"?><map orientation=\"staggered\" staggeraxis=\"columns\" staggerindex=\"odd\"></map>" );

        data = ToBytes( "<map\n  orientation='hexagonal'\n  staggeraxis = 'x' >" );
        CHECK( TmxWorkarounds :: FixStaggerAxis( data ) == true );
        CHECK( Text( data ) == "<map\n  orientation='hexagonal'\n  staggeraxis = 'columns' >" );

        // A '>' inside an earlier value does not end the tag.
        data = ToBytes( "<map class=\"a>b\" staggeraxis=\"x\">" );
        CHECK( TmxWorkarounds :: FixStaggerAxis( data ) == true );
        CHECK( Text( data ) == "<map class=\"a>b\" staggeraxis=\"columns\">" );
    }

    TEST_CASE( "FixStaggerAxis leaves alone what libtmx reads already, and anything outside the root <map> tag" )  {

        for( const char* szMap : { "<map orientation=\"staggered\" staggeraxis=\"y\">",
                                   "<map orientation=\"staggered\" staggeraxis=\"columns\">",
                                   "<map orientation=\"orthogonal\" width=\"2\">",
                                   "<map orientation=\"staggered\"><properties><property name=\"staggeraxis\" value=\"x\"/>"
                                   "<property name=\"note\" value=\"staggeraxis=&quot;x&quot;\"/></properties></map>",
                                   "<map class=\"staggeraxis\" orientation=\"staggered\">",
                                   "<mapping staggeraxis=\"x\">",
                                   "no xml at all" } )  {
            std :: vector<unsigned char>  data = ToBytes( szMap );

            INFO( szMap );
            CHECK( TmxWorkarounds :: FixStaggerAxis( data ) == false );
            CHECK( Text( data ) == szMap );
        }
    }

    TEST_CASE( "A staggered and a hexagonal map saved by Tiled with stagger axis X load, and lay their cells out along X" )  {

        MemoryFileSystemFixture  fsFixture;
        MockEngineFixture        engineFixture;

        fsFixture.fs.files["maps/staggered.tmx"] = ToBytes( TiledMap( "staggered", "" ) );
        fsFixture.fs.files["maps/hexagonal.tmx"] = ToBytes( TiledMap( "hexagonal", " hexsidelength=\"0\"" ) );

        for( const char* szPath : { "maps/staggered.tmx", "maps/hexagonal.tmx" } )  {
            INFO( szPath );

            std :: unique_ptr<MapData>  pMap = MapData :: Load( szPath );

            REQUIRE( pMap != nullptr );                                  // libtmx refused it before I20
            CHECK( pMap -> Tmx() -> stagger_axis == SA_X );
            CHECK( pMap -> Tmx() -> stagger_index == SI_ODD );

            // Axis X, index odd: column 1 is half a column to the right and half a tile down.
            SunLight :: Base :: stCoordinate2D  corner = pMap -> MapProjection().TileDrawPosition(
                SunLight :: TileMap :: stMatrixPosition { 0, 1 }, pMap -> Tmx(), 32, 16, 0, 0 );

            CHECK( corner.x == 16 );
            CHECK( corner.y == 8 );
        }
    }
}
