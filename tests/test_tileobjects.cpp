/*
 * Tile objects (B2 of the master plan): an object with a gid is drawn as that tile, at
 * the object's position. Tiled places a tile object by its bottom-left corner, so the
 * tile's top-left is at (x, y - tile height). The tile is clipped to the viewport like a
 * map tile. Drawn through the renderer against MockEngine, so the TILE events say what
 * was drawn, where, and from which part of the tileset.
 */

#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"
#include "mock_clock.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    const int           kTileSize              = 16;     // the tileset's tiles are 16 x 16
    const unsigned int  kFirstTileGID          = 1;      // gid 1: the first tile, at source x 0
    const unsigned int  kSecondTileGID         = 2;      // gid 2: the second tile, at source x 16
    const unsigned int  kFlippedSecondTileGID  = 0x80000002u;   // gid 2 with the horizontal-flip bit set
    const unsigned int  kPastTilesetGID        = 5;      // the tileset has two tiles, so 5 has none
    const int           kFrameMillis           = 100;    // each animation frame of tile 0
    const int           kViewportOriginX       = 40;     // the viewport's top-left, in window pixels
    const int           kViewportOriginY       = 40;
    const int           kViewportWidth         = 1000;
    const int           kViewportHeight        = 800;

    // A tileset of two tiles (gids 1 and 2), and a map with one object group holding the
    // given objects. No tile layer: every TILE event is one of the objects.
    Bytes MakeTmx( const std :: string &strObjects )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"60\" height=\"40\""
            << " tilewidth=\"" << kTileSize << "\" tileheight=\"" << kTileSize << "\" nextlayerid=\"3\" nextobjectid=\"10\">"
            << "<tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"" << kTileSize << "\" tileheight=\"" << kTileSize
            << "\" tilecount=\"2\" columns=\"2\">"
            << "<image source=\"tiles.png\" width=\"32\" height=\"16\"/>"
            << "<tile id=\"0\"><animation>"
            << "<frame tileid=\"0\" duration=\"" << kFrameMillis << "\"/>"
            << "<frame tileid=\"1\" duration=\"" << kFrameMillis << "\"/>"
            << "</animation></tile></tileset>"
            << "<objectgroup id=\"1\" name=\"sprites\">" << strObjects << "</objectgroup>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // One tile object of the tile size, with the gid and position given by the caller.
    std :: string TileObject( int nId, unsigned int nGID, int nX, int nY )  {

        std :: ostringstream  xml;

        xml << "<object id=\"" << nId << "\" gid=\"" << nGID << "\" x=\"" << nX << "\" y=\"" << nY
            << "\" width=\"" << kTileSize << "\" height=\"" << kTileSize << "\"/>";

        return xml.str();
    }

    struct Scene  {

        MockClockFixture                    clockFixture;   // installed before anything timestamps against it
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        explicit Scene( const std :: string &strObjects )  {

            fsFixture.fs.files["maps/sprites.tmx"] = MakeTmx( strObjects );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport.emplace();
            config.viewport -> pos.x        = kViewportOriginX;
            config.viewport -> pos.y        = kViewportOriginY;
            config.viewport -> size.nWidth  = kViewportWidth;
            config.viewport -> size.nHeight = kViewportHeight;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/sprites.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

            pRenderer -> SetCameraPosition( 0, 0 );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        double Zoom( void )  { return pRenderer -> GetView( 0 ) -> GetViewport().GetZoomProperties().fZoomFactor; }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // The SCALED events of the last frame, in drawing order. The frame's own blit to the
        // window is one of them too, and it comes last, after the objects.
        std :: vector<Event> Scaled( void )  {
            std :: vector<Event>  scaled;

            for( const Event &evt : engine().events )
                if( evt.kind == Event :: SCALED )
                    scaled.push_back( evt );

            return scaled;
        }

        // The first scaled draw of the frame: the object's, since the blit comes after it.
        Event FirstScaled( void )  {
            std :: vector<Event>  scaled = Scaled();

            return scaled.empty() ? Event {} : scaled[0];
        }

        // The TILE events of the last frame, in drawing order.
        std :: vector<Event> Tiles( void )  {
            std :: vector<Event>  tiles;

            for( const Event &evt : engine().events )
                if( evt.kind == Event :: TILE )
                    tiles.push_back( evt );

            return tiles;
        }
    };
}

TEST_SUITE( "renderer/tile objects" )  {

    TEST_CASE( "A tile object is drawn with its bottom-left at the object's position" )  {

        // The object's y is its bottom edge: the tile's top is kTileSize above it.
        const int  nObjectX = 100;
        const int  nObjectY = 50;

        Scene  scene( TileObject( 1, kSecondTileGID, nObjectX, nObjectY ) );

        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.Tiles();

        REQUIRE( tiles.size() == 1 );

        double  fZoom = scene.Zoom();
        CHECK( tiles[0].x == ( float ) ( nObjectX * fZoom + kViewportOriginX ) );
        CHECK( tiles[0].y == ( float ) ( ( nObjectY - kTileSize ) * fZoom + kViewportOriginY ) );
        CHECK( tiles[0].w == ( float ) ( kTileSize * fZoom ) );
        CHECK( tiles[0].h == ( float ) ( kTileSize * fZoom ) );
    }

    TEST_CASE( "The source rectangle is the tile the gid names" )  {

        Scene  scene( TileObject( 1, kSecondTileGID, 100, 50 ) );

        scene.RunFrames( 1 );

        REQUIRE( scene.Tiles().size() == 1 );
        CHECK( scene.Tiles()[0].srcX == ( float ) kTileSize );   // the second tile starts one tile along
    }

    TEST_CASE( "The flip bits of the gid are ignored: a flipped gid draws the same tile" )  {

        Scene  scene( TileObject( 1, kFlippedSecondTileGID, 100, 50 ) );

        scene.RunFrames( 1 );

        REQUIRE( scene.Tiles().size() == 1 );
        CHECK( scene.Tiles()[0].srcX == ( float ) kTileSize );
    }

    TEST_CASE( "A gid past the tileset draws nothing, and does not read past the tile table" )  {

        Scene  scene( TileObject( 1, kPastTilesetGID, 100, 50 ) );

        scene.RunFrames( 1 );

        CHECK( scene.Tiles().empty() );
    }

    TEST_CASE( "A tile object crossing the left viewport edge is cut at that edge" )  {

        // With the camera at (cameraX, cameraY), the viewport's left edge is at x = cameraX in the map.
        // The tile starts overhangX px left of that edge, so overhangX px are cut and the rest is drawn.
        const int  nCameraX = 100;
        const int  nCameraY = 100;
        const int  nOverhangX = 10;
        const int  nObjectX   = nCameraX - nOverhangX;
        const int  nObjectY   = nCameraY + 2 * kTileSize;   // two tiles below the top edge: inside vertically

        Scene  scene( TileObject( 1, kSecondTileGID, nObjectX, nObjectY ) );

        scene.pRenderer -> SetCameraPosition( nCameraX, nCameraY );
        scene.RunFrames( 1 );

        std :: vector<Event>  tiles = scene.Tiles();

        REQUIRE( tiles.size() == 1 );

        double  fZoom = scene.Zoom();
        CHECK( tiles[0].x == ( float ) kViewportOriginX );
        CHECK( tiles[0].w == ( float ) ( ( kTileSize - nOverhangX ) * fZoom ) );
    }

    TEST_CASE( "A tile object entirely outside the viewport is not drawn" )  {

        const int  nFarAway = 3000;

        Scene  scene( TileObject( 1, kSecondTileGID, nFarAway, nFarAway ) );

        scene.RunFrames( 1 );

        CHECK( scene.Tiles().empty() );
    }

    TEST_CASE( "A resized tile object is stretched into its box, not drawn at the tile's size" )  {

        // A 32 x 32 object of the second tile (16 px): its box is twice the tile.
        const int  nObjectX = 100;
        const int  nObjectY = 50;
        const int  nBoxSize = 2 * kTileSize;

        std :: string  strObject = "<object id=\"1\" gid=\"" + std :: to_string( kSecondTileGID ) + "\" x=\"" +
                                   std :: to_string( nObjectX ) + "\" y=\"" + std :: to_string( nObjectY ) +
                                   "\" width=\"" + std :: to_string( nBoxSize ) + "\" height=\"" +
                                   std :: to_string( nBoxSize ) + "\"/>";

        Scene  scene( strObject );

        scene.RunFrames( 1 );

        // Not a TILE event: the resized object goes through DrawTextureScaled.
        CHECK( scene.Tiles().empty() );
        CHECK( scene.Scaled().size() >= 1 );

        double  fZoom = scene.Zoom();
        CHECK( scene.FirstScaled().x == ( float ) ( nObjectX * fZoom + kViewportOriginX ) );
        CHECK( scene.FirstScaled().y == ( float ) ( ( nObjectY - nBoxSize ) * fZoom + kViewportOriginY ) );
        CHECK( scene.FirstScaled().w == ( float ) ( nBoxSize * fZoom ) );
        CHECK( scene.FirstScaled().h == ( float ) ( nBoxSize * fZoom ) );

        // The whole tile is the source: it starts at the second tile and is one tile in size.
        CHECK( scene.FirstScaled().srcX == ( float ) kTileSize );
        CHECK( scene.FirstScaled().srcW == ( float ) kTileSize );
        CHECK( scene.FirstScaled().srcH == ( float ) kTileSize );
    }

    TEST_CASE( "A resized tile object cut by the left viewport edge keeps only the visible part of the tile" )  {

        // The box is 32 px wide and starts 10 px left of the edge (at x = 100 in the map): 22 px are visible,
        // which is 11 px of the 16 px tile, from its 5th pixel (16 + 5 = 21 in the tileset).
        const int  nCameraX  = 100;
        const int  nCameraY  = 100;
        const int  nBoxSize  = 2 * kTileSize;
        const int  nOverhang = 10;
        const int  nObjectX  = nCameraX - nOverhang;
        const int  nObjectY  = nCameraY + nBoxSize + kTileSize;

        std :: string  strObject = "<object id=\"1\" gid=\"" + std :: to_string( kSecondTileGID ) + "\" x=\"" +
                                   std :: to_string( nObjectX ) + "\" y=\"" + std :: to_string( nObjectY ) +
                                   "\" width=\"" + std :: to_string( nBoxSize ) + "\" height=\"" +
                                   std :: to_string( nBoxSize ) + "\"/>";

        Scene  scene( strObject );

        scene.pRenderer -> SetCameraPosition( nCameraX, nCameraY );
        scene.RunFrames( 1 );

        REQUIRE( scene.Scaled().size() >= 1 );

        double  fZoom    = scene.Zoom();
        double  fVisible = nBoxSize - nOverhang;

        CHECK( scene.FirstScaled().x == ( float ) kViewportOriginX );
        CHECK( scene.FirstScaled().w == ( float ) ( fVisible * fZoom ) );
        CHECK( scene.FirstScaled().srcX == ( float ) ( kTileSize + nOverhang / 2 ) );
        CHECK( scene.FirstScaled().srcW == ( float ) ( ( fVisible / nBoxSize ) * kTileSize ) );
    }

    TEST_CASE( "An animated tile object animates like its tile does on a tile layer" )  {

        // Object of the first tile (gid 1), which is animated: frame 0 is tile 0, frame 1 is tile 1.
        const int  nObjectX = 100;
        const int  nObjectY = 50;

        Scene  scene( TileObject( 1, kFirstTileGID, nObjectX, nObjectY ) );

        scene.RunFrames( 1 );

        REQUIRE( scene.Tiles().size() == 1 );
        CHECK( scene.Tiles()[0].srcX == 0.0f );

        // After one frame's time the animation has moved on to the second tile.
        scene.clockFixture.clock.Advance( kFrameMillis );
        scene.RunFrames( 1 );

        REQUIRE( scene.Tiles().size() == 1 );
        CHECK( scene.Tiles()[0].srcX == ( float ) kTileSize );
    }
}
