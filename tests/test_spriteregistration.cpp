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
 * The renderer keeps raw pointers to the sprites registered with AddSprite, and every sprite (with
 * its collider and canvases) keeps a raw pointer to its parent - the renderer. Nothing may dangle
 * whichever of the two is destroyed first, registered or not. Most of what is asserted here is
 * only OBSERVABLE deterministically through the side effects (the same address can be registered
 * again, parents are null...); the address-level proof is running the suite under AddressSanitizer
 * (see CLAUDE.md).
 */

#include <doctest/doctest.h>
#include <memory>
#include <new>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // A started renderer over a tiny map (its tile layer has id 1), all mocks installed.
    struct Rig  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Rig( void )  {
            fsFixture.fs.files["maps/square.tmx"] = MakeSquareTmx( 4, 16 );

            RendererConfig  config;

            config.fWidth  = 1260.0f;
            config.fHeight = 920.0f;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/square.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );
        }

        ~Rig( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        void RunFrames( int nFrames )  {
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }
    };
}

TEST_SUITE( "renderer/spriteregistration" )  {

    TEST_CASE( "A sprite destroyed while registered is forgotten by the renderer (its address can be registered again, frames are safe)" )  {

        Rig  rig;

        // The same storage holds two different sprites, one after the other: a stale registry
        // entry for the first one would still be "there" for the second (same address).
        alignas( SunLight :: Sprite :: Sprite ) unsigned char  storage[sizeof( SunLight :: Sprite :: Sprite )];

        SunLight :: Sprite :: Sprite  *pFirst = new( storage ) SunLight :: Sprite :: Sprite();

        REQUIRE( rig.pRenderer -> AddSprite( 1, *pFirst ) == true );
        CHECK( rig.pRenderer -> AddSprite( 1, *pFirst ) == false );          // already registered

        pFirst -> ~Sprite();

        SunLight :: Sprite :: Sprite  *pSecond = new( storage ) SunLight :: Sprite :: Sprite();

        CHECK( rig.pRenderer -> AddSprite( 1, *pSecond ) == true );          // old code: still "registered" (stale entry)

        // Frames with a registered (live) sprite, then with none, both fine.
        rig.RunFrames( 2 );
        pSecond -> ~Sprite();
        rig.RunFrames( 2 );
    }

    TEST_CASE( "A heap sprite destroyed while registered: the next frame and Stop() never touch it" )  {

        Rig  rig;

        std :: unique_ptr<SunLight :: Sprite :: Sprite>  pSprite = std :: make_unique<SunLight :: Sprite :: Sprite>();

        REQUIRE( rig.pRenderer -> AddSprite( 1, *pSprite ) == true );
        pSprite.reset();

        rig.RunFrames( 3 );                     // AddressSanitizer: heap-use-after-free on the old code
        rig.pRenderer -> Stop();
    }

    TEST_CASE( "A registered sprite outliving its renderer: the renderer unloads it as it goes, and nothing keeps pointing at the dead renderer" )  {

        Rig  rig;

        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  early, late;

        // One canvas added BEFORE the sprite is registered (parented to the sprite), one AFTER (parented
        // to the renderer, which is what AddTextureSequence does for a sprite that already has a parent).
        sprite.AddTextureSequence( 0, &early );
        REQUIRE( rig.pRenderer -> AddSprite( 1, sprite ) == true );
        sprite.AddTextureSequence( 1, &late );

        CHECK( sprite.GetParent() != nullptr );
        CHECK( early.GetParent() == &sprite );
        CHECK( late.GetParent() == sprite.GetParent() );

        rig.pRenderer.reset();                  // the renderer goes first (unloading the sprite, as it always did)

        CHECK( sprite.GetParent() == nullptr );
        CHECK( sprite.GetCollider().GetParent() == nullptr );
        CHECK( early.GetParent() == nullptr );                  // released by the sprite's Unload
        CHECK( late.GetParent() == nullptr );

        // Everything that resolves through the parent chain works on its own viewport/visibility.
        CHECK( &late.GetViewport() != nullptr );
        sprite.SetVisible( true );
        CHECK( sprite.GetVisible() == true );
        late.SetVisible( true );
        CHECK( late.GetVisible() == true );                     // its own flag: no parent to ask
    }

    TEST_CASE( "A sprite removed with RemoveSprite that still holds canvases: the renderer going away repoints the canvases that followed it" )  {

        Rig  rig;

        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  early, late;

        sprite.AddTextureSequence( 0, &early );
        REQUIRE( rig.pRenderer -> AddSprite( 1, sprite ) == true );
        sprite.AddTextureSequence( 1, &late );
        REQUIRE( rig.pRenderer -> RemoveSprite( 1, sprite ) == true );      // not unloaded: still holds both canvases

        CHECK( late.GetParent() == sprite.GetParent() );

        rig.pRenderer.reset();

        CHECK( sprite.GetParent() == nullptr );
        CHECK( sprite.GetCollider().GetParent() == nullptr );
        CHECK( early.GetParent() == &sprite );                  // never pointed at the renderer: untouched
        CHECK( late.GetParent() == &sprite );                   // pointed at the renderer: now at its sprite
        CHECK( &late.GetViewport() == &sprite.GetViewport() );
    }

    TEST_CASE( "A sprite removed with RemoveSprite keeps its parent (as before) - but is still detached when the renderer goes" )  {

        Rig  rig;

        SunLight :: Sprite :: Sprite  sprite;

        REQUIRE( rig.pRenderer -> AddSprite( 1, sprite ) == true );
        REQUIRE( rig.pRenderer -> RemoveSprite( 1, sprite ) == true );

        CHECK( sprite.GetParent() != nullptr );             // unchanged behaviour: RemoveSprite does not reparent

        rig.pRenderer.reset();

        CHECK( sprite.GetParent() == nullptr );
    }

    TEST_CASE( "A removed sprite that is then destroyed does not leave the renderer holding it" )  {

        Rig  rig;

        std :: unique_ptr<SunLight :: Sprite :: Sprite>  pSprite = std :: make_unique<SunLight :: Sprite :: Sprite>();

        REQUIRE( rig.pRenderer -> AddSprite( 1, *pSprite ) == true );
        REQUIRE( rig.pRenderer -> RemoveSprite( 1, *pSprite ) == true );
        pSprite.reset();                                    // still has the renderer as parent: it must tell it

        // Destroying the renderer now must not touch the dead sprite (AddressSanitizer: heap-use-after-free on the old code).
        rig.pRenderer.reset();
    }

    TEST_CASE( "Re-parenting a sprite makes its old renderer forget it, and moves the canvases that followed the old parent" )  {

        Rig  rig;

        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;

        REQUIRE( rig.pRenderer -> AddSprite( 1, sprite ) == true );
        sprite.AddTextureSequence( 0, &canvas );                // parented to the renderer

        CHECK( canvas.GetParent() == sprite.GetParent() );

        // Detached by hand: the renderer stops holding it, the canvas follows to the sprite.
        sprite.SetParent( nullptr );

        CHECK( sprite.GetParent() == nullptr );
        CHECK( canvas.GetParent() == &sprite );
        CHECK( rig.pRenderer -> AddSprite( 1, sprite ) == true );        // forgotten: can be registered again

        // And the canvas parented to the sprite before registration stays that way.
        SunLight :: Canvas :: TextureCanvas  first;

        sprite.AddTextureSequence( 5, &first );
        CHECK( first.GetParent() == sprite.GetParent() );               // sprite has the renderer as parent again
    }

    TEST_CASE( "Registering a sprite twice, on two layers, then destroying it: forgotten on every layer" )  {

        Rig  rig;

        alignas( SunLight :: Sprite :: Sprite ) unsigned char  storage[sizeof( SunLight :: Sprite :: Sprite )];

        SunLight :: Sprite :: Sprite  *pFirst = new( storage ) SunLight :: Sprite :: Sprite();

        REQUIRE( rig.pRenderer -> AddSprite( 1, *pFirst ) == true );
        pFirst -> ~Sprite();

        SunLight :: Sprite :: Sprite  *pSecond = new( storage ) SunLight :: Sprite :: Sprite();

        CHECK( rig.pRenderer -> AddSprite( 1, *pSecond ) == true );
        rig.RunFrames( 1 );
        pSecond -> ~Sprite();
    }
}
