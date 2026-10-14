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
 * T2 of the master plan: the map-loading pipeline itself, on real .tmx bytes parsed by libtmx -
 * what LoadMap reads, what it leaves loaded (map info, layers, objects, tileset textures), what a
 * failed load leaves behind, and what UnloadMap/Stop/a second LoadMap release. Orientation,
 * alignment, external tilesets and scroll steps have their own suites.
 *
 * Two bugs found writing it, both pinned here:
 * - A LoadMap over a loaded map never freed the old one (map, tile textures, Resource Manager),
 *   and a failed parse lost the old map without freeing it either.
 * - GetLayer/SetLayer converted opacity with two formulas that were not inverses, neither in
 *   libtmx's 0..1: an opaque layer read back as 256, and SetLayer(255) made a layer invisible.
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    // A 5x3 map of 16x8 tiles, its tileset embedded with an image beside the map. Layers: "ground"
    // (id 1, every cell gid 1), a group "grp" (id 2) holding the tile layer "inner" (id 3, opacity
    // 0.5) and the object group "nested" (id 4, object "exit"), then the object group "markers"
    // (id 5, object "spawn").
    const char  *g_szLevel =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"5\" height=\"3\" tilewidth=\"16\" tileheight=\"8\">"
        "<tileset firstgid=\"1\" name=\"ground\" tilewidth=\"16\" tileheight=\"8\" tilecount=\"4\" columns=\"4\">"
        "<image source=\"ground.png\" width=\"64\" height=\"8\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"5\" height=\"3\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
        "<group id=\"2\" name=\"grp\">"
        "<layer id=\"3\" name=\"inner\" width=\"5\" height=\"3\" opacity=\"0.5\"><data encoding=\"csv\">0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
        "<objectgroup id=\"4\" name=\"nested\"><object id=\"2\" name=\"exit\" x=\"64\" y=\"16\" width=\"8\" height=\"4\"/></objectgroup>"
        "</group>"
        "<objectgroup id=\"5\" name=\"markers\"><object id=\"1\" name=\"spawn\" x=\"24\" y=\"40\" width=\"16\" height=\"8\"/></objectgroup>"
        "</map>";

    // A second, differently sized map with its own embedded tileset image - and the same layer id 1.
    const char  *g_szOther =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"32\" tileheight=\"32\">"
        "<tileset firstgid=\"1\" name=\"rock\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
        "<image source=\"rock.png\" width=\"32\" height=\"32\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,1,1,1</data></layer>"
        "</map>";

    // Cut off mid-element: libtmx's parser refuses it.
    const char  *g_szTruncated =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"5\" height=\"3\" tilewidth=\"16\" tileheight=\"8\">"
        "<layer id=\"1\" name=\"ground\" width=\"5\"";

    Bytes ToBytes( const char *sz )  {
        std :: string  str( sz );
        return Bytes( str.begin(), str.end() );
    }

    void* const  TILES  = ( void* ) 0x3001;
    void* const  SPRITE = ( void* ) 0x3002;

    // A started renderer on the mocks, with the three maps above in an in-memory FileSystem.
    struct Scene  {

        MemoryFileSystemFixture             fsFixture;
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {

            RendererConfig  config;

            config.fWidth  = 320.0f;
            config.fHeight = 240.0f;

            fsFixture.fs.files["maps/level.tmx"]     = ToBytes( g_szLevel );
            fsFixture.fs.files["maps/other.tmx"]     = ToBytes( g_szOther );
            fsFixture.fs.files["maps/truncated.tmx"] = ToBytes( g_szTruncated );

            engine().hLoadTextureResult = TILES;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        bool Load( const char *szMap )  {
            return pRenderer -> LoadMap( szMap, ITM :: MAP_ALIGNMENT_TOP_LEFT );
        }

        bool MapInfo( SunLight :: TileMap :: stMapInfo &info )  {
            info = {};
            return pRenderer -> GetMapInfo( info );
        }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        int DrawsOf( void *handle )  {
            int  nCount = 0;

            for( const Event &evt : engine().events )
                if( ( evt.kind == Event :: TILE ) && ( evt.handle == handle ) )
                    nCount++;

            return nCount;
        }
    };

    // A one-texture sprite registered on layer 1, recognised by it's own texture handle.
    struct SpriteRig  {

        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;
        Scene                                &m_Scene;

        SpriteRig( Scene &scene ) : m_Scene( scene )  {

            scene.engine().hLoadTextureResult = SPRITE;
            scene.engine().nLoadTextureWidth  = 16;
            scene.engine().nLoadTextureHeight = 16;

            REQUIRE( canvas.Load( "sprite.png" ) == true );
            canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 20, 20 }, { 16, 16 } } );

            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );

            REQUIRE( scene.pRenderer -> AddSprite( 1, sprite ) == true );

            scene.engine().hLoadTextureResult = TILES;
        }

        ~SpriteRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( 1, sprite );
        }
    };
}

TEST_SUITE( "renderer/map loading" )  {

    TEST_CASE( "A map file that cannot be read is refused: no map, no texture loaded" )  {

        Scene                             scene;
        SunLight :: TileMap :: stMapInfo  info;

        CHECK( scene.Load( "maps/missing.tmx" ) == false );
        CHECK( scene.MapInfo( info ) == false );
        CHECK( scene.engine().nLoadTextureCalls == 0 );
    }

    TEST_CASE( "A truncated map is refused, leaves no map, and a good map loads afterwards" )  {

        Scene                             scene;
        SunLight :: TileMap :: stMapInfo  info;

        CHECK( scene.Load( "maps/truncated.tmx" ) == false );
        CHECK( scene.MapInfo( info ) == false );

        CHECK( scene.Load( "maps/level.tmx" ) == true );
        CHECK( scene.MapInfo( info ) == true );
    }

    TEST_CASE( "GetMapInfo reports exactly the size and tile size the file declares" )  {

        Scene                             scene;
        SunLight :: TileMap :: stMapInfo  info;

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );
        REQUIRE( scene.MapInfo( info ) == true );

        CHECK( info.mapSize.nWidth == 5 );
        CHECK( info.mapSize.nHeight == 3 );
        CHECK( info.tileSize.nWidth == 16 );
        CHECK( info.tileSize.nHeight == 8 );
        CHECK( info.pMap != nullptr );
    }

    TEST_CASE( "An embedded tileset's image is loaded once, relative to the map, and UnloadMap releases it once" )  {

        Scene  scene;

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );
        CHECK( scene.engine().nLoadTextureCalls == 1 );
        CHECK( scene.engine().strLastLoadTextureFileName == "maps/ground.png" );
        CHECK( scene.engine().nUnloadTextureCalls == 0 );

        CHECK( scene.pRenderer -> UnloadMap() == true );
        CHECK( scene.engine().nUnloadTextureCalls == 1 );
        CHECK( scene.pRenderer -> UnloadMap() == false );       // nothing left to release
        CHECK( scene.engine().nUnloadTextureCalls == 1 );
    }

    TEST_CASE( "Stop releases the tileset image of a map that is still loaded" )  {

        Scene  scene;

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );

        scene.pRenderer -> Stop();

        CHECK( scene.engine().nUnloadTextureCalls == 1 );
    }

    TEST_CASE( "Repeated load/unload cycles keep texture loads and unloads balanced" )  {

        Scene                    scene;
        static constexpr int     CYCLES = 5;

        for( int nCycle = 0; nCycle < CYCLES; nCycle++ )  {
            REQUIRE( scene.Load( "maps/level.tmx" ) == true );
            REQUIRE( scene.pRenderer -> UnloadMap() == true );
        }

        CHECK( scene.engine().nLoadTextureCalls == CYCLES );
        CHECK( scene.engine().nUnloadTextureCalls == CYCLES );
    }

    TEST_CASE( "GetLayer finds a layer by id and by name, also inside a group; an unknown one is refused" )  {

        Scene                           scene;
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );

        REQUIRE( scene.pRenderer -> GetLayer( 1, layer ) == true );
        REQUIRE( layer.pLayer != nullptr );
        CHECK( std :: string( layer.pLayer -> name ) == "ground" );

        REQUIRE( scene.pRenderer -> GetLayer( "inner", layer ) == true );
        CHECK( layer.pLayer -> id == 3 );

        REQUIRE( scene.pRenderer -> GetLayer( 3, layer ) == true );
        CHECK( std :: string( layer.pLayer -> name ) == "inner" );

        CHECK( scene.pRenderer -> GetLayer( 99, layer ) == false );
        CHECK( scene.pRenderer -> GetLayer( "nope", layer ) == false );
    }

    TEST_CASE( "GetLayer reports opacity in 0..255: an opaque layer is 255, a half one 128" )  {

        Scene                           scene;
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );

        REQUIRE( scene.pRenderer -> GetLayer( "ground", layer ) == true );
        CHECK( layer.nOpacity == 255 );
        CHECK( layer.bVisible == true );

        REQUIRE( scene.pRenderer -> GetLayer( "inner", layer ) == true );
        CHECK( layer.nOpacity == 128 );                         // 0.5 x 255 = 127.5, rounded
    }

    TEST_CASE( "SetLayer stores opacity as libtmx's 0..1, clamped, and GetLayer then SetLayer changes nothing else" )  {

        Scene                           scene;
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );
        REQUIRE( scene.pRenderer -> GetLayer( "ground", layer ) == true );

        tmx_layer  *pTmx = layer.pLayer;

        // The round trip Caravellius does: read the layer, change only its visibility, write it back.
        layer.bVisible = false;
        REQUIRE( scene.pRenderer -> SetLayer( "ground", layer ) == true );
        CHECK( pTmx -> opacity == 1.0 );
        CHECK( pTmx -> visible == 0 );

        REQUIRE( scene.pRenderer -> GetLayer( "ground", layer ) == true );
        CHECK( layer.nOpacity == 255 );
        CHECK( layer.bVisible == false );

        layer.nOpacity = 0;
        REQUIRE( scene.pRenderer -> SetLayer( 1, layer ) == true );
        CHECK( pTmx -> opacity == 0.0 );

        layer.nOpacity = 128;
        REQUIRE( scene.pRenderer -> SetLayer( 1, layer ) == true );
        CHECK( pTmx -> opacity == 128.0 / 255.0 );

        layer.nOpacity = 300;
        REQUIRE( scene.pRenderer -> SetLayer( 1, layer ) == true );
        CHECK( pTmx -> opacity == 1.0 );

        layer.nOpacity = -5;
        REQUIRE( scene.pRenderer -> SetLayer( 1, layer ) == true );
        CHECK( pTmx -> opacity == 0.0 );
    }

    TEST_CASE( "GetObjectByName finds an object in an object group and in one nested in a group" )  {

        Scene                            scene;
        SunLight :: TileMap :: stObject  object;

        CHECK( scene.pRenderer -> GetObjectByName( "spawn", object ) == false );   // no map yet

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );

        REQUIRE( scene.pRenderer -> GetObjectByName( "spawn", object ) == true );
        CHECK( object.strName == "spawn" );
        CHECK( object.dimension.pos.x == 24 );
        CHECK( object.dimension.pos.y == 40 );
        CHECK( object.dimension.size.nWidth == 16 );
        CHECK( object.dimension.size.nHeight == 8 );

        REQUIRE( scene.pRenderer -> GetObjectByName( "exit", object ) == true );
        CHECK( object.strName == "exit" );
        CHECK( object.dimension.pos.x == 64 );
        CHECK( object.dimension.pos.y == 16 );
        CHECK( object.dimension.size.nWidth == 8 );
        CHECK( object.dimension.size.nHeight == 4 );

        CHECK( scene.pRenderer -> GetObjectByName( "nope", object ) == false );

        REQUIRE( scene.pRenderer -> UnloadMap() == true );
        CHECK( scene.pRenderer -> GetObjectByName( "spawn", object ) == false );
    }

    TEST_CASE( "Loading a map over a loaded one releases the old map's tileset image" )  {

        Scene                             scene;
        SunLight :: TileMap :: stMapInfo  info;

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );
        REQUIRE( scene.Load( "maps/other.tmx" ) == true );

        CHECK( scene.engine().nLoadTextureCalls == 2 );
        CHECK( scene.engine().nUnloadTextureCalls == 1 );       // ground.png went with the old map
        CHECK( scene.engine().strLastLoadTextureFileName == "maps/rock.png" );

        REQUIRE( scene.MapInfo( info ) == true );
        CHECK( info.mapSize.nWidth == 2 );
        CHECK( info.tileSize.nWidth == 32 );

        scene.pRenderer -> Stop();
        CHECK( scene.engine().nUnloadTextureCalls == 2 );
    }

    TEST_CASE( "A map that fails to parse leaves the loaded map loaded, untouched" )  {

        Scene                             scene;
        SunLight :: TileMap :: stMapInfo  info;

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );

        CHECK( scene.Load( "maps/truncated.tmx" ) == false );
        CHECK( scene.engine().nUnloadTextureCalls == 0 );

        REQUIRE( scene.MapInfo( info ) == true );
        CHECK( info.mapSize.nWidth == 5 );
        CHECK( info.mapSize.nHeight == 3 );

        CHECK( scene.Load( "maps/missing.tmx" ) == false );
        CHECK( scene.MapInfo( info ) == true );
    }

    TEST_CASE( "A sprite registered on the old map stays registered across a LoadMap, it's texture untouched" )  {

        Scene      scene;

        REQUIRE( scene.Load( "maps/level.tmx" ) == true );

        SpriteRig  rig( scene );

        REQUIRE( scene.engine().nLoadTextureCalls == 2 );       // ground.png, sprite.png
        REQUIRE( scene.Load( "maps/other.tmx" ) == true );

        CHECK( scene.engine().nUnloadTextureCalls == 1 );       // ground.png only, not the sprite's

        scene.RunFrames( 1 );

        CHECK( scene.DrawsOf( SPRITE ) == 1 );                 // drawn on the new map's layer 1
        CHECK( scene.DrawsOf( TILES ) == 4 );                  // the new map's 2x2 tiles
    }
}
