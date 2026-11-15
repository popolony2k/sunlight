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
 * Several worlds, shown by views (I19c of the master plan): world handles, a view showing a world,
 * each pass drawing its view's world, every world updating, per-world collisions, detaching sprites.
 * Run against MockEngine: world A's tiles are 16 px, world B's 32 px, so a draw tells which world it is.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "collision/icollisionlistener.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"
#include "mock_clock.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap                 ITM;
typedef std :: shared_ptr<SunLight :: TileMap :: IWorld>  WorldHandle;
typedef std :: shared_ptr<SunLight :: TileMap :: IView>   ViewHandle;
typedef MockEngine :: Event                             Event;

namespace  {

    const float  __TILE_A = 16.0f;        // world A's tile size (the default world's map)
    const float  __TILE_B = 32.0f;        // world B's

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: Base :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    Bytes ToBytes( const std :: string& str )  {
        return Bytes( str.begin(), str.end() );
    }

    // Map A: 4 x 4 of 16 px, layers "ground" (id 1) and "props" (id 2).
    std :: string MapA( void )  {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
               "<tileset firstgid=\"1\" name=\"a\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"a.png\" width=\"16\" height=\"16\"/></tileset>"
               "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
               "<layer id=\"2\" name=\"props\" width=\"4\" height=\"4\"><data encoding=\"csv\">0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
               "</map>";
    }

    // Map B: 2 x 2 of 32 px, layers "floor" (id 1) and "only_in_b" (id 7) - a layer id map A has not.
    std :: string MapB( void )  {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"32\" tileheight=\"32\" nextlayerid=\"8\">"
               "<tileset firstgid=\"1\" name=\"b\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\"><image source=\"b.png\" width=\"32\" height=\"32\"/></tileset>"
               "<layer id=\"1\" name=\"floor\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,1,1,1</data></layer>"
               "<layer id=\"7\" name=\"only_in_b\" width=\"2\" height=\"2\"><data encoding=\"csv\">0,0,0,0</data></layer>"
               "</map>";
    }

    const SunLight :: Base :: stDimension2D  g_DefaultRect = Rect( 10, 10, 100, 100 );
    const SunLight :: Base :: stDimension2D  g_SideRect    = Rect( 300, 10, 100, 100 );

    // Mocks, a started renderer with map A in its default world (default view at g_DefaultRect).
    struct Scene  {

        MockClockFixture                    clockFixture;
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/a.tmx"] = ToBytes( MapA() );
            fsFixture.fs.files["maps/b.tmx"] = ToBytes( MapB() );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = g_DefaultRect;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/a.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // Tile draws of a given size whose top-left lands inside the rectangle.
        int TilesIn( const SunLight :: Base :: stDimension2D& rect, float fSize )  {
            int  nCount = 0;

            for( const Event& evt : engine().events )  {
                if( ( evt.kind == Event :: TILE ) && ( evt.w == fSize ) &&
                    ( evt.x >= rect.pos.x ) && ( evt.x < rect.pos.x + rect.size.nWidth ) &&
                    ( evt.y >= rect.pos.y ) && ( evt.y < rect.pos.y + rect.size.nHeight ) )
                    nCount++;
            }

            return nCount;
        }
    };

    // A sprite with two canvases of one 16 px frame each, 10 ms apart: with the clock moving 20 ms per
    // frame, every Advance() switches to the other canvas - so GetActiveTexture tells how many times it
    // advanced (odd or even), drawn or not.
    struct TwoFrameSprite  {
        SunLight :: Canvas :: TextureCanvas  first, second;
        SunLight :: Sprite :: Sprite         sprite;

        TwoFrameSprite( MockEngine& engine )  {
            engine.nLoadTextureWidth  = 16;
            engine.nLoadTextureHeight = 16;

            REQUIRE( first.Load( "s1.png" ) == true );
            REQUIRE( second.Load( "s2.png" ) == true );

            sprite.SetWorldSpace( true );
            sprite.AddTextureSequence( 0, &first, 10 );
            sprite.AddTextureSequence( 0, &second, 10 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );
        }

        bool OnSecond( void )  { return sprite.GetActiveTexture() == &second; }
    };

    struct CountingListener : public SunLight :: Collision :: ICollisionListener  {
        int  nColliderHits = 0;

        void OnCollision( SunLight :: Collision :: Collider*, SunLight :: Collision :: Collider* ) override  { nColliderHits++; }
        void OnCollision( SunLight :: Collision :: Collider*, SunLight :: TileMap :: stTile* ) override  {}
    };
}

TEST_SUITE( "renderer/worlds" )  {

    TEST_CASE( "World handles: the default world is id 0 and permanent; CreateWorld gives ids from 1; RemoveWorld leaves an inert handle" )  {

        Scene        scene;
        WorldHandle  pDefault = scene.pRenderer -> GetDefaultWorld();

        REQUIRE( pDefault != nullptr );
        CHECK( pDefault -> GetId() == 0 );
        CHECK( pDefault == scene.pRenderer -> GetDefaultWorld() );
        CHECK( scene.pRenderer -> GetWorldCount() == 1 );
        CHECK( scene.pRenderer -> RemoveWorld( pDefault ) == false );

        WorldHandle  pB = scene.pRenderer -> CreateWorld();
        WorldHandle  pC = scene.pRenderer -> CreateWorld();

        CHECK( pB -> GetId() == 1 );
        CHECK( pC -> GetId() == 2 );
        CHECK( scene.pRenderer -> GetWorldCount() == 3 );

        CHECK( scene.pRenderer -> RemoveWorld( pB ) == true );
        CHECK( pB -> IsRemoved() == true );
        CHECK( scene.pRenderer -> RemoveWorld( pB ) == false );            // already removed
        CHECK( scene.pRenderer -> RemoveWorld( nullptr ) == false );
        CHECK( scene.pRenderer -> GetWorldCount() == 2 );

        // Inert: nothing acts through the renderer any more, but every call is safe.
        SunLight :: Sprite :: Sprite      sprite;
        SunLight :: TileMap :: stLayer    layer {};
        SunLight :: TileMap :: stMapInfo  info {};

        CHECK( pB -> LoadMap( "maps/b.tmx" ) == false );
        CHECK( pB -> UnloadMap() == false );
        CHECK( pB -> AddSprite( 1, sprite ) == false );
        CHECK( pB -> GetLayer( 1, layer ) == false );
        CHECK( pB -> GetMapInfo( info ) == false );
        CHECK( scene.pRenderer -> CreateWorld() -> GetId() == 3 );         // ids are never reused
    }

    TEST_CASE( "Another renderer's world is refused, by RemoveWorld and by SetWorld" )  {

        Scene                               scene;
        RendererConfig                      config;
        std :: unique_ptr<TileMapRenderer>  pOther = TileMapRenderer :: Create( config, nullptr );    // never started

        REQUIRE( pOther != nullptr );

        WorldHandle  pForeign = pOther -> CreateWorld();
        ViewHandle   pView    = scene.pRenderer -> CreateView( g_SideRect );

        CHECK( scene.pRenderer -> RemoveWorld( pForeign ) == false );
        CHECK( pView -> SetWorld( pForeign ) == false );
        CHECK( pView -> SetWorld( pOther -> GetDefaultWorld() ) == false );
        CHECK( pView -> GetWorld() == scene.pRenderer -> GetDefaultWorld() );
    }

    TEST_CASE( "A world answers for its own map; the renderer's single-map calls stay on the default world" )  {

        Scene        scene;
        WorldHandle  pB = scene.pRenderer -> CreateWorld();

        SunLight :: TileMap :: stMapInfo  info {};

        CHECK( pB -> GetMapInfo( info ) == false );                         // empty
        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );

        REQUIRE( pB -> GetMapInfo( info ) == true );
        CHECK( info.tileSize.nWidth == 32 );
        CHECK( info.mapSize.nWidth == 2 );

        SunLight :: TileMap :: stLayer  layer {};

        CHECK( pB -> GetLayer( 7, layer ) == true );
        CHECK( pB -> GetLayer( "only_in_b", layer ) == true );
        CHECK( pB -> GetLayer( 2, layer ) == false );                       // map A's "props"

        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

        CHECK( pB -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 40, 10 }, pos ) == true );
        CHECK( pos.nTileCol == 1 );                                         // 32 px cells
        CHECK( pos.nTileRow == 0 );

        REQUIRE( pB -> GetLayer( 1, layer ) == true );

        SunLight :: TileMap :: stTile  tile {};

        REQUIRE( pB -> GetTile( SunLight :: TileMap :: stMatrixPosition { 1, 1 }, layer, tile ) == true );
        CHECK( tile.dimension.pos.x == 32 );
        CHECK( tile.dimension.size.nWidth == 32 );

        // The renderer itself: still map A.
        REQUIRE( scene.pRenderer -> GetMapInfo( info ) == true );
        CHECK( info.tileSize.nWidth == 16 );
        CHECK( scene.pRenderer -> GetLayer( 7, layer ) == false );
        CHECK( scene.pRenderer -> GetDefaultWorld() -> GetLayer( 2, layer ) == true );
    }

    TEST_CASE( "SetWorld: the default view always shows the default world; an extra view shows the world it is given" )  {

        Scene        scene;
        WorldHandle  pB    = scene.pRenderer -> CreateWorld();
        ViewHandle   pView = scene.pRenderer -> CreateView( g_SideRect );

        CHECK( scene.pRenderer -> GetDefaultView().SetWorld( pB ) == false );
        CHECK( scene.pRenderer -> GetDefaultView().SetWorld( scene.pRenderer -> GetDefaultWorld() ) == true );   // I21: changes nothing, a yes
        CHECK( scene.pRenderer -> GetDefaultView().GetWorld() == scene.pRenderer -> GetDefaultWorld() );

        CHECK( pView -> GetWorld() == scene.pRenderer -> GetDefaultWorld() );
        CHECK( pView -> SetWorld( nullptr ) == false );
        CHECK( pView -> SetWorld( pB ) == true );
        CHECK( pView -> GetWorld() == pB );
        CHECK( pView -> SetWorld( scene.pRenderer -> GetDefaultWorld() ) == true );   // and back
        CHECK( pView -> GetWorld() == scene.pRenderer -> GetDefaultWorld() );
    }

    TEST_CASE( "Each pass draws its own view's world: map A in the default view, map B in the extra one" )  {

        Scene        scene;
        WorldHandle  pB    = scene.pRenderer -> CreateWorld();
        ViewHandle   pView = scene.pRenderer -> CreateView( g_SideRect );

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );

        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect, __TILE_A ) == 16 );                // before SetWorld: map A twice
        CHECK( scene.TilesIn( g_SideRect, __TILE_B ) == 0 );

        REQUIRE( pView -> SetWorld( pB ) == true );
        scene.RunFrames( 1 );

        CHECK( scene.TilesIn( g_DefaultRect, __TILE_A ) == 16 );             // the default view: map A, whole
        CHECK( scene.TilesIn( g_DefaultRect, __TILE_B ) == 0 );
        CHECK( scene.TilesIn( g_SideRect, __TILE_B ) == 4 );                 // the extra view: map B, whole
        CHECK( scene.TilesIn( g_SideRect, __TILE_A ) == 0 );
    }

    TEST_CASE( "A view's camera, FitToMap and layer names work on the world it shows" )  {

        Scene        scene;
        WorldHandle  pB    = scene.pRenderer -> CreateWorld();
        ViewHandle   pView = scene.pRenderer -> CreateView( g_SideRect );

        pView -> SetScrollStepSize( -1, -1 );                                // "the tile size of the map it shows"
        REQUIRE( pView -> SetWorld( pB ) == true );
        CHECK( pView -> FitToMap() == false );                              // B has no map yet
        CHECK( pView -> ShowLayer( "ground", false ) == false );            // map A's name, not B's

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );
        CHECK( pView -> ShowLayer( "only_in_b", false ) == true );
        CHECK( pView -> IsLayerShown( 7 ) == false );

        // The scroll step's -1 resolved to B's 32 px tiles, not A's 16 (a step already resolved - CreateView
        // gives a view its map's tile size - stays, as it does across LoadMap).
        int  nStepW = 0, nStepH = 0;

        pView -> GetScrollStepSize( nStepW, nStepH );
        CHECK( nStepW == 32 );
        CHECK( nStepH == 32 );

        // FitToMap: map B is 64 px, the view 100: the largest zoom that fits it.
        REQUIRE( pView -> FitToMap() == true );

        float  fZoom = pView -> GetViewport().GetZoomProperties().fZoomFactor;

        CHECK( 64.0f * fZoom <= 100.0f );
        CHECK( 64.0f * ( fZoom + SunLight :: Base :: ZOOM_STEP ) > 100.0f );
    }

    TEST_CASE( "A sprite belongs to one world at a time; AddSprite on a world draws it in the views showing that world" )  {

        Scene           scene;
        WorldHandle     pB    = scene.pRenderer -> CreateWorld();
        ViewHandle      pView = scene.pRenderer -> CreateView( g_SideRect );
        TwoFrameSprite  rig( scene.engine() );

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );
        REQUIRE( pView -> SetWorld( pB ) == true );

        REQUIRE( pB -> AddSprite( 1, rig.sprite ) == true );
        CHECK( scene.pRenderer -> AddSprite( 1, rig.sprite ) == false );      // the default world: refused
        CHECK( scene.pRenderer -> GetDefaultWorld() -> AddSprite( 2, rig.sprite ) == false );
        CHECK( pB -> AddSprite( 7, rig.sprite ) == true );                     // another layer of ITS world: fine

        // Drawn by the view showing B only (16 px sprite frames, at B's position 0, 0 in the side view).
        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect, __TILE_A ) == 2 );                  // one per layer it is on
        CHECK( scene.TilesIn( g_DefaultRect, __TILE_A ) == 16 );             // map A only: no sprite

        CHECK( pB -> RemoveSprite( 1, rig.sprite ) == true );
        CHECK( pB -> RemoveSprite( 7, rig.sprite ) == true );
        CHECK( scene.pRenderer -> AddSprite( 1, rig.sprite ) == true );        // free again: the default world takes it
        scene.pRenderer -> RemoveSprite( 1, rig.sprite );
    }

    TEST_CASE( "Every live world updates every frame: the sprites of a world no view shows still advance" )  {

        Scene           scene;
        WorldHandle     pB = scene.pRenderer -> CreateWorld();
        TwoFrameSprite  rig( scene.engine() );

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );
        REQUIRE( pB -> AddSprite( 1, rig.sprite ) == true );
        CHECK( rig.OnSecond() == false );

        // No view shows B (single-view frames, then multi-view frames with an extra view on A).
        for( int nFrame = 0; nFrame < 3; nFrame++ )  {
            scene.clockFixture.clock.Advance( 20 );
            scene.RunFrames( 1 );
        }

        CHECK( rig.OnSecond() == true );                                       // advanced 3 times

        scene.pRenderer -> CreateView( g_SideRect );

        scene.clockFixture.clock.Advance( 20 );
        scene.RunFrames( 1 );
        CHECK( rig.OnSecond() == false );                                      // a 4th

        CHECK( scene.TilesIn( g_SideRect, __TILE_A ) == 16 );                // nothing of B drawn anywhere
        pB -> RemoveSprite( 1, rig.sprite );
    }

    TEST_CASE( "IWorld::UnloadMap detaches the world's sprites: no longer registered or colliding, but neither unloaded nor orphaned" )  {

        Scene             scene;
        WorldHandle       pB = scene.pRenderer -> CreateWorld();
        TwoFrameSprite    rig( scene.engine() ), other( scene.engine() );
        CountingListener  onB;

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );
        REQUIRE( pB -> AddSprite( 1, rig.sprite ) == true );
        REQUIRE( pB -> AddSprite( 7, other.sprite ) == true );                 // overlapping, at 0, 0
        REQUIRE( pB -> GetCollisionManager().AddColliderToColliderRule( 1, 7 ) == true );
        pB -> GetCollisionManager().AddCollisionListener( &onB );

        scene.RunFrames( 1 );
        REQUIRE( onB.nColliderHits > 0 );                                      // they collide while registered

        int  nUnloadsBefore = scene.engine().nUnloadTextureCalls;

        CHECK( pB -> UnloadMap() == true );
        CHECK( scene.engine().nUnloadTextureCalls == nUnloadsBefore + 1 );     // map B's tileset image only
        CHECK( pB -> RemoveSprite( 1, rig.sprite ) == false );                 // no longer registered
        CHECK( pB -> RemoveSprite( 7, other.sprite ) == false );

        onB.nColliderHits = 0;
        scene.RunFrames( 1 );
        CHECK( onB.nColliderHits == 0 );                                       // and their colliders are gone

        CHECK( rig.sprite.GetTextureSequenceSize( 0 ) == 2 );                  // not emptied (Sprite::Unload would)
        CHECK( rig.sprite.GetParent() == scene.pRenderer.get() );              // still parented, as after RemoveSprite
        CHECK( pB -> UnloadMap() == false );                                    // nothing left to unload
    }

    TEST_CASE( "RemoveWorld: its views go back to the default world, its sprites are detached and let go, its map is freed" )  {

        Scene           scene;
        WorldHandle     pB    = scene.pRenderer -> CreateWorld();
        ViewHandle      pView = scene.pRenderer -> CreateView( g_SideRect );
        TwoFrameSprite  rig( scene.engine() );

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );
        REQUIRE( pView -> SetWorld( pB ) == true );
        REQUIRE( pB -> AddSprite( 1, rig.sprite ) == true );

        int  nUnloadsBefore = scene.engine().nUnloadTextureCalls;

        REQUIRE( scene.pRenderer -> RemoveWorld( pB ) == true );
        CHECK( scene.engine().nUnloadTextureCalls == nUnloadsBefore + 1 );
        CHECK( pView -> GetWorld() == scene.pRenderer -> GetDefaultWorld() );
        CHECK( rig.sprite.GetParent() == nullptr );
        CHECK( rig.sprite.GetTextureSequenceSize( 0 ) == 2 );
        CHECK( scene.pRenderer -> AddSprite( 1, rig.sprite ) == true );        // free for another world
        scene.pRenderer -> RemoveSprite( 1, rig.sprite );

        scene.RunFrames( 1 );
        CHECK( scene.TilesIn( g_SideRect, __TILE_A ) == 16 );                // the view shows map A again
    }

    TEST_CASE( "Collisions are per world: a rule and a listener on one world never see another's colliders or map" )  {

        Scene             scene;
        WorldHandle       pB = scene.pRenderer -> CreateWorld();
        TwoFrameSprite    first( scene.engine() ), second( scene.engine() );
        CountingListener  onDefault, onB;

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );

        // A tile rule is checked against the world's OWN map: layer 7 exists in B only.
        CHECK( pB -> GetCollisionManager().AddColliderToTileRule( 1, 7 ) == true );
        CHECK( scene.pRenderer -> GetCollisionManager().AddColliderToTileRule( 1, 7 ) == false );

        // Two overlapping sprites in B, on layers 1 and 7, with a rule between those layers there.
        REQUIRE( pB -> AddSprite( 1, first.sprite ) == true );
        REQUIRE( pB -> AddSprite( 7, second.sprite ) == true );
        REQUIRE( pB -> GetCollisionManager().AddColliderToColliderRule( 1, 7 ) == true );
        REQUIRE( scene.pRenderer -> GetCollisionManager().AddColliderToColliderRule( 1, 2 ) == true );

        pB -> GetCollisionManager().AddCollisionListener( &onB );
        scene.pRenderer -> GetCollisionManager().AddCollisionListener( &onDefault );

        scene.RunFrames( 1 );

        CHECK( onB.nColliderHits > 0 );                                        // B's own rule fired, unshown
        CHECK( onDefault.nColliderHits == 0 );                                 // nothing reached the default world's

        pB -> RemoveSprite( 1, first.sprite );
        pB -> RemoveSprite( 7, second.sprite );
    }

    TEST_CASE( "A world handle outlives its renderer: inert, its sprites detached and no longer parented" )  {

        WorldHandle     pB;
        ViewHandle      pView;
        TwoFrameSprite  *pRig = nullptr;
        Scene           scene;

        pB    = scene.pRenderer -> CreateWorld();
        pView = scene.pRenderer -> CreateView( g_SideRect );
        pRig  = new TwoFrameSprite( scene.engine() );

        REQUIRE( pB -> LoadMap( "maps/b.tmx" ) == true );
        REQUIRE( pView -> SetWorld( pB ) == true );
        REQUIRE( pB -> AddSprite( 1, pRig -> sprite ) == true );

        scene.pRenderer -> Stop();
        scene.pRenderer.reset();

        CHECK( pB -> IsRemoved() == true );
        CHECK( pView -> GetWorld() == pB );                                    // a removed view keeps its last world
        CHECK( pRig -> sprite.GetParent() == nullptr );
        CHECK( pB -> AddSprite( 1, pRig -> sprite ) == false );

        SunLight :: TileMap :: stLayer  layer {};

        CHECK( pB -> GetCollisionManager().AddColliderToTileRule( 1, 7 ) == false );   // the world's map is gone
        delete pRig;
    }
}
