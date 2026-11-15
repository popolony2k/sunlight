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
 * Several maps in one world, at offsets (I19d of the master plan): AddMap, the world's layer table
 * (one id per layer name), drawing layer by layer across the maps with the sprites once, picking and
 * tiles in the world's pixels, the camera's limits from the box every map fits in (negative offsets
 * too), and collisions against the map under the collider. Draws are told apart by texture handle:
 * every map's tileset gets its own (MockEngine::hLoadTextureResult set before each load).
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
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
typedef MockEngine :: Event                             Event;

namespace  {

    const SunLight :: Base :: TextureHandle  g_TexA      = ( SunLight :: Base :: TextureHandle ) 0xA0;
    const SunLight :: Base :: TextureHandle  g_TexC      = ( SunLight :: Base :: TextureHandle ) 0xC0;
    const SunLight :: Base :: TextureHandle  g_TexIso    = ( SunLight :: Base :: TextureHandle ) 0xE0;
    const SunLight :: Base :: TextureHandle  g_TexSprite = ( SunLight :: Base :: TextureHandle ) 0x77;

    Bytes ToBytes( const std :: string& str )  {
        return Bytes( str.begin(), str.end() );
    }

    // Map A: 4 x 4 of 16 px; "ground" (id 1) everywhere, "props" (id 2) one tile at row 0, column 0.
    const char  *g_szMapA =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
        "<tileset firstgid=\"1\" name=\"a\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"a.png\" width=\"16\" height=\"16\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
        "<layer id=\"2\" name=\"props\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</data></layer>"
        "</map>";

    // Map C: 2 x 2 of 16 px; "props" (ITS id 1 - map A's name, another id) one tile at row 0, column 0,
    // whose tile has a 16 x 16 collision rectangle; "roof" (id 2, a name map A has not) one at row 1, column 1.
    const char  *g_szMapC =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\">"
        "<tileset firstgid=\"1\" name=\"c\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"c.png\" width=\"16\" height=\"16\"/>"
        "<tile id=\"0\"><objectgroup draworder=\"index\"><object id=\"1\" x=\"0\" y=\"0\" width=\"16\" height=\"16\"/></objectgroup></tile></tileset>"
        "<layer id=\"1\" name=\"props\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,0,0,0</data></layer>"
        "<layer id=\"2\" name=\"roof\" width=\"2\" height=\"2\"><data encoding=\"csv\">0,0,0,1</data></layer>"
        "</map>";

    // An isometric 2 x 2 map of 32 x 16 tiles: a 64 x 32 box, a diamond with four empty corners.
    const char  *g_szMapIso =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"isometric\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"32\" tileheight=\"16\" nextlayerid=\"2\">"
        "<tileset firstgid=\"1\" name=\"i\" tilewidth=\"32\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"i.png\" width=\"32\" height=\"16\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,1,1,1</data></layer>"
        "</map>";

    // An isometric 2 x 1 map of ODD tiles (33 x 17): Tiled's box is (2 + 1) x 33 / 2 by (2 + 1) x 17 / 2,
    // in whole pixels - 49 x 25.
    const char  *g_szMapOdd =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"isometric\" renderorder=\"right-down\" width=\"2\" height=\"1\" tilewidth=\"33\" tileheight=\"17\" nextlayerid=\"2\">"
        "<tileset firstgid=\"1\" name=\"o\" tilewidth=\"33\" tileheight=\"17\" tilecount=\"1\" columns=\"1\"><image source=\"o.png\" width=\"33\" height=\"17\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"1\"><data encoding=\"csv\">1,1</data></layer>"
        "</map>";

    // A staggered map ONE row high (axis y, index even: row 0 is the shifted one), 32 x 16 tiles, 2
    // columns - the one case where Tiled's pixel box does not start at (0, 0): its HexagonalRenderer boxes
    // it as (16, 0, 64, 16), where sunlight's MapPixelSize gives (0, 0, 80, 16). Both draw the cells at the
    // same place - the map's origin at its offset, the shifted row half a tile right. (Axis x is the
    // same case turned; libtmx does not parse Tiled's staggeraxis="x" - see the master plan.)
    const char  *g_szMapStaggeredRow =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"staggered\" renderorder=\"right-down\" width=\"2\" height=\"1\" tilewidth=\"32\" tileheight=\"16\""
        " staggeraxis=\"y\" staggerindex=\"even\" nextlayerid=\"2\">"
        "<tileset firstgid=\"1\" name=\"s\" tilewidth=\"32\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"s.png\" width=\"32\" height=\"16\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"2\" height=\"1\"><data encoding=\"csv\">1,1</data></layer>"
        "</map>";

    // An isometric 2 x 2 map of 32 x 16 tiles with a "props" layer: one tile at row 0, column 0, whose
    // tile has a 32 x 16 collision rectangle. Cell (0, 0) is drawn at x = (0 - 0 + 2 - 1) x 16 = 16, y = 0.
    const char  *g_szMapIsoProps =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"isometric\" renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"32\" tileheight=\"16\" nextlayerid=\"2\">"
        "<tileset firstgid=\"1\" name=\"ip\" tilewidth=\"32\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"ip.png\" width=\"32\" height=\"16\"/>"
        "<tile id=\"0\"><objectgroup draworder=\"index\"><object id=\"1\" x=\"0\" y=\"0\" width=\"32\" height=\"16\"/></objectgroup></tile></tileset>"
        "<layer id=\"1\" name=\"props\" width=\"2\" height=\"2\"><data encoding=\"csv\">1,0,0,0</data></layer>"
        "</map>";

    // Mocks; a started renderer with map A in its default world (default view at 10, 10, 200 x 200).
    struct Scene  {

        MockClockFixture                    clockFixture;
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/a.tmx"]   = ToBytes( g_szMapA );
            fsFixture.fs.files["maps/c.tmx"]   = ToBytes( g_szMapC );
            fsFixture.fs.files["maps/iso.tmx"] = ToBytes( g_szMapIso );
            fsFixture.fs.files["maps/odd.tmx"] = ToBytes( g_szMapOdd );
            fsFixture.fs.files["maps/col.tmx"] = ToBytes( g_szMapStaggeredRow );
            fsFixture.fs.files["maps/isoprops.tmx"] = ToBytes( g_szMapIsoProps );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = SunLight :: Base :: stDimension2D { { 10, 10 }, { 200, 200 } };

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );

            engine().hLoadTextureResult = g_TexA;
            REQUIRE( pRenderer -> LoadMap( "maps/a.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }
        WorldHandle Default( void )  { return pRenderer -> GetDefaultWorld(); }

        int AddMap( WorldHandle pWorld, const char* szPath, SunLight :: Base :: TextureHandle hTexture, int nX, int nY )  {
            engine().hLoadTextureResult = hTexture;
            return pWorld -> AddMap( szPath, nX, nY );
        }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // The tile draws of one texture, in order, as (x, y).
        std :: vector<std :: pair<float, float>> Draws( SunLight :: Base :: TextureHandle hTexture )  {
            std :: vector<std :: pair<float, float>>  draws;

            for( const Event& evt : engine().events )
                if( ( evt.kind == Event :: TILE ) && ( evt.handle == hTexture ) )
                    draws.push_back( { evt.x, evt.y } );

            return draws;
        }

        // Index in the event log of the first tile draw of a texture at (x, y), or -1.
        int IndexOf( SunLight :: Base :: TextureHandle hTexture, float fX, float fY )  {
            for( int nIdx = 0; nIdx < ( int ) engine().events.size(); nIdx++ )  {
                const Event  &evt = engine().events[nIdx];

                if( ( evt.kind == Event :: TILE ) && ( evt.handle == hTexture ) && ( evt.x == fX ) && ( evt.y == fY ) )
                    return nIdx;
            }

            return -1;
        }
    };

    struct TileListener : public SunLight :: Collision :: ICollisionListener  {
        std :: vector<SunLight :: TileMap :: stTile>  tiles;

        void OnCollision( SunLight :: Collision :: Collider*, SunLight :: Collision :: Collider* ) override  {}
        void OnCollision( SunLight :: Collision :: Collider*, SunLight :: TileMap :: stTile* pTile ) override  { tiles.push_back( *pTile ); }
    };
}

TEST_SUITE( "renderer/worldmaps" )  {

    TEST_CASE( "AddMap places maps in a world: ids never reused, offsets kept, LoadMap makes it one map again" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();

        CHECK( pWorld -> GetMapCount() == 1 );

        int  nC = scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );
        int  nI = scene.AddMap( pWorld, "maps/iso.tmx", g_TexIso, -64, 96 );

        CHECK( nC > 0 );
        CHECK( nI > nC );
        CHECK( pWorld -> GetMapCount() == 3 );
        CHECK( scene.AddMap( pWorld, "maps/missing.tmx", g_TexC, 0, 0 ) == -1 );

        int  nX = 0, nY = 0;

        REQUIRE( pWorld -> GetMapOffset( nI, nX, nY ) == true );
        CHECK( nX == -64 );
        CHECK( nY == 96 );
        CHECK( pWorld -> GetMapOffset( 999, nX, nY ) == false );

        CHECK( pWorld -> RemoveMap( nC ) == true );
        CHECK( pWorld -> RemoveMap( nC ) == false );
        CHECK( pWorld -> GetMapCount() == 2 );

        // A new world takes its first map from AddMap too.
        WorldHandle  pB = scene.pRenderer -> CreateWorld();

        CHECK( scene.AddMap( pB, "maps/c.tmx", g_TexC, 10, 20 ) >= 0 );
        CHECK( pB -> GetMapCount() == 1 );

        // LoadMap: the only map, at (0, 0).
        scene.engine().hLoadTextureResult = g_TexA;
        REQUIRE( pWorld -> LoadMap( "maps/a.tmx" ) == true );
        CHECK( pWorld -> GetMapCount() == 1 );

        // Removed: refused.
        REQUIRE( scene.pRenderer -> RemoveWorld( pB ) == true );
        CHECK( pB -> AddMap( "maps/c.tmx", 0, 0 ) == -1 );
        CHECK( pB -> GetMapOffset( 0, nX, nY ) == false );
    }

    TEST_CASE( "One world layer id per NAME: the first map's ids kept, a new name numbered above the highest, stable while maps remain" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();

        CHECK( pWorld -> GetLayerId( "ground" ) == 1 );
        CHECK( pWorld -> GetLayerId( "props" ) == 2 );
        CHECK( pWorld -> GetLayerId( "roof" ) == -1 );

        int  nC = scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );

        CHECK( pWorld -> GetLayerId( "props" ) == 2 );                // map C's own id 1: the world's 2
        CHECK( pWorld -> GetLayerId( "roof" ) == 3 );                 // map C's own id 2: new, above 2

        SunLight :: TileMap :: stLayer  layer {};

        CHECK( pWorld -> GetLayer( 3, layer ) == true );              // the world's id finds map C's roof
        CHECK( std :: string( layer.pLayer -> name ) == "roof" );

        // Removing the first map keeps every id.
        int  nFirst = -1;

        for( int nId = 0; nId < nC; nId++ )  {
            int  nX = 0, nY = 0;

            if( pWorld -> GetMapOffset( nId, nX, nY ) )
                nFirst = nId;
        }

        REQUIRE( nFirst >= 0 );
        REQUIRE( pWorld -> RemoveMap( nFirst ) == true );
        CHECK( pWorld -> GetLayerId( "props" ) == 2 );
        CHECK( pWorld -> GetLayerId( "roof" ) == 3 );
        CHECK( pWorld -> GetLayerId( "ground" ) == 1 );               // the table keeps it while maps remain
    }

    TEST_CASE( "The world's bounds: every map's pixel box, negative offsets included; Tiled's whole-pixel box for odd tiles" )  {

        Scene                              scene;
        WorldHandle                        pWorld = scene.Default();
        SunLight :: Base :: stDimension2D  bounds {};

        REQUIRE( pWorld -> GetBounds( bounds ) == true );
        CHECK( bounds.pos.x == 0 );
        CHECK( bounds.size.nWidth == 64 );                             // map A alone: its own 64 x 64

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );
        REQUIRE( pWorld -> GetBounds( bounds ) == true );
        CHECK( bounds.size.nWidth == 96 );
        CHECK( bounds.size.nHeight == 64 );

        scene.AddMap( pWorld, "maps/iso.tmx", g_TexIso, -32, -16 );   // a 64 x 32 box
        REQUIRE( pWorld -> GetBounds( bounds ) == true );
        CHECK( bounds.pos.x == -32 );
        CHECK( bounds.pos.y == -16 );
        CHECK( bounds.size.nWidth == 128 );                            // -32 .. 96
        CHECK( bounds.size.nHeight == 80 );                            // -16 .. 64

        WorldHandle  pOdd = scene.pRenderer -> CreateWorld();

        scene.AddMap( pOdd, "maps/odd.tmx", g_TexIso, 0, 0 );
        REQUIRE( pOdd -> GetBounds( bounds ) == true );
        CHECK( bounds.size.nWidth == 49 );                             // 3 x 33 / 2, truncated, as Tiled
        CHECK( bounds.size.nHeight == 25 );                            // 3 x 17 / 2
    }

    TEST_CASE( "A staggered map one row high: its cells where Tiled draws them (origin at the offset), its box from the origin" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.pRenderer -> CreateWorld();
        int          nRow   = scene.AddMap( pWorld, "maps/col.tmx", g_TexIso, 100, 50 );

        REQUIRE( nRow >= 0 );

        SunLight :: TileMap :: stTile      tile {};
        SunLight :: Base :: stDimension2D  bounds {};

        REQUIRE( pWorld -> GetTile( nRow, SunLight :: TileMap :: stMatrixPosition { 0, 0 }, pWorld -> GetLayerId( "ground" ), tile ) == true );
        CHECK( tile.dimension.pos.x == 100 + 16 );                     // the shifted row: half a tile right, as Tiled
        CHECK( tile.dimension.pos.y == 50 );

        REQUIRE( pWorld -> GetBounds( bounds ) == true );
        CHECK( bounds.pos.x == 100 );                                  // sunlight's box keeps the empty left strip...
        CHECK( bounds.size.nWidth == 80 );                             // ...and ends where Tiled's does: 100 + 16 + 64
        CHECK( bounds.size.nHeight == 16 );
    }

    TEST_CASE( "Picking in the world's pixels: the map and its cell; an isometric map's empty corner falls through to the next map" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();
        int          nC     = scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );
        int          nMapId = -1;

        SunLight :: TileMap :: stMatrixPosition  pos { -1, -1 };

        REQUIRE( pWorld -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 85, 20 }, nMapId, pos ) == true );
        CHECK( nMapId == nC );
        CHECK( pos.nTileCol == 1 );                                    // 85 - 64 = 21: column 1
        CHECK( pos.nTileRow == 1 );

        REQUIRE( pWorld -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 20, 40 }, nMapId, pos ) == true );
        CHECK( nMapId != nC );                                         // map A
        CHECK( pos.nTileCol == 1 );
        CHECK( pos.nTileRow == 2 );

        CHECK( pWorld -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 80, 50 }, nMapId, pos ) == false );   // below map C: no map
        CHECK( pWorld -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { -5, 5 }, nMapId, pos ) == false );

        // An isometric map over map A's corner, added FIRST in a world of its own: (2, 2) is in its box
        // but outside its diamond, so map A (behind it, at the same place) answers.
        WorldHandle  pB  = scene.pRenderer -> CreateWorld();
        int          nIso = scene.AddMap( pB, "maps/iso.tmx", g_TexIso, 0, 0 );
        int          nA   = scene.AddMap( pB, "maps/a.tmx", g_TexA, 0, 0 );

        REQUIRE( pB -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 2, 2 }, nMapId, pos ) == true );
        CHECK( nMapId == nA );
        REQUIRE( pB -> TileMapToTileMatrix( SunLight :: Base :: stCoordinate2D { 32, 16 }, nMapId, pos ) == true );
        CHECK( nMapId == nIso );                                       // the diamond's centre
    }

    TEST_CASE( "GetTile with a map id: that map's tile, its rectangle in the world's pixels" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();
        int          nC     = scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 32 );

        SunLight :: TileMap :: stTile  tile {};

        REQUIRE( pWorld -> GetTile( nC, SunLight :: TileMap :: stMatrixPosition { 1, 1 }, pWorld -> GetLayerId( "roof" ), tile ) == true );
        CHECK( tile.dimension.pos.x == 64 + 16 );
        CHECK( tile.dimension.pos.y == 32 + 16 );
        CHECK( pWorld -> GetTile( nC, SunLight :: TileMap :: stMatrixPosition { 0, 0 }, pWorld -> GetLayerId( "ground" ), tile ) == false );   // C has no ground
        CHECK( pWorld -> GetTile( 999, SunLight :: TileMap :: stMatrixPosition { 0, 0 }, 1, tile ) == false );

        // The older GetTile reads the first map, and refuses another map's layer.
        SunLight :: TileMap :: stLayer  roof {};

        REQUIRE( pWorld -> GetLayer( "roof", roof ) == true );
        CHECK( pWorld -> GetTile( SunLight :: TileMap :: stMatrixPosition { 1, 1 }, roof, tile ) == false );
    }

    TEST_CASE( "Drawing: each map at its offset, layer by layer across the maps, a sprite once after its layer of every map" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );

        // A sprite on "props" (world id 2), at world position (70, 2) - over map C.
        scene.engine().hLoadTextureResult = g_TexSprite;
        scene.engine().nLoadTextureWidth  = 16;
        scene.engine().nLoadTextureHeight = 16;

        SunLight :: Canvas :: TextureCanvas  canvas;
        SunLight :: Sprite :: Sprite         sprite;

        REQUIRE( canvas.Load( "s.png" ) == true );
        canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 16, 16 } } );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetWorldSpace( true );
        sprite.GetDimension2D().pos.x = 70;
        sprite.GetDimension2D().pos.y = 2;
        REQUIRE( scene.pRenderer -> AddSprite( 2, sprite ) == true );

        scene.RunFrames( 1 );

        // Map A: 16 ground + 1 prop, from the viewport origin (10, 10); map C: 1 prop + 1 roof, 64 px right.
        CHECK( scene.Draws( g_TexA ).size() == 17 );
        CHECK( scene.Draws( g_TexC ).size() == 2 );

        int  nAGround = scene.IndexOf( g_TexA, 58, 58 );              // A's last ground tile (row 3, col 3)
        int  nCProps  = scene.IndexOf( g_TexC, 74, 10 );              // C's prop, at 10 + 64
        int  nCRoof   = scene.IndexOf( g_TexC, 90, 26 );              // C's roof, row 1, col 1
        int  nSprite  = scene.IndexOf( g_TexSprite, 80, 12 );         // 10 + 70, 10 + 2

        REQUIRE( nAGround >= 0 );
        REQUIRE( nCProps >= 0 );
        REQUIRE( nCRoof >= 0 );
        REQUIRE( nSprite >= 0 );
        CHECK( nAGround < nCProps );                                   // "ground" before "props"
        CHECK( nCProps < nSprite );                                    // the sprite after "props" of EVERY map...
        CHECK( nSprite < nCRoof );                                     // ...and before "roof"
        CHECK( scene.Draws( g_TexSprite ).size() == 1 );               // once

        // A view mask by NAME hides map C's roof: the world's id.
        REQUIRE( scene.pRenderer -> GetDefaultView().ShowLayer( "roof", false ) == true );
        CHECK( scene.pRenderer -> GetDefaultView().IsLayerShown( 3 ) == false );
        scene.RunFrames( 1 );
        CHECK( scene.Draws( g_TexC ).size() == 1 );

        scene.pRenderer -> RemoveSprite( 2, sprite );
    }

    TEST_CASE( "I21: SetLayer on a world of several maps changes that layer in EVERY map that has it - by id, by name, through IWorld or the renderer" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();
        WorldHandle  pOther = scene.pRenderer -> CreateWorld();

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );            // its "props" is ITS layer 1
        scene.AddMap( pOther, "maps/a.tmx", g_TexA, 0, 0 );
        scene.AddMap( pOther, "maps/c.tmx", g_TexC, 64, 0 );

        const SunLight :: Base :: stColor  red { 255, 0, 0, 255 };

        // Through the world handle, by name: the props tile of map A (10, 10) and of map C (74, 10) both red.
        SunLight :: TileMap :: stLayer  layer {};

        REQUIRE( pWorld -> GetLayer( "props", layer ) == true );
        layer.tint = red;
        REQUIRE( pWorld -> SetLayer( "props", layer ) == true );
        CHECK( std :: string( layer.pLayer -> name ) == "props" );

        scene.RunFrames( 1 );

        int  nRed = 0, nGround = 0;

        for( const Event& evt : scene.engine().events )  {
            if( evt.kind != Event :: TILE )
                continue;

            bool  bProp = ( ( evt.handle == g_TexA ) && ( evt.x == 10 ) && ( evt.y == 10 ) && ( evt.color.nGreen == 0 ) ) ||
                          ( ( evt.handle == g_TexC ) && ( evt.x == 74 ) && ( evt.y == 10 ) );

            if( bProp && ( evt.color.nRed == 255 ) && ( evt.color.nGreen == 0 ) )
                nRed++;
            else if( ( evt.handle == g_TexA ) && ( evt.color.nGreen == 255 ) )
                nGround++;
        }

        CHECK( nRed == 2 );                                               // one prop per map
        CHECK( nGround == 16 );                                           // the ground keeps its colour

        // Through the renderer's own SetLayer (the default world), by id: hidden in both maps.
        REQUIRE( scene.pRenderer -> GetLayer( 2, layer ) == true );
        layer.bVisible = false;
        REQUIRE( scene.pRenderer -> SetLayer( 2, layer ) == true );
        scene.RunFrames( 1 );
        CHECK( scene.Draws( g_TexA ).size() == 16 );                      // the ground only
        CHECK( scene.Draws( g_TexC ).size() == 1 );                       // map C's roof only

        // A created world, by world id: its "roof" (id 3) exists only in its second map.
        REQUIRE( pOther -> GetLayer( 3, layer ) == true );
        layer.bVisible = false;
        CHECK( pOther -> SetLayer( pOther -> GetLayerId( "roof" ), layer ) == true );
        REQUIRE( pOther -> GetLayer( "roof", layer ) == true );
        CHECK( layer.bVisible == false );

        // Nothing there: refused.
        CHECK( pWorld -> SetLayer( "nowhere", layer ) == false );
        CHECK( pWorld -> SetLayer( 99, layer ) == false );
        REQUIRE( scene.pRenderer -> RemoveWorld( pOther ) == true );
        CHECK( pOther -> SetLayer( "roof", layer ) == false );
    }

    TEST_CASE( "The camera's limits come from every map: ResetCamera at the world's top-left, scrolling stops at its edges" )  {

        Scene        scene;
        WorldHandle  pWorld = scene.Default();

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, -32, -16 );       // the world: -32 .. 64 by -16 .. 64

        int  nX = 0, nY = 0;

        scene.pRenderer -> ResetCamera();
        scene.pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == -32 );                                            // the map point at the view's top-left
        CHECK( nY == -16 );

        scene.pRenderer -> SetScrollStepSize( 16, 16 );
        scene.pRenderer -> MoveCameraRight();                         // already at the left edge
        scene.pRenderer -> MoveCameraDown();                          // already at the top edge
        scene.pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == -32 );
        CHECK( nY == -16 );

        // The view (200 x 200 at zoom 1) is larger than the 96 x 80 world: no scrolling the other way.
        scene.pRenderer -> MoveCameraLeft();
        scene.pRenderer -> MoveCameraUp();
        scene.pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == -32 );
        CHECK( nY == -16 );

        // FitToMap: the whole world (96 x 80) in the view.
        REQUIRE( scene.pRenderer -> GetDefaultView().FitToMap() == true );

        float  fZoom = scene.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;

        CHECK( 96.0f * fZoom <= 200.0f );
        CHECK( 96.0f * ( fZoom + SunLight :: Base :: ZOOM_STEP ) > 200.0f );
        scene.pRenderer -> GetCameraPosition( nX, nY );
        CHECK( nX == -32 );
        CHECK( nY == -16 );
    }

    // A world-space collider sprite of a given size, on layer 1, for the collision tests below.
    struct ColliderRig  {
        SunLight :: Canvas :: TextureCanvas  canvas;
        SunLight :: Sprite :: Sprite         sprite;
        Scene                               &m_Scene;

        ColliderRig( Scene& scene, int nX, int nY, int nW, int nH ) : m_Scene( scene )  {
            scene.engine().hLoadTextureResult = g_TexSprite;
            scene.engine().nLoadTextureWidth  = nW;
            scene.engine().nLoadTextureHeight = nH;

            REQUIRE( canvas.Load( "s.png" ) == true );
            canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { nW, nH } } );
            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );
            sprite.SetWorldSpace( true );
            sprite.GetDimension2D().pos.x = nX;
            sprite.GetDimension2D().pos.y = nY;
            REQUIRE( scene.pRenderer -> AddSprite( 1, sprite ) == true );
        }

        ~ColliderRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( 1, sprite );
        }
    };

    TEST_CASE( "Collisions in a world of several maps: the map under the collider, its own layer of that name, tiles in the world's pixels" )  {

        Scene         scene;
        WorldHandle   pWorld = scene.Default();
        TileListener  listener;

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );

        scene.engine().hLoadTextureResult = g_TexSprite;
        scene.engine().nLoadTextureWidth  = 16;
        scene.engine().nLoadTextureHeight = 16;

        SunLight :: Canvas :: TextureCanvas  canvas;
        SunLight :: Sprite :: Sprite         sprite;

        REQUIRE( canvas.Load( "s.png" ) == true );
        canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 8, 8 } } );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        sprite.SetWorldSpace( true );
        sprite.GetDimension2D().pos.x = 68;                            // over map C's prop (64 .. 80, 0 .. 16)
        sprite.GetDimension2D().pos.y = 4;
        REQUIRE( scene.pRenderer -> AddSprite( 1, sprite ) == true );

        // The rule names "props" by its WORLD id (2): map C's own "props" is its layer 1.
        REQUIRE( scene.pRenderer -> GetCollisionManager().AddColliderToTileRule( 1, 2 ) == true );
        scene.pRenderer -> GetCollisionManager().AddCollisionListener( &listener );

        scene.RunFrames( 1 );

        REQUIRE( listener.tiles.size() == 1 );
        CHECK( listener.tiles[0].dimension.pos.x == 64 );              // in the world's pixels
        CHECK( listener.tiles[0].dimension.pos.y == 0 );

        // Over map A's prop instead (no collision shape there): nothing.
        listener.tiles.clear();
        sprite.GetDimension2D().pos.x = 4;
        scene.RunFrames( 1 );
        CHECK( listener.tiles.empty() );

        scene.pRenderer -> RemoveSprite( 1, sprite );
    }

    TEST_CASE( "I19e: a collider straddling a map edge touches the tiles of EVERY map it overlaps, orthogonal and isometric, in the world's pixels" )  {

        Scene         scene;
        WorldHandle   pWorld = scene.Default();
        TileListener  listener;

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );              // its prop: 64 .. 80 by 0 .. 16
        scene.AddMap( pWorld, "maps/isoprops.tmx", g_TexIso, 80, 0 );     // its prop's tile: 96 .. 128 by 0 .. 16

        // 76 .. 100 by 4 .. 12: over map C's prop AND the isometric map's.
        ColliderRig  rig( scene, 76, 4, 24, 8 );

        REQUIRE( scene.pRenderer -> GetCollisionManager().AddColliderToTileRule( 1, pWorld -> GetLayerId( "props" ) ) == true );
        scene.pRenderer -> GetCollisionManager().AddCollisionListener( &listener );

        scene.RunFrames( 1 );

        REQUIRE( listener.tiles.size() == 2 );
        CHECK( listener.tiles[0].dimension.pos.x == 64 );              // map C first (added first)...
        CHECK( listener.tiles[0].dimension.pos.y == 0 );
        CHECK( listener.tiles[1].dimension.pos.x == 96 );              // ...then the isometric map's, 80 + 16
        CHECK( listener.tiles[1].dimension.pos.y == 0 );
        CHECK( listener.tiles[1].dimension.size.nWidth == 32 );

        // 66 .. 90: map C's tile only (the isometric one starts at 96), as a single map would report it.
        listener.tiles.clear();
        rig.sprite.GetDimension2D().pos.x = 66;
        scene.RunFrames( 1 );
        REQUIRE( listener.tiles.size() == 1 );
        CHECK( listener.tiles[0].dimension.pos.x == 64 );
    }

    TEST_CASE( "I19e: a collider between the maps touches nothing; a map without the rule's layer is skipped" )  {

        Scene         scene;
        WorldHandle   pWorld = scene.Default();
        TileListener  listener;

        scene.AddMap( pWorld, "maps/c.tmx", g_TexC, 64, 0 );              // 64 .. 96 by 0 .. 32
        scene.AddMap( pWorld, "maps/iso.tmx", g_TexIso, 64, 64 );        // no "props" layer at all

        REQUIRE( scene.pRenderer -> GetCollisionManager().AddColliderToTileRule( 1, pWorld -> GetLayerId( "props" ) ) == true );
        scene.pRenderer -> GetCollisionManager().AddCollisionListener( &listener );

        // In no map's box: right of map A, below map C, above the isometric map.
        ColliderRig  rig( scene, 70, 40, 8, 8 );

        scene.RunFrames( 1 );
        CHECK( listener.tiles.empty() );

        // Across map A's edge into the isometric map (which has no "props"): map A's props has no shape there.
        rig.sprite.GetDimension2D().pos.x = 60;
        rig.sprite.GetDimension2D().pos.y = 60;
        scene.RunFrames( 1 );
        CHECK( listener.tiles.empty() );
    }
}
