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
 * M2 of the master plan: AddSprite has always accepted a sprite with no map loaded (the layer
 * id is then a plain grouping key), but RenderMap skipped all layer/sprite drawing without a
 * map, so such a sprite was never drawn. Now a mapless frame draws every registered layer id's
 * sprites, in ascending id order, honouring the active view's layer mask.
 */

#include <doctest/doctest.h>
#include <memory>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef MockEngine :: Event  Event;

namespace  {

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: Base :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // A started renderer that never loads a map.
    struct MaplessScene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        MaplessScene( void )  {

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 10, 10, 400, 300 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
        }

        ~MaplessScene( void )  {
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

        // Texture handles of every TILE draw, in draw order.
        std :: vector<void*>  TileHandles( void )  {
            std :: vector<void*>  handles;

            for( const Event& evt : engine().events )
                if( evt.kind == Event :: TILE )
                    handles.push_back( evt.handle );

            return handles;
        }
    };

    // A one-texture sprite on a caller-chosen layer id, recognised by its own texture handle.
    struct SpriteRig  {
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;
        MaplessScene                         &m_Scene;
        int                                  m_nLayerId;

        SpriteRig( MaplessScene& scene, int nLayerId, void* handle ) : m_Scene( scene ), m_nLayerId( nLayerId )  {

            scene.engine().hLoadTextureResult = handle;
            scene.engine().nLoadTextureWidth  = 16;
            scene.engine().nLoadTextureHeight = 16;

            REQUIRE( canvas.Load( "sprite.png" ) == true );
            canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 20, 20 }, { 16, 16 } } );

            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );

            REQUIRE( scene.pRenderer -> AddSprite( nLayerId, sprite ) == true );
        }

        ~SpriteRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( m_nLayerId, sprite );
        }
    };

    void* const  H1 = ( void* ) 0x2001;
    void* const  H2 = ( void* ) 0x2002;
}

TEST_SUITE( "renderer/mapless_sprites" )  {

    TEST_CASE( "A sprite registered with no map loaded is drawn" )  {

        MaplessScene  scene;
        SpriteRig     rig( scene, 3, H1 );

        scene.RunFrames( 1 );

        CHECK( scene.TileHandles() == std :: vector<void*> { H1 } );
    }

    TEST_CASE( "With no map, layer ids are drawn in ascending order, whatever order they were added in" )  {

        MaplessScene  scene;
        SpriteRig     high( scene, 5, H1 );   // added first, higher id
        SpriteRig     low( scene, 2, H2 );

        scene.RunFrames( 1 );

        CHECK( scene.TileHandles() == std :: vector<void*> { H2, H1 } );
    }

    TEST_CASE( "With no map, a view's layer mask hides that layer id's sprites" )  {

        MaplessScene  scene;
        SpriteRig     shown( scene, 1, H1 );
        SpriteRig     hidden( scene, 2, H2 );

        scene.pRenderer -> GetView( 0 ) -> ShowLayer( 2, false );
        scene.RunFrames( 1 );

        CHECK( scene.TileHandles() == std :: vector<void*> { H1 } );
    }

    TEST_CASE( "With no map and an extra view, each view draws the sprite once and it advances once per frame" )  {

        MaplessScene  scene;
        SpriteRig     rig( scene, 1, H1 );

        REQUIRE( scene.pRenderer -> CreateView( Rect( 500, 10, 400, 300 ) ) != nullptr );
        scene.RunFrames( 1 );

        // Two passes (default view and the extra one), one draw each.
        CHECK( scene.TileHandles() == std :: vector<void*> { H1, H1 } );
    }

    TEST_CASE( "With no map, Stop() releases the registered sprites, as it does with a map" )  {

        // With a map, Stop() -> UnloadMap() -> UnloadSprites() drops every registered sprite.
        // With no map UnloadMap() does nothing, so before this the per-layer sprite lists were
        // never released (a leak LeakSanitizer reported in CI) and the sprites stayed registered
        // across a restart - drawn again after Start() with no new AddSprite.
        MaplessScene  scene;
        SpriteRig     rig( scene, 1, H1 );

        scene.pRenderer -> Stop();
        REQUIRE( scene.pRenderer -> Start() == true );
        scene.RunFrames( 1 );

        CHECK( scene.TileHandles().empty() );
    }

    TEST_CASE( "With no map and no sprite, the frame only clears: no clip is opened" )  {

        MaplessScene  scene;

        scene.RunFrames( 1 );

        int  nClips = 0;

        for( const Event& evt : scene.engine().events )
            if( evt.kind == Event :: CLIP_BEGIN )
                nClips++;

        CHECK( nClips == 0 );
        CHECK( scene.TileHandles().empty() );
    }
}
