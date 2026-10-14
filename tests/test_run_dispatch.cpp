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
 * T3 of the master plan: what Run() dispatches each frame, driven end to end through Run() on
 * the mocks - input handlers (ControllerManager::Update), listeners' OnUpdate, collisions
 * (CollisionManager::Update) - and in what order relative to drawing. ControllerManager and
 * CollisionManager have their own suites; these pin how the frame loop drives them.
 *
 * Found writing it: the map listeners, the collision listeners and the input handlers were
 * walked directly while the callbacks they call may add to (map listeners: also remove from)
 * the very std::deque being walked - whose push_back invalidates every iterator. Pinned at the
 * end. (Collision listeners have no remove call, so they stay registered for the renderer's
 * life; every listener here outlives the frames it is called in.)
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "collision/collider.h"
#include "collision/icollisionlistener.h"
#include "tilemap/itilemaplistener.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_input.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
using SunLight :: Collision :: Collider;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;
namespace SI = SunLight :: Input;

namespace  {

    // A 4x4 map of 16x16 tiles; gid 1 (only at row 1, column 1) carries a full-tile collision shape.
    const char  *g_szCollisionMap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\">"
        "<tileset firstgid=\"1\" name=\"walls\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"1\" columns=\"1\">"
        "<image source=\"walls.png\" width=\"16\" height=\"16\"/>"
        "<tile id=\"0\"><objectgroup><object id=\"1\" x=\"0\" y=\"0\" width=\"16\" height=\"16\"/></objectgroup></tile>"
        "</tileset>"
        "<layer id=\"1\" name=\"walls\" width=\"4\" height=\"4\"><data encoding=\"csv\">0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0</data></layer>"
        "</map>";

    void* const  SPRITE = ( void* ) 0x4001;

    // AddSprite also puts the sprite's own collider on the collider layer of the same id: kept off
    // layers 0 and 1, which the colliding pair below uses, so the sprite never collides with it.
    static constexpr int  SPRITE_LAYER = 5;

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: Base :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // A started renderer on the mocks - input included - that runs a given number of frames at a time.
    struct Scene  {

        MemoryFileSystemFixture             fsFixture;
        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MockInputHandlerFixture             inputFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {

            RendererConfig  config;

            config.fWidth  = 320.0f;
            config.fHeight = 240.0f;

            fsFixture.fs.files["maps/walls.tmx"] = Bytes( g_szCollisionMap, g_szCollisionMap + std :: string( g_szCollisionMap ).size() );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( inputFixture.pLastCreated != nullptr );
            REQUIRE( pRenderer -> Start() == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )       { return engineFixture.engine; }
        MockInputHandler& input( void )  { return *inputFixture.pLastCreated; }

        SunLight :: Collision :: ICollisionManager& collisions( void )  { return pRenderer -> GetCollisionManager(); }

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

    // Counts it's own calls.
    struct Counter  {
        int  nCalls = 0;
        SI :: ControllerType  lastType = SI :: CONTROLLER_NULL;
        int                   nLastId  = -2;

        SI :: INPUT_EVENT_HANDLER Handler( void )  {
            return [this]( SI :: ControllerType type, int nId )  { nCalls++; lastType = type; nLastId = nId; };
        }
    };

    struct UpdateCounter : public SunLight :: TileMap :: ITileMapListener  {
        int  nUpdates = 0;
        int  nStops   = 0;

        void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  { nUpdates++; }
        void OnStop( void ) override                               { nStops++; }
    };

    struct CollisionRecorder : public SunLight :: Collision :: ICollisionListener  {
        std :: vector<std :: pair<Collider*, Collider*>>                  colliderHits;
        std :: vector<std :: pair<Collider*, SunLight :: TileMap :: stTile*>>  tileHits;

        void OnCollision( Collider *pFirst, Collider *pSecond ) override                       { colliderHits.push_back( { pFirst, pSecond } ); }
        void OnCollision( Collider *pFirst, SunLight :: TileMap :: stTile *pSecond ) override  { tileHits.push_back( { pFirst, pSecond } ); }
    };

    // Two colliders on layers 0 and 1, in a collider-to-collider rule, overlapping.
    struct CollidingPair  {
        Collider  first, second;

        CollidingPair( Scene &scene )  {
            first.SetDimension2D( Rect( 0, 0, 50, 50 ) );
            second.SetDimension2D( Rect( 25, 25, 50, 50 ) );

            REQUIRE( scene.collisions().AddCollider( 0, &first ) == true );
            REQUIRE( scene.collisions().AddCollider( 1, &second ) == true );
            REQUIRE( scene.collisions().AddColliderToColliderRule( 0, 1 ) == true );
        }
    };

    // A one-texture sprite on a mapless layer id (SPRITE_LAYER), drawn with it's own handle.
    struct SpriteRig  {
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;
        Scene                                &m_Scene;

        SpriteRig( Scene &scene ) : m_Scene( scene )  {

            scene.engine().hLoadTextureResult = SPRITE;
            scene.engine().nLoadTextureWidth  = 16;
            scene.engine().nLoadTextureHeight = 16;

            REQUIRE( canvas.Load( "sprite.png" ) == true );
            canvas.SetDimension2D( Rect( 20, 20, 16, 16 ) );

            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );

            REQUIRE( scene.pRenderer -> AddSprite( SPRITE_LAYER, sprite ) == true );
        }

        ~SpriteRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( SPRITE_LAYER, sprite );
        }
    };
}

TEST_SUITE( "renderer/Run dispatch" )  {

    TEST_CASE( "A held key fires its handler once per frame; a key that is not held does not" )  {

        Scene    scene;
        Counter  held, idle;

        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_RIGHT, held.Handler() );
        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_LEFT, idle.Handler() );
        scene.input().downKeys.insert( SI :: KEY_RIGHT );

        scene.RunFrames( 3 );

        CHECK( held.nCalls == 3 );
        CHECK( held.lastType == SI :: CONTROLLER_KEYBOARD );
        CHECK( idle.nCalls == 0 );

        scene.input().downKeys.clear();
        scene.RunFrames( 2 );

        CHECK( held.nCalls == 3 );
    }

    TEST_CASE( "The no-input handler fires only on frames where nothing else did" )  {

        Scene    scene;
        Counter  key, none;

        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_SPACE, key.Handler() );
        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_NULL, none.Handler() );

        scene.RunFrames( 2 );
        CHECK( none.nCalls == 2 );
        CHECK( none.lastType == SI :: CONTROLLER_NULL );

        scene.input().downKeys.insert( SI :: KEY_SPACE );
        scene.RunFrames( 3 );
        CHECK( key.nCalls == 3 );
        CHECK( none.nCalls == 2 );
    }

    TEST_CASE( "A gamepad button fires its handler once per frame for an added pad, never for one that was not added" )  {

        Scene    scene;
        Counter  button;

        scene.pRenderer -> SetUserGamePadEventHandler( SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN, button.Handler() );
        scene.input().downButtons.insert( SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN );

        scene.RunFrames( 2 );
        CHECK( button.nCalls == 0 );                           // no pad added yet

        scene.pRenderer -> AddGamePad( 0 );
        scene.RunFrames( 3 );
        CHECK( button.nCalls == 3 );
        CHECK( button.lastType == SI :: CONTROLLER_GAMEPAD );
        CHECK( button.nLastId == 0 );
    }

    TEST_CASE( "Overlapping colliders in a rule collide once per frame; moved apart, they stop" )  {

        Scene              scene;
        CollidingPair      pair( scene );
        CollisionRecorder  recorder;

        scene.collisions().AddCollisionListener( &recorder );

        scene.RunFrames( 3 );

        REQUIRE( recorder.colliderHits.size() == 3 );
        CHECK( recorder.colliderHits[0].first == &pair.first );
        CHECK( recorder.colliderHits[0].second == &pair.second );

        pair.second.SetDimension2D( Rect( 200, 200, 10, 10 ) );
        scene.RunFrames( 2 );

        CHECK( recorder.colliderHits.size() == 3 );

    }

    TEST_CASE( "A collider over a map tile with a collision shape, in a collider-to-tile rule, collides with that tile" )  {

        Scene              scene;
        Collider           collider;
        CollisionRecorder  recorder;

        REQUIRE( scene.pRenderer -> LoadMap( "maps/walls.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

        collider.SetDimension2D( Rect( 2, 2, 8, 8 ) );              // over row 0, column 0: no tile
        REQUIRE( scene.collisions().AddCollider( 0, &collider ) == true );
        REQUIRE( scene.collisions().AddColliderToTileRule( 0, 1 ) == true );
        scene.collisions().AddCollisionListener( &recorder );

        scene.RunFrames( 2 );
        CHECK( recorder.tileHits.empty() );

        collider.SetDimension2D( Rect( 20, 20, 8, 8 ) );            // over row 1, column 1: the wall
        scene.RunFrames( 3 );

        REQUIRE( recorder.tileHits.size() == 3 );
        CHECK( recorder.tileHits[0].first == &collider );
        CHECK( recorder.colliderHits.empty() );

        scene.collisions().RemoveCollider( 0, &collider );
    }

    TEST_CASE( "Each frame draws, then dispatches input, then OnUpdate, then collisions, then the fade" )  {

        // Each callback records how many engine calls the frame had made when it ran.
        struct OrderListener : public SunLight :: TileMap :: ITileMapListener, public SunLight :: Collision :: ICollisionListener  {
            Scene                       &scene;
            std :: vector<std :: string>  order;
            size_t                       nAtUpdate = 0, nAtCollision = 0;

            OrderListener( Scene &s ) : scene( s )  {}

            void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  { order.push_back( "update" ); nAtUpdate = scene.engine().events.size(); }
            void OnStop( void ) override  {}
            void OnCollision( Collider*, Collider* ) override  { order.push_back( "collision" ); nAtCollision = scene.engine().events.size(); }
            void OnCollision( Collider*, SunLight :: TileMap :: stTile* ) override  {}
        };

        Scene          scene;
        SpriteRig      sprite( scene );
        CollidingPair  pair( scene );
        OrderListener  listener( scene );
        size_t         nAtKey = 0;

        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_UP, [&]( SI :: ControllerType, int )  {
            listener.order.push_back( "key" );
            nAtKey = scene.engine().events.size();
        } );
        scene.input().downKeys.insert( SI :: KEY_UP );
        scene.pRenderer -> AddTileMapListener( &listener );
        scene.collisions().AddCollisionListener( &listener );
        scene.pRenderer -> SetScreenFade( 0.5f );

        scene.RunFrames( 1 );

        REQUIRE( listener.order == std :: vector<std :: string> { "key", "update", "collision" } );

        size_t  nSpriteDraw = 0, nFade = 0;

        for( size_t nEvent = 0; nEvent < scene.engine().events.size(); nEvent++ )  {
            const Event  &evt = scene.engine().events[nEvent];

            if( ( evt.kind == Event :: TILE ) && ( evt.handle == SPRITE ) )
                nSpriteDraw = nEvent + 1;
            if( evt.kind == Event :: FILL )
                nFade = nEvent + 1;
        }

        CHECK( nSpriteDraw > 0 );
        CHECK( nSpriteDraw <= nAtKey );                         // drawn before input ran...
        CHECK( nAtKey <= listener.nAtUpdate );
        CHECK( listener.nAtUpdate <= listener.nAtCollision );
        CHECK( nFade > listener.nAtCollision );                 // ...and the fade after everything

        scene.pRenderer -> RemoveTileMapListener( &listener );
    }

    TEST_CASE( "An invisible renderer still presents and counts frames, but draws and dispatches nothing" )  {

        Scene              scene;
        SpriteRig          sprite( scene );
        CollidingPair      pair( scene );
        UpdateCounter      updates;
        CollisionRecorder  recorder;
        Counter            key;

        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_UP, key.Handler() );
        scene.input().downKeys.insert( SI :: KEY_UP );
        scene.pRenderer -> AddTileMapListener( &updates );
        scene.collisions().AddCollisionListener( &recorder );

        scene.pRenderer -> SetVisible( false );
        scene.RunFrames( 3 );

        CHECK( scene.windowFixture.window.nEndFrameCalls == 3 );
        CHECK( scene.DrawsOf( SPRITE ) == 0 );
        CHECK( key.nCalls == 0 );
        CHECK( updates.nUpdates == 0 );
        CHECK( recorder.colliderHits.empty() );

        scene.pRenderer -> SetVisible( true );
        scene.RunFrames( 1 );

        CHECK( scene.DrawsOf( SPRITE ) == 1 );
        CHECK( key.nCalls == 1 );
        CHECK( updates.nUpdates == 1 );
        CHECK( recorder.colliderHits.size() == 1 );

        scene.pRenderer -> RemoveTileMapListener( &updates );
    }

    TEST_CASE( "A collision callback that removes the collider it hit gets one event, and the run goes on" )  {

        struct Despawner : public SunLight :: Collision :: ICollisionListener  {
            SunLight :: Collision :: ICollisionManager  &manager;
            int                                         nHits = 0;

            Despawner( SunLight :: Collision :: ICollisionManager &m ) : manager( m )  {}

            void OnCollision( Collider*, Collider *pSecond ) override  { nHits++; manager.RemoveCollider( 1, pSecond ); }
            void OnCollision( Collider*, SunLight :: TileMap :: stTile* ) override  {}
        };

        Scene          scene;
        CollidingPair  pair( scene );
        Despawner      despawner( scene.collisions() );

        scene.collisions().AddCollisionListener( &despawner );
        scene.RunFrames( 3 );

        CHECK( despawner.nHits == 1 );
        CHECK( scene.windowFixture.window.nEndFrameCalls == 3 );

    }

    // ---- A callback that changes the list it is being called from ----

    TEST_CASE( "A listener removed by another one earlier in the same frame is not called, from then on" )  {

        struct Remover : public SunLight :: TileMap :: ITileMapListener  {
            TileMapRenderer                        &renderer;
            SunLight :: TileMap :: ITileMapListener *pVictim;

            int                                    nUpdates = 0;

            Remover( TileMapRenderer &r, SunLight :: TileMap :: ITileMapListener *pV ) : renderer( r ), pVictim( pV )  {}

            void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  { nUpdates++; renderer.RemoveTileMapListener( pVictim ); }
            void OnStop( void ) override  {}
        };

        Scene          scene;
        UpdateCounter  victim, bystander;
        Remover        remover( *scene.pRenderer, &victim );

        scene.pRenderer -> AddTileMapListener( &remover );
        scene.pRenderer -> AddTileMapListener( &victim );
        scene.pRenderer -> AddTileMapListener( &bystander );

        scene.RunFrames( 3 );

        // Every other listener exactly once per frame. With the old range-for, erasing the victim
        // shifted the deque under the loop's iterator: the remover itself was called twice a frame.
        CHECK( remover.nUpdates == 3 );
        CHECK( victim.nUpdates == 0 );
        CHECK( bystander.nUpdates == 3 );

        scene.pRenderer -> RemoveTileMapListener( &remover );
        scene.pRenderer -> RemoveTileMapListener( &bystander );
    }

    TEST_CASE( "Listeners added from OnUpdate start on the next frame - many of them, past the list's first block" )  {

        // Enough to grow the deque's block map while it is being walked: with the old range-for,
        // that left the loop's iterator pointing into freed memory (AddressSanitizer reports it).
        static constexpr int  ADDED = 4096;

        struct Spawner : public SunLight :: TileMap :: ITileMapListener  {
            TileMapRenderer                  &renderer;
            std :: vector<UpdateCounter>     &spawned;
            bool                             bDone = false;

            Spawner( TileMapRenderer &r, std :: vector<UpdateCounter> &s ) : renderer( r ), spawned( s )  {}

            void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  {
                if( !bDone )
                    for( UpdateCounter &counter : spawned )
                        renderer.AddTileMapListener( &counter );
                bDone = true;
            }
            void OnStop( void ) override  {}
        };

        Scene                          scene;
        std :: vector<UpdateCounter>   spawned( ADDED );
        Spawner                        spawner( *scene.pRenderer, spawned );

        scene.pRenderer -> AddTileMapListener( &spawner );

        scene.RunFrames( 1 );
        CHECK( spawned.front().nUpdates == 0 );
        CHECK( spawned.back().nUpdates == 0 );

        scene.RunFrames( 1 );
        CHECK( spawned.front().nUpdates == 1 );
        CHECK( spawned.back().nUpdates == 1 );

        for( UpdateCounter &counter : spawned )
            scene.pRenderer -> RemoveTileMapListener( &counter );
        scene.pRenderer -> RemoveTileMapListener( &spawner );
    }

    TEST_CASE( "A listener removed by another one from OnStop is not stopped" )  {

        struct StopRemover : public SunLight :: TileMap :: ITileMapListener  {
            TileMapRenderer                        &renderer;
            SunLight :: TileMap :: ITileMapListener *pVictim;

            StopRemover( TileMapRenderer &r, SunLight :: TileMap :: ITileMapListener *pV ) : renderer( r ), pVictim( pV )  {}

            void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  {}
            void OnStop( void ) override  { renderer.RemoveTileMapListener( pVictim ); }
        };

        Scene          scene;
        UpdateCounter  victim;
        StopRemover    remover( *scene.pRenderer, &victim );

        scene.pRenderer -> AddTileMapListener( &remover );
        scene.pRenderer -> AddTileMapListener( &victim );

        scene.pRenderer -> Stop();

        CHECK( victim.nStops == 0 );

        scene.pRenderer -> RemoveTileMapListener( &remover );
    }

    TEST_CASE( "Collision listeners added from OnCollision start with the next collision" )  {

        static constexpr int  ADDED = 4096;

        struct Spawner : public SunLight :: Collision :: ICollisionListener  {
            SunLight :: Collision :: ICollisionManager  &manager;
            std :: vector<CollisionRecorder>            &spawned;
            bool                                        bDone = false;

            Spawner( SunLight :: Collision :: ICollisionManager &m, std :: vector<CollisionRecorder> &s ) : manager( m ), spawned( s )  {}

            void OnCollision( Collider*, Collider* ) override  {
                if( !bDone )
                    for( CollisionRecorder &recorder : spawned )
                        manager.AddCollisionListener( &recorder );
                bDone = true;
            }
            void OnCollision( Collider*, SunLight :: TileMap :: stTile* ) override  {}
        };

        Scene                              scene;
        CollidingPair                      pair( scene );
        std :: vector<CollisionRecorder>   spawned( ADDED );
        Spawner                            spawner( scene.collisions(), spawned );

        scene.collisions().AddCollisionListener( &spawner );

        scene.RunFrames( 1 );
        CHECK( spawned.front().colliderHits.empty() );
        CHECK( spawned.back().colliderHits.empty() );

        scene.RunFrames( 1 );
        CHECK( spawned.front().colliderHits.size() == 1 );
        CHECK( spawned.back().colliderHits.size() == 1 );
    }

    TEST_CASE( "Key handlers added from a key handler are dispatched from the next frame on" )  {

        static constexpr int  ADDED = 4096;

        Scene                   scene;
        std :: vector<Counter>  added( ADDED );
        bool                    bDone = false;

        scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_ENTER, [&]( SI :: ControllerType, int )  {
            if( !bDone )
                for( Counter &counter : added )
                    scene.pRenderer -> SetUserKeyEventHandler( SI :: KEY_ENTER, counter.Handler() );
            bDone = true;
        } );
        scene.input().downKeys.insert( SI :: KEY_ENTER );

        scene.RunFrames( 1 );
        CHECK( added.front().nCalls == 0 );
        CHECK( added.back().nCalls == 0 );

        scene.RunFrames( 1 );
        CHECK( added.front().nCalls == 1 );
        CHECK( added.back().nCalls == 1 );
    }

    TEST_CASE( "Gamepad handlers and pads added from a gamepad handler are dispatched from the next frame on" )  {

        Scene    scene;
        Counter  added;
        int      nCalls = 0;

        scene.pRenderer -> AddGamePad( 0 );
        scene.pRenderer -> SetUserGamePadEventHandler( SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN, [&]( SI :: ControllerType, int )  {
            if( nCalls++ == 0 )  {
                scene.pRenderer -> SetUserGamePadEventHandler( SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN, added.Handler() );
                scene.pRenderer -> AddGamePad( 1 );
            }
        } );
        scene.input().downButtons.insert( SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN );

        scene.RunFrames( 1 );
        CHECK( nCalls == 1 );                                  // pad 0 only
        CHECK( added.nCalls == 0 );

        scene.RunFrames( 1 );
        CHECK( nCalls == 3 );                                  // pads 0 and 1
        CHECK( added.nCalls == 2 );
    }
}
