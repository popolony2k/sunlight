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
 * S1c2 of the master plan: LayerRenderer on its own - tile layers, image layers and tile objects
 * drawn through a DrawContext from a MapData, with no TileMapRenderer, only an in-memory FileSystem,
 * the mock engine and the mock clock. Every expected position is worked out by hand from
 * (map position + camera) x zoom + viewport origin.
 */

#include <doctest/doctest.h>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "renderer/map/layerrenderer.h"
#include "renderer/map/mapdata.h"
#include "mock_engine.h"
#include "mock_clock.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef MockEngine :: Event  Event;

namespace  {

    // An 8 x 6 orthogonal map of 16 x 8 tiles. Tileset "t" (firstgid 1, three tiles side by side in a
    // 48 x 8 image): tile 2 (gid 3) is animated - tile 2 for 100 ms, then tile 1 for 100 ms.
    // Layers:
    // - "ground" (id 1): every cell gid 1, except row 0's gid 2 at column 1, nothing at column 2, gid 3 at column 3;
    // - "tinted" (id 2): opacity 0.5, tint #ff8040, offset (2, 4); only cell (row 1, column 1), gid 2;
    // - "sky" (id 3): an image layer, a 40 x 20 image at offset (3, 4), tint #204060;
    // - "things" (id 4): tint #80ff80; tile objects of gid 2 - "own" at its tile's size, "stretched"
    //   to twice that, "turned" by 90 degrees.
    const char  *g_szMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"8\" height=\"6\" tilewidth=\"16\" tileheight=\"8\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"8\" tilecount=\"3\" columns=\"3\">"
        "<image source=\"t.png\" width=\"48\" height=\"8\"/>"
        "<tile id=\"2\"><animation><frame tileid=\"2\" duration=\"100\"/><frame tileid=\"1\" duration=\"100\"/></animation></tile>"
        "</tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"8\" height=\"6\"><data encoding=\"csv\">"
        "1,2,0,3,1,1,1,1,"
        "1,1,1,1,1,1,1,1,"
        "1,1,1,1,1,1,1,1,"
        "1,1,1,1,1,1,1,1,"
        "1,1,1,1,1,1,1,1,"
        "1,1,1,1,1,1,1,1"
        "</data></layer>"
        "<layer id=\"2\" name=\"tinted\" width=\"8\" height=\"6\" opacity=\"0.5\" tintcolor=\"#ff8040\" offsetx=\"2\" offsety=\"4\"><data encoding=\"csv\">"
        "0,0,0,0,0,0,0,0,"
        "0,2,0,0,0,0,0,0,"
        "0,0,0,0,0,0,0,0,"
        "0,0,0,0,0,0,0,0,"
        "0,0,0,0,0,0,0,0,"
        "0,0,0,0,0,0,0,0"
        "</data></layer>"
        "<imagelayer id=\"3\" name=\"sky\" offsetx=\"3\" offsety=\"4\" tintcolor=\"#204060\"><image source=\"sky.png\" width=\"40\" height=\"20\"/></imagelayer>"
        "<objectgroup id=\"4\" name=\"things\" tintcolor=\"#80ff80\">"
        "<object id=\"1\" name=\"own\" gid=\"2\" x=\"10\" y=\"30\" width=\"16\" height=\"8\"/>"
        "<object id=\"2\" name=\"stretched\" gid=\"2\" x=\"50\" y=\"40\" width=\"32\" height=\"16\"/>"
        "<object id=\"3\" name=\"turned\" gid=\"2\" x=\"70\" y=\"60\" width=\"16\" height=\"8\" rotation=\"90\"/>"
        "</objectgroup>"
        "</map>";

    const int  TILE_W         = 16;
    const int  TILE_H         = 8;
    const int  FILLED_GROUND  = ( 8 * 6 ) - 1;     // every "ground" cell but the empty one

    Bytes ToBytes( const char *sz )  {
        std :: string  str( sz );
        return Bytes( str.begin(), str.end() );
    }

    // Zoom 2, a camera of (5, -3), the viewport at (100, 200): map (x, y) lands at
    // ((x + 5) x 2 + 100, (y - 3) x 2 + 200).
    DrawContext ScaledContext( void )  {
        return DrawContext { { { 100, 200 }, { 640, 480 } }, 2.0f, { 5.0f, -3.0f } };
    }

    float ScreenX( float fMapX )  { return ( ( fMapX + 5.0f ) * 2.0f ) + 100.0f; }
    float ScreenY( float fMapY )  { return ( ( fMapY - 3.0f ) * 2.0f ) + 200.0f; }

    // The map, loaded through the in-memory files and the mock engine, and the clock its animation reads.
    struct LoadedMap  {
        MemoryFileSystemFixture      fsFixture;
        MockEngineFixture            engineFixture;
        MockClockFixture             clockFixture;
        std :: unique_ptr<MapData>   pMap;
        LayerRenderer                renderer;

        LoadedMap( void )  {
            fsFixture.fs.files["maps/level.tmx"] = ToBytes( g_szMap );
            pMap = MapData :: Load( "maps/level.tmx" );
            REQUIRE( pMap != nullptr );
            engine().events.clear();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        tmx_layer* Layer( const char *szName )  {
            tmx_layer  *pLayer = pMap -> FindLayer( szName );
            REQUIRE( pLayer != nullptr );
            return pLayer;
        }

        tmx_object* Object( const char *szName )  {
            for( tmx_object *pObject = Layer( "things" ) -> content.objgr -> head; pObject != nullptr; pObject = pObject -> next )
                if( ( pObject -> name != nullptr ) && ( std :: strcmp( pObject -> name, szName ) == 0 ) )
                    return pObject;

            FAIL( "no object named " << szName );
            return nullptr;
        }

        std :: vector<Event> EventsOf( Event :: Kind kind ) const  {
            std :: vector<Event>  found;

            for( const Event &evt : engineFixture.engine.events )
                if( evt.kind == kind )
                    found.push_back( evt );

            return found;
        }

        // The TILE event drawn at a screen position, or a failure.
        Event TileAt( float fX, float fY ) const  {
            for( const Event &evt : EventsOf( Event :: TILE ) )
                if( ( evt.x == fX ) && ( evt.y == fY ) )
                    return evt;

            FAIL( "no tile drawn at " << fX << ", " << fY );
            return Event {};
        }
    };

    void CheckColor( const SunLight :: Base :: stColor &color, int nRed, int nGreen, int nBlue, int nAlpha )  {
        CHECK( color.nRed == nRed );
        CHECK( color.nGreen == nGreen );
        CHECK( color.nBlue == nBlue );
        CHECK( color.nAlpha == nAlpha );
    }
}

TEST_SUITE( "renderer/LayerRenderer" )  {

    TEST_CASE( "A tile layer: each filled cell once, at its zoomed screen position, whole, from its tile's source" )  {

        LoadedMap  map;

        map.renderer.DrawTileLayer( ScaledContext(), *map.pMap, map.Layer( "ground" ) );

        REQUIRE( map.EventsOf( Event :: TILE ).size() == FILLED_GROUND );

        // Row 0, column 1 (gid 2): its top edge is above the viewport (map y 0 - 3 < 0), but the tile is
        // drawn whole, at its unclipped position - the view's clip cuts it, not LayerRenderer.
        Event  tile = map.TileAt( ScreenX( 1 * TILE_W ), ScreenY( 0 ) );

        CHECK( tile.w == TILE_W * 2 );
        CHECK( tile.h == TILE_H * 2 );
        CHECK( tile.scale == 2.0f );
        CHECK( tile.srcX == 1 * TILE_W );
        CHECK( tile.handle == map.engine().hLoadTextureResult );
        CheckColor( tile.color, 0xFF, 0xFF, 0xFF, 0xFF );   // no tint, opacity 1

        CHECK( map.TileAt( ScreenX( 7 * TILE_W ), ScreenY( 5 * TILE_H ) ).srcX == 0 );   // the last cell, gid 1

        // Column 2 of row 0 is empty: nothing is drawn there.
        for( const Event &evt : map.EventsOf( Event :: TILE ) )
            CHECK_FALSE( ( ( evt.x == ScreenX( 2 * TILE_W ) ) && ( evt.y == ScreenY( 0 ) ) ) );
    }

    TEST_CASE( "A tile layer's opacity, tint and offset apply to each of its tiles" )  {

        LoadedMap  map;

        map.renderer.DrawTileLayer( ScaledContext(), *map.pMap, map.Layer( "tinted" ) );

        REQUIRE( map.EventsOf( Event :: TILE ).size() == 1 );

        // Row 1, column 1, moved by the layer's (2, 4); alpha is 0.5 of 255, truncated.
        Event  tile = map.TileAt( ScreenX( ( 1 * TILE_W ) + 2 ), ScreenY( ( 1 * TILE_H ) + 4 ) );

        CHECK( tile.srcX == 1 * TILE_W );
        CheckColor( tile.color, 0xFF, 0x80, 0x40, 127 );
    }

    TEST_CASE( "An animated cell is drawn at its current frame" )  {

        LoadedMap  map;
        float      fX = ScreenX( 3 * TILE_W );
        float      fY = ScreenY( 0 );

        map.renderer.DrawTileLayer( ScaledContext(), *map.pMap, map.Layer( "ground" ) );
        CHECK( map.TileAt( fX, fY ).srcX == 2 * TILE_W );      // frame 0: tile 2

        map.engine().events.clear();
        map.clockFixture.clock.Advance( 100 );
        map.renderer.DrawTileLayer( ScaledContext(), *map.pMap, map.Layer( "ground" ) );
        CHECK( map.TileAt( fX, fY ).srcX == 1 * TILE_W );      // frame 1: tile 1
    }

    TEST_CASE( "A tile layer draws only the tiles the viewport can show" )  {

        // A 32 x 16 viewport at zoom 2 shows 16 x 8 map pixels: one cell. A tile entirely off the viewport is
        // skipped (Viewport::ClipRect), so only the cells beside it can be drawn. Culling (which cells are
        // visited at all) only saves work and is not visible here; test_tileculling pins that it loses
        // no visible cell.
        LoadedMap    map;
        DrawContext  small { { { 0, 0 }, { 32, 16 } }, 2.0f, { 0.0f, 0.0f } };

        map.renderer.DrawTileLayer( small, *map.pMap, map.Layer( "ground" ) );

        std :: vector<Event>  tiles = map.EventsOf( Event :: TILE );

        CHECK( tiles.size() > 0 );
        CHECK( tiles.size() < FILLED_GROUND );

        for( const Event &evt : tiles )  {
            CHECK( evt.x < 3 * TILE_W * 2 );
            CHECK( evt.y < 3 * TILE_H * 2 );
        }
    }

    TEST_CASE( "An image layer: its whole image at its zoomed position, with its tint, opaque" )  {

        LoadedMap  map;

        map.renderer.DrawImageLayer( ScaledContext(), map.Layer( "sky" ) );

        REQUIRE( map.EventsOf( Event :: TILE ).size() == 1 );

        Event  image = map.EventsOf( Event :: TILE )[0];

        CHECK( image.x == ScreenX( 3 ) );
        CHECK( image.y == ScreenY( 4 ) );
        CHECK( image.w == 40 * 2 );
        CHECK( image.h == 20 * 2 );
        CHECK( image.srcX == 0 );
        CHECK( image.scale == 2.0f );
        CheckColor( image.color, 0x20, 0x40, 0x60, 0xFF );
        CHECK( map.engine().lastDrawTextureTiledSource.width == 40 );
        CHECK( map.engine().lastDrawTextureTiledSource.height == 20 );
    }

    TEST_CASE( "An image layer entirely outside the viewport is not drawn" )  {

        LoadedMap    map;
        DrawContext  away { { { 100, 200 }, { 640, 480 } }, 2.0f, { 5000.0f, 0.0f } };

        map.renderer.DrawImageLayer( away, map.Layer( "sky" ) );

        CHECK( map.EventsOf( Event :: TILE ).empty() );
    }

    TEST_CASE( "A tile object at its tile's size is drawn as a map tile, from its bottom-left corner" )  {

        LoadedMap  map;

        map.renderer.DrawTileObject( ScaledContext(), *map.pMap, map.Object( "own" ), map.Layer( "things" ), 0.5f );

        REQUIRE( map.EventsOf( Event :: TILE ).size() == 1 );

        // (10, 30) is the bottom-left corner: the tile's top-left is (10, 30 - 8).
        Event  tile = map.TileAt( ScreenX( 10 ), ScreenY( 30 - TILE_H ) );

        CHECK( tile.w == TILE_W * 2 );
        CHECK( tile.h == TILE_H * 2 );
        CHECK( tile.srcX == 1 * TILE_W );
        CheckColor( tile.color, 0x80, 0xFF, 0x80, 127 );
    }

    TEST_CASE( "A resized tile object is stretched into its box, and cut at the viewport's edge" )  {

        LoadedMap  map;

        // Box: (50, 40 - 16) to (82, 40), so screen (210, 242) to (274, 274).
        map.renderer.DrawTileObject( ScaledContext(), *map.pMap, map.Object( "stretched" ), map.Layer( "things" ), 1.0f );

        REQUIRE( map.EventsOf( Event :: SCALED ).size() == 1 );

        Event  whole = map.EventsOf( Event :: SCALED )[0];

        CHECK( whole.x == ScreenX( 50 ) );
        CHECK( whole.y == ScreenY( 40 - 16 ) );
        CHECK( whole.w == 64 );
        CHECK( whole.h == 32 );
        CHECK( whole.srcX == 1 * TILE_W );
        CHECK( whole.srcY == 0 );
        CHECK( whole.srcW == TILE_W );
        CHECK( whole.srcH == TILE_H );
        CHECK( whole.color.nAlpha == 0xFF );

        // A viewport ending at x = 240 keeps the box's left 30 of 64 pixels, and so 30 / 64 of the tile.
        DrawContext  narrow { { { 100, 200 }, { 140, 480 } }, 2.0f, { 5.0f, -3.0f } };

        map.engine().events.clear();
        map.renderer.DrawTileObject( narrow, *map.pMap, map.Object( "stretched" ), map.Layer( "things" ), 1.0f );

        REQUIRE( map.EventsOf( Event :: SCALED ).size() == 1 );

        Event  cut = map.EventsOf( Event :: SCALED )[0];

        CHECK( cut.x == ScreenX( 50 ) );
        CHECK( cut.w == 30 );
        CHECK( cut.h == 32 );
        CHECK( cut.srcX == 1 * TILE_W );
        CHECK( cut.srcW == 30.0f / 64.0f * TILE_W );
        CHECK( cut.srcH == TILE_H );

        // A viewport that ends before the box draws nothing.
        DrawContext  before { { { 100, 200 }, { 100, 480 } }, 2.0f, { 5.0f, -3.0f } };

        map.engine().events.clear();
        map.renderer.DrawTileObject( before, *map.pMap, map.Object( "stretched" ), map.Layer( "things" ), 1.0f );

        CHECK( map.EventsOf( Event :: SCALED ).empty() );
    }

    TEST_CASE( "A turned tile object is turned about its bottom-left corner, with its layer's tint" )  {

        LoadedMap  map;

        map.renderer.DrawTileObject( ScaledContext(), *map.pMap, map.Object( "turned" ), map.Layer( "things" ), 1.0f );

        REQUIRE( map.EventsOf( Event :: TILE_ROTATED ).size() == 1 );

        Event  turned = map.EventsOf( Event :: TILE_ROTATED )[0];

        CHECK( turned.x == ScreenX( 70 ) );       // the anchor: the bottom-left corner
        CHECK( turned.y == ScreenY( 60 ) );
        CHECK( turned.w == TILE_W * 2 );
        CHECK( turned.h == TILE_H * 2 );
        CHECK( turned.scale == 90.0f );           // the rotation
        CHECK( turned.srcX == 0 );                // the origin: one box height above the anchor
        CHECK( turned.srcY == TILE_H * 2 );
        CHECK( turned.srcW == TILE_W );
        CHECK( turned.srcH == TILE_H );
        CheckColor( turned.color, 0x80, 0xFF, 0x80, 0xFF );
    }

    TEST_CASE( "A tile object with no gid, or a gid past the tileset, draws nothing" )  {

        LoadedMap     map;
        tmx_object    object = *map.Object( "own" );

        object.content.gid = 0;
        map.renderer.DrawTileObject( ScaledContext(), *map.pMap, &object, map.Layer( "things" ), 1.0f );

        object.content.gid = 99;
        map.renderer.DrawTileObject( ScaledContext(), *map.pMap, &object, map.Layer( "things" ), 1.0f );

        CHECK( map.engine().events.empty() );
    }
}
