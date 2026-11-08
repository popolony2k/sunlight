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
 * The world container (I19b of the master plan): a World holds a map, the sprites registered on its
 * layers and its collision rules; a renderer's default world holds what the renderer itself used to.
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include "renderer/world/world.h"
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using SunLight :: Renderer :: MapData;
using SunLight :: Renderer :: World;

namespace  {

    // A 2 x 2 orthogonal map of 16 px tiles, one tileset image, one tile layer (id 1).
    const char  *g_szMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"16\" tileheight=\"16\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\">"
        "<image source=\"t.png\" width=\"16\" height=\"16\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,1,1,1</data></layer>"
        "</map>";

    Bytes ToBytes( const char* sz )  {
        std :: string  str( sz );
        return Bytes( str.begin(), str.end() );
    }
}

TEST_SUITE( "renderer/world" )  {

    TEST_CASE( "A new world is empty: no map, no sprites; unloading its map says there was none" )  {

        World  world( nullptr );

        CHECK( world.Map() == nullptr );
        CHECK( world.Sprites().Empty() == true );
        CHECK( world.UnloadMap() == false );
    }

    TEST_CASE( "A world holds the map it is given, frees it once on UnloadMap, and leaves its sprites and colliders as they are" )  {

        MemoryFileSystemFixture  fsFixture;
        MockEngineFixture        engineFixture;
        World                    world( nullptr );
        SunLight :: Sprite :: Sprite  sprite;

        fsFixture.fs.files["maps/level.tmx"] = ToBytes( g_szMap );

        std :: unique_ptr<MapData>  pMap = MapData :: Load( "maps/level.tmx" );

        REQUIRE( pMap != nullptr );

        MapData  *pRaw = pMap.get();

        world.SetMap( std :: move( pMap ) );
        CHECK( world.Map() == pRaw );
        CHECK( world.Map() -> FindLayer( 1 ) != nullptr );

        world.Sprites().OpenLayer( 1 );
        REQUIRE( world.Sprites().Add( 1, &sprite ) == true );
        REQUIRE( world.Collisions().AddCollider( 1, &sprite.GetCollider() ) == true );

        int  nUnloadsBefore = engineFixture.engine.nUnloadTextureCalls;

        CHECK( world.UnloadMap() == true );
        CHECK( world.Map() == nullptr );
        CHECK( engineFixture.engine.nUnloadTextureCalls == nUnloadsBefore + 1 );   // the tileset image, once
        CHECK( world.UnloadMap() == false );

        // The sprites and colliders are the renderer's to deal with (UnloadMap there): untouched here.
        CHECK( world.Sprites().Contains( 1, &sprite ) == true );
        CHECK( world.Collisions().RemoveCollider( 1, &sprite.GetCollider() ) == true );
        world.Sprites().Remove( 1, &sprite );
    }

    TEST_CASE( "Giving a world a second map frees the first one, exactly once" )  {

        MemoryFileSystemFixture  fsFixture;
        MockEngineFixture        engineFixture;
        World                    world( nullptr );

        fsFixture.fs.files["maps/level.tmx"] = ToBytes( g_szMap );
        world.SetMap( MapData :: Load( "maps/level.tmx" ) );

        int  nUnloadsBefore = engineFixture.engine.nUnloadTextureCalls;

        world.SetMap( MapData :: Load( "maps/level.tmx" ) );
        CHECK( engineFixture.engine.nUnloadTextureCalls == nUnloadsBefore + 1 );
        CHECK( world.Map() != nullptr );
    }

    TEST_CASE( "A renderer's default world outlives its maps: the same collision manager before, between and after them" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MemoryFileSystemFixture  fsFixture;

        fsFixture.fs.files["maps/level.tmx"] = ToBytes( g_szMap );

        SunLight :: Renderer :: RendererConfig  config;
        std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  pRenderer = SunLight :: Renderer :: TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        SunLight :: Collision :: ICollisionManager  *pBefore = &pRenderer -> GetCollisionManager();

        REQUIRE( pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( &pRenderer -> GetCollisionManager() == pBefore );

        REQUIRE( pRenderer -> UnloadMap() == true );
        CHECK( &pRenderer -> GetCollisionManager() == pBefore );

        // The world is still there for the next map.
        REQUIRE( pRenderer -> LoadMap( "maps/level.tmx", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        CHECK( &pRenderer -> GetCollisionManager() == pBefore );

        SunLight :: TileMap :: stMapInfo  info {};

        CHECK( pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.mapSize.nWidth == 2 );
        pRenderer -> Stop();
    }
}
