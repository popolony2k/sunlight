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
 * I23 of the master plan: an image layer with the Tiled custom property screen_space = true is drawn
 * as every image layer was before L1 - at its offset in window pixels, at its own size, not moved by
 * the camera or the zoom, not cut to the viewport - while keeping its place in the layer order.
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

    const float  __COVER_W  = 200.0f;      // wider and taller than the 100 x 100 viewport
    const float  __COVER_H  = 120.0f;
    const float  __PLAIN_W  = 64.0f;
    const float  __OFF_W    = 48.0f;

    // A 4 x 4 map of 16 px: "ground" (id 1), a screen-space image layer "cover" (id 2: offset 5, 7,
    // opacity 0.5, red tint), a plain image layer "plain" (id 3) and one with screen_space = false (id 4).
    const char  *g_szMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.10\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"5\">"
        "<tileset firstgid=\"1\" name=\"t\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\"><image source=\"t.png\" width=\"16\" height=\"16\"/></tileset>"
        "<layer id=\"1\" name=\"ground\" width=\"4\" height=\"4\"><data encoding=\"csv\">1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1</data></layer>"
        "<imagelayer id=\"2\" name=\"cover\" offsetx=\"5\" offsety=\"7\" opacity=\"0.5\" tintcolor=\"#ff0000\">"
        "<properties><property name=\"screen_space\" type=\"bool\" value=\"true\"/></properties>"
        "<image source=\"cover.png\" width=\"200\" height=\"120\"/></imagelayer>"
        "<imagelayer id=\"3\" name=\"plain\"><image source=\"plain.png\" width=\"64\" height=\"64\"/></imagelayer>"
        "<imagelayer id=\"4\" name=\"off\"><properties><property name=\"screen_space\" type=\"bool\" value=\"false\"/></properties>"
        "<image source=\"off.png\" width=\"48\" height=\"48\"/></imagelayer>"
        "</map>";

    Bytes ToBytes( const std :: string& str )  {
        return Bytes( str.begin(), str.end() );
    }

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {
            fsFixture.fs.files["maps/cover.tmx"] = ToBytes( g_szMap );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = SunLight :: Base :: stDimension2D { { 10, 10 }, { 100, 100 } };

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/cover.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
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

        // Index of the first image draw of a given source width, or -1.
        int IndexOfWidth( float fSourceWidth )  {
            for( int nIdx = 0; nIdx < ( int ) engine().events.size(); nIdx++ )  {
                const Event  &evt = engine().events[nIdx];

                if( ( evt.kind == Event :: TILE ) && ( ( evt.w == fSourceWidth ) || ( evt.w / evt.scale == fSourceWidth ) ) &&
                    ( evt.srcX == 0.0f ) && ( evt.h != 16.0f ) )
                    return nIdx;
            }

            return -1;
        }
    };
}

TEST_SUITE( "renderer/screenspaceimagelayer" )  {

    TEST_CASE( "I23: a screen_space image layer is drawn at its offset in window pixels, its own size, whatever the camera and zoom, not cut to the viewport" )  {

        Scene  scene;

        scene.pRenderer -> SetCameraPosition( 30, 20 );
        scene.pRenderer -> ZoomIn();
        scene.pRenderer -> ZoomIn();

        float  fZoom = scene.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;

        REQUIRE( fZoom != 1.0f );                                          // the camera and zoom really moved

        scene.RunFrames( 1 );

        int  nCover = scene.IndexOfWidth( __COVER_W );

        REQUIRE( nCover > 0 );

        const Event  &cover = scene.engine().events[nCover];

        CHECK( cover.x == 5.0f );                                          // its offset, in window pixels
        CHECK( cover.y == 7.0f );
        CHECK( cover.w == __COVER_W );                                     // its own size
        CHECK( cover.h == __COVER_H );
        CHECK( cover.scale == 1.0f );
        CHECK( cover.color.nRed == 255 );                                  // its tint...
        CHECK( cover.color.nGreen == 0 );
        CHECK( cover.color.nAlpha == 127 );                                // ...and opacity (0.5 of 255)

        // The viewport's clip ends just before it and begins again, the same, just after.
        REQUIRE( nCover + 1 < ( int ) scene.engine().events.size() );
        CHECK( scene.engine().events[nCover - 1].kind == Event :: CLIP_END );
        CHECK( scene.engine().events[nCover + 1].kind == Event :: CLIP_BEGIN );
        CHECK( scene.engine().events[nCover + 1].x == 10.0f );
        CHECK( scene.engine().events[nCover + 1].y == 10.0f );
        CHECK( scene.engine().events[nCover + 1].w == 100.0f );
        CHECK( scene.engine().events[nCover + 1].h == 100.0f );
    }

    TEST_CASE( "I23: image layers without the property, or with it false, keep L1's map-space drawing" )  {

        Scene  scene;

        scene.pRenderer -> SetCameraPosition( 30, 20 );
        scene.pRenderer -> ZoomIn();
        scene.RunFrames( 1 );

        float  fZoom = scene.pRenderer -> GetViewport().GetZoomProperties().fZoomFactor;

        for( float fWidth : { __PLAIN_W, __OFF_W } )  {
            INFO( "image width " << fWidth );

            int  nIdx = scene.IndexOfWidth( fWidth );

            REQUIRE( nIdx > 0 );

            const Event  &img = scene.engine().events[nIdx];

            CHECK( img.scale == fZoom );                                   // scaled by the zoom...
            CHECK( img.x != 0.0f );                                        // ...and moved by the camera and viewport
            CHECK( scene.engine().events[nIdx - 1].kind != Event :: CLIP_END );   // and cut to the viewport
        }
    }

    TEST_CASE( "I23: a screen_space image layer keeps its place in the layer order: a sprite on it draws over it" )  {

        Scene  scene;

        scene.engine().hLoadTextureResult = ( SunLight :: Base :: TextureHandle ) 0x77;
        scene.engine().nLoadTextureWidth  = 16;
        scene.engine().nLoadTextureHeight = 16;

        SunLight :: Canvas :: TextureCanvas  canvas;
        SunLight :: Sprite :: Sprite         sprite;

        REQUIRE( canvas.Load( "ship.png" ) == true );
        canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 20, 20 }, { 16, 16 } } );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.SetActiveTextureSequence( 0 );
        sprite.SetVisible( true );
        REQUIRE( scene.pRenderer -> AddSprite( 2, sprite ) == true );         // on the cover's layer

        scene.RunFrames( 1 );

        int  nCover  = scene.IndexOfWidth( __COVER_W );
        int  nSprite = -1;

        for( int nIdx = 0; nIdx < ( int ) scene.engine().events.size(); nIdx++ )
            if( ( scene.engine().events[nIdx].kind == Event :: TILE ) && ( scene.engine().events[nIdx].handle == ( void * ) 0x77 ) )
                nSprite = nIdx;

        REQUIRE( nCover > 0 );
        REQUIRE( nSprite > 0 );
        CHECK( nCover < nSprite );                                         // the ship over the cover

        scene.pRenderer -> RemoveSprite( 2, sprite );
    }
}
