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
 * I25 of the master plan: gamepads read through SDL2 - the shared device list (SDLGamepadDevices),
 * the pads' state on raylib's scale (SDLGamepadInput), rumble on the same devices
 * (SDLGamepadHaptics), the factory's choice and fallback, and the renderer's per-frame
 * IInputHandler::Update. Everything on the fake SDL API: no real device is ever opened.
 */

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <vector>
#include "backends/sdl/sdlgamepaddevices.h"
#include "backends/sdl/sdlgamepadinput.h"
#include "backends/sdl/sdlgamepadhaptics.h"
#include "backends/raylib/raylibgamepadinput.h"
#include "input/gamepadinputfactory.h"
#include "renderer/tilemaprenderer.h"
#include "fake_sdlcontrollerapi.h"
#include "mock_clock.h"
#include "mock_engine.h"
#include "mock_input.h"
#include "mock_window.h"

using SunLight :: Input :: SDL :: ISDLControllerApi;
using SunLight :: Input :: SDL :: SDLGamepadDevices;
using SunLight :: Input :: SDL :: SDLGamepadInput;
using SunLight :: Input :: SDL :: SDLGamepadHaptics;
using FakeSDL :: FakeLog;
using FakeSDL :: FakeDevice;
using FakeSDL :: FakeControllerApi;
namespace SI = SunLight :: Input;

namespace  {

    const int16_t  AXIS_MAX = 32767;
    const int16_t  AXIS_MIN = -32768;

    // A device list on the fake API, with the fake kept reachable.
    struct Devices  {
        FakeLog                               log;
        FakeControllerApi                     *pApi = nullptr;      // owned by pDevices
        std :: shared_ptr<SDLGamepadDevices>  pDevices;

        Devices( void )  {
            std :: unique_ptr<FakeControllerApi>  pFake = std :: make_unique<FakeControllerApi>( log );

            pApi     = pFake.get();
            pDevices = std :: make_shared<SDLGamepadDevices>( std :: move( pFake ) );
        }

        void Plug( const std :: string& strName, int32_t nInstanceId )  {
            pApi -> devices.push_back( FakeDevice { strName, nInstanceId, true, true } );
        }

        // Unplugged: SDL reports the open controller detached and drops it from its list.
        void Unplug( int32_t nInstanceId )  {
            for( size_t nPos = 0; nPos < pApi -> devices.size(); nPos++ )  {
                if( pApi -> devices[ nPos ].nInstanceId == nInstanceId )  {
                    pApi -> devices.erase( pApi -> devices.begin() + nPos );
                    return;
                }
            }
        }
    };

    // One pad (instance 7) read through SDLGamepadInput.
    struct OnePad : public Devices  {
        std :: unique_ptr<SDLGamepadInput>  pInput;

        OnePad( void )  {
            Plug( "Pro Controller", 7 );
            pInput = std :: make_unique<SDLGamepadInput>( pDevices );
        }

        FakeDevice& Pad( void )  { return pApi -> Device( 7 ); }
    };
}

TEST_SUITE( "backends/sdl/SDLGamepadDevices" )  {

    TEST_CASE( "Pads take the lowest free id and keep it until unplugged; the next pad gets the lowest free one again" )  {

        Devices  rig;

        rig.Plug( "A", 10 );
        rig.Plug( "B", 11 );
        rig.Plug( "C", 12 );
        rig.pDevices -> Update();

        CHECK( std :: string( rig.pDevices -> Name( 0 ) ) == "A" );
        CHECK( std :: string( rig.pDevices -> Name( 1 ) ) == "B" );
        CHECK( std :: string( rig.pDevices -> Name( 2 ) ) == "C" );
        CHECK( rig.log.nOpens == 3 );

        rig.Unplug( 11 );
        CHECK( rig.pDevices -> IsAvailable( 1 ) );         // nothing changes before Update()
        rig.pDevices -> Update();
        CHECK( rig.pDevices -> IsAvailable( 1 ) == false );
        CHECK( rig.pDevices -> IsAvailable( 2 ) );            // C keeps its id
        CHECK( rig.log.nCloses == 1 );

        rig.Plug( "D", 13 );
        rig.pDevices -> Update();
        CHECK( std :: string( rig.pDevices -> Name( 1 ) ) == "D" );
        CHECK( std :: string( rig.pDevices -> Name( 2 ) ) == "C" );
        CHECK( rig.log.nOpens == 4 );                         // A and C were not reopened
    }

    TEST_CASE( "Each Update updates SDL once; a set whose SDL did not start does nothing at all" )  {

        Devices  rig;

        rig.Plug( "A", 10 );
        rig.pDevices -> Update();
        rig.pDevices -> Update();
        CHECK( rig.pApi -> nUpdates == 2 );
        CHECK( rig.log.nOpens == 1 );                         // opened once, on the frame it appeared

        Devices  notReady;

        notReady.pApi -> bReady = false;
        notReady.Plug( "A", 10 );
        notReady.pDevices -> Update();
        CHECK( notReady.pDevices -> IsReady() == false );
        CHECK( notReady.pApi -> nUpdates == 0 );
        CHECK( notReady.log.nOpens == 0 );
        CHECK( notReady.pDevices -> IsAvailable( 0 ) == false );
    }

    TEST_CASE( "A device that is not a game controller is skipped; one that fails to open is not retried until it is plugged in again" )  {

        Devices  rig;

        rig.Plug( "Joystick", 10 );
        rig.pApi -> devices[0].bIsGameController = false;
        rig.Plug( "Pad", 11 );
        rig.pDevices -> Update();

        CHECK( std :: string( rig.pDevices -> Name( 0 ) ) == "Pad" );
        CHECK( rig.pDevices -> IsAvailable( 1 ) == false );

        Devices  failing;

        failing.Plug( "Pad", 10 );
        failing.pApi -> bOpenFails = true;
        failing.pDevices -> Update();
        failing.pApi -> bOpenFails = false;
        failing.pDevices -> Update();
        CHECK( failing.pDevices -> IsAvailable( 0 ) == false );      // not retried while listed

        failing.Unplug( 10 );
        failing.pDevices -> Update();
        failing.Plug( "Pad", 10 );
        failing.pDevices -> Update();
        CHECK( failing.pDevices -> IsAvailable( 0 ) );               // plugged in again: opened
    }

    TEST_CASE( "At most MAX_GAMEPAD_COUNT pads are open; out-of-range ids have nothing; the destructor closes every pad" )  {

        Devices  rig;

        for( int32_t nPad = 0; nPad <= MAX_GAMEPAD_COUNT; nPad++ )
            rig.Plug( "Pad", 100 + nPad );

        rig.pDevices -> Update();
        CHECK( rig.log.nOpens == MAX_GAMEPAD_COUNT );
        CHECK( rig.pDevices -> IsAvailable( MAX_GAMEPAD_COUNT - 1 ) );
        CHECK( rig.pDevices -> IsAvailable( MAX_GAMEPAD_COUNT ) == false );
        CHECK( rig.pDevices -> IsAvailable( -1 ) == false );
        CHECK( std :: string( rig.pDevices -> Name( -1 ) ).empty() );

        rig.pDevices.reset();
        CHECK( rig.log.nCloses == MAX_GAMEPAD_COUNT );
    }
}

TEST_SUITE( "backends/sdl/SDLGamepadInput" )  {

    TEST_CASE( "A pad already plugged in is there from construction; with no pad every query answers raylib's neutral value" )  {

        OnePad  rig;

        CHECK( rig.pInput -> IsGamepadAvailable( 0 ) );
        CHECK( std :: string( rig.pInput -> GetGamepadName( 0 ) ) == "Pro Controller" );
        CHECK( rig.pInput -> GetGamepadAxisCount( 0 ) == 6 );

        CHECK( rig.pInput -> IsGamepadAvailable( 1 ) == false );
        CHECK( std :: string( rig.pInput -> GetGamepadName( 1 ) ).empty() );
        CHECK( rig.pInput -> GetGamepadAxisCount( 1 ) == 0 );
        CHECK( rig.pInput -> IsGamepadButtonDown( 1, SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == false );
        CHECK( rig.pInput -> IsGamepadButtonUp( 1, SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == false );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 1, SI :: GAMEPAD_AXIS_LEFT_X ) == 0.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 1, SI :: GAMEPAD_AXIS_LEFT_TRIGGER ) == -1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 1, SI :: GAMEPAD_AXIS_RIGHT_TRIGGER ) == -1.0f );
    }

    TEST_CASE( "Every SDL button is the sunlight button raylib maps GLFW's same button to" )  {

        struct stCase  {
            ISDLControllerApi :: Button  sdl;
            SI :: GamepadButton          ours;
        };

        const stCase  cases[] = {
            { ISDLControllerApi :: BUTTON_A,             SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN },
            { ISDLControllerApi :: BUTTON_B,             SI :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT },
            { ISDLControllerApi :: BUTTON_X,             SI :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT },
            { ISDLControllerApi :: BUTTON_Y,             SI :: GAMEPAD_BUTTON_RIGHT_FACE_UP },
            { ISDLControllerApi :: BUTTON_BACK,          SI :: GAMEPAD_BUTTON_MIDDLE_LEFT },
            { ISDLControllerApi :: BUTTON_GUIDE,         SI :: GAMEPAD_BUTTON_MIDDLE },
            { ISDLControllerApi :: BUTTON_START,         SI :: GAMEPAD_BUTTON_MIDDLE_RIGHT },
            { ISDLControllerApi :: BUTTON_LEFTSTICK,     SI :: GAMEPAD_BUTTON_LEFT_THUMB },
            { ISDLControllerApi :: BUTTON_RIGHTSTICK,    SI :: GAMEPAD_BUTTON_RIGHT_THUMB },
            { ISDLControllerApi :: BUTTON_LEFTSHOULDER,  SI :: GAMEPAD_BUTTON_LEFT_TRIGGER_1 },
            { ISDLControllerApi :: BUTTON_RIGHTSHOULDER, SI :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1 },
            { ISDLControllerApi :: BUTTON_DPAD_UP,       SI :: GAMEPAD_BUTTON_LEFT_FACE_UP },
            { ISDLControllerApi :: BUTTON_DPAD_DOWN,     SI :: GAMEPAD_BUTTON_LEFT_FACE_DOWN },
            { ISDLControllerApi :: BUTTON_DPAD_LEFT,     SI :: GAMEPAD_BUTTON_LEFT_FACE_LEFT },
            { ISDLControllerApi :: BUTTON_DPAD_RIGHT,    SI :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT },
        };

        OnePad  rig;

        for( const stCase& c : cases )  {
            CAPTURE( ( int ) c.sdl );
            rig.Pad().buttons = { c.sdl };
            rig.pInput -> Update();

            for( int nButton = SI :: GAMEPAD_BUTTON_UNKNOWN; nButton <= SI :: GAMEPAD_BUTTON_RIGHT_THUMB; nButton++ )  {
                bool  bExpected = ( nButton == c.ours );

                CAPTURE( nButton );
                CHECK( rig.pInput -> IsGamepadButtonDown( 0, ( SI :: GamepadButton ) nButton ) == bExpected );
                CHECK( rig.pInput -> IsGamepadButtonUp( 0, ( SI :: GamepadButton ) nButton ) == !bExpected );
            }
        }
    }

    TEST_CASE( "Sticks read -1..1 and triggers -1 (released) .. 1 (pulled), as GLFW reports them" )  {

        OnePad  rig;

        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_X ) == 0.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_TRIGGER ) == -1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_RIGHT_TRIGGER ) == -1.0f );

        rig.Pad().axes = { AXIS_MAX, AXIS_MIN, AXIS_MIN, AXIS_MAX, AXIS_MAX, AXIS_MAX };
        rig.pInput -> Update();

        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_X ) == 1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_Y ) == -1.0f );       // one step past -1, clamped
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_RIGHT_X ) == -1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_RIGHT_Y ) == 1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_TRIGGER ) == 1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_RIGHT_TRIGGER ) == 1.0f );

        // Each axis reads its own value: none crossed with another.
        rig.Pad().axes = { 0, AXIS_MAX, 0, 0, 0, AXIS_MAX };
        rig.pInput -> Update();
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_X ) == 0.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_Y ) == 1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_LEFT_TRIGGER ) == -1.0f );
        CHECK( rig.pInput -> GetGamepadAxisMovement( 0, SI :: GAMEPAD_AXIS_RIGHT_TRIGGER ) == 1.0f );
    }

    TEST_CASE( "The second triggers are pressed past 0.1 on the trigger's -1..1 scale, as raylib makes them" )  {

        // 0.1 on the -1..1 scale is 0.55 of SDL's 0..32767: 18021.85. 18021 is just under, 18022 just over.
        const int16_t  UNDER = 18021;
        const int16_t  OVER  = 18022;

        OnePad  rig;

        rig.Pad().axes[ ISDLControllerApi :: AXIS_TRIGGERLEFT ]  = UNDER;
        rig.Pad().axes[ ISDLControllerApi :: AXIS_TRIGGERRIGHT ] = OVER;
        rig.pInput -> Update();

        CHECK( rig.pInput -> IsGamepadButtonDown( 0, SI :: GAMEPAD_BUTTON_LEFT_TRIGGER_2 ) == false );
        CHECK( rig.pInput -> IsGamepadButtonDown( 0, SI :: GAMEPAD_BUTTON_RIGHT_TRIGGER_2 ) == true );
        CHECK( rig.pInput -> IsGamepadButtonDown( 0, SI :: GAMEPAD_BUTTON_LEFT_TRIGGER_1 ) == false );
        CHECK( rig.pInput -> IsGamepadButtonDown( 0, SI :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1 ) == false );
    }

    TEST_CASE( "GetGamepadButtonPressed is the last button held at the last Update, in GLFW's order, never a second trigger" )  {

        OnePad  rig;

        CHECK( rig.pInput -> GetGamepadButtonPressed() == SI :: GAMEPAD_BUTTON_UNKNOWN );

        // A comes before the d-pad in GLFW's order: the d-pad wins.
        rig.Pad().buttons = { ISDLControllerApi :: BUTTON_DPAD_LEFT, ISDLControllerApi :: BUTTON_A };
        rig.pInput -> Update();
        CHECK( rig.pInput -> GetGamepadButtonPressed() == SI :: GAMEPAD_BUTTON_LEFT_FACE_LEFT );

        // Guide comes after start in GLFW's order (back, start, guide).
        rig.Pad().buttons = { ISDLControllerApi :: BUTTON_GUIDE, ISDLControllerApi :: BUTTON_START };
        rig.pInput -> Update();
        CHECK( rig.pInput -> GetGamepadButtonPressed() == SI :: GAMEPAD_BUTTON_MIDDLE );

        // Held every frame, it stays; a pulled trigger alone is never reported; released, it resets.
        rig.pInput -> Update();
        CHECK( rig.pInput -> GetGamepadButtonPressed() == SI :: GAMEPAD_BUTTON_MIDDLE );

        rig.Pad().buttons.clear();
        rig.Pad().axes[ ISDLControllerApi :: AXIS_TRIGGERLEFT ] = AXIS_MAX;
        rig.pInput -> Update();
        CHECK( rig.pInput -> IsGamepadButtonDown( 0, SI :: GAMEPAD_BUTTON_LEFT_TRIGGER_2 ) );
        CHECK( rig.pInput -> GetGamepadButtonPressed() == SI :: GAMEPAD_BUTTON_UNKNOWN );
    }

    TEST_CASE( "GetGamepadButtonPressed walks the pads in id order: the higher pad's button wins" )  {

        OnePad  rig;

        rig.Plug( "Second", 8 );
        rig.pInput -> Update();
        REQUIRE( rig.pInput -> IsGamepadAvailable( 1 ) );

        rig.Pad().buttons                 = { ISDLControllerApi :: BUTTON_DPAD_LEFT };
        rig.pApi -> Device( 8 ).buttons   = { ISDLControllerApi :: BUTTON_A };
        rig.pInput -> Update();
        CHECK( rig.pInput -> GetGamepadButtonPressed() == SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN );
        CHECK( rig.pInput -> IsGamepadButtonDown( 0, SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == false );
        CHECK( rig.pInput -> IsGamepadButtonDown( 1, SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == true );
    }

    TEST_CASE( "A pad plugged in or unplugged shows on the next Update" )  {

        OnePad  rig;

        rig.Plug( "Second", 8 );
        CHECK( rig.pInput -> IsGamepadAvailable( 1 ) == false );
        rig.pInput -> Update();
        CHECK( rig.pInput -> IsGamepadAvailable( 1 ) );

        rig.Unplug( 7 );
        rig.pInput -> Update();
        CHECK( rig.pInput -> IsGamepadAvailable( 0 ) == false );
        CHECK( rig.pInput -> IsGamepadAvailable( 1 ) );
    }
}

TEST_SUITE( "backends/sdl/SDLGamepadHaptics on the pads' devices" )  {

    TEST_CASE( "Two pads with the same name each rumble their own device, and haptics never opens one itself" )  {

        MockClockFixture  clockFixture;
        Devices           rig;
        MockInputHandler  input;

        rig.Plug( "Pro Controller", 7 );
        rig.Plug( "Pro Controller", 8 );
        rig.pDevices -> Update();
        REQUIRE( rig.log.nOpens == 2 );

        SDLGamepadHaptics  haptics( input, rig.pDevices );

        CHECK( haptics.IsSupported( 0 ) );
        CHECK( haptics.IsSupported( 1 ) );
        CHECK( haptics.IsSupported( 2 ) == false );

        haptics.SetVibration( 1, 1.0f, 1.0f, 0.0f );
        haptics.SetVibration( 0, 0.5f, 0.5f, 0.0f );
        REQUIRE( rig.log.rumbles.size() == 2 );
        CHECK( rig.log.rumbles[0].nInstanceId == 8 );
        CHECK( rig.log.rumbles[0].nLow == 65535 );
        CHECK( rig.log.rumbles[1].nInstanceId == 7 );
        CHECK( rig.log.rumbles[1].nLow == 32767 );
        CHECK( rig.log.nOpens == 2 );                       // no open of its own
    }

    TEST_CASE( "A rumble's deadline stops it; an unplugged pad does not rumble; the haptics' end closes nothing" )  {

        MockClockFixture  clockFixture;
        Devices           rig;
        MockInputHandler  input;

        rig.Plug( "Pad", 7 );
        rig.pDevices -> Update();

        {
            SDLGamepadHaptics  haptics( input, rig.pDevices );

            haptics.SetVibration( 0, 1.0f, 1.0f, 0.25f );
            clockFixture.clock.Advance( 250 );
            haptics.Update();
            REQUIRE( rig.log.rumbles.size() == 2 );
            CHECK( rig.log.rumbles[1].nLow == 0 );
            CHECK( rig.log.rumbles[1].nDurationMs == 0 );

            rig.Unplug( 7 );
            rig.pDevices -> Update();
            CHECK( haptics.IsSupported( 0 ) == false );
            haptics.SetVibration( 0, 1.0f, 1.0f, 0.0f );
            CHECK( rig.log.rumbles.size() == 2 );
        }

        CHECK( rig.log.nCloses == 1 );                      // the unplug's, not the haptics'
    }

    TEST_CASE( "Made with only an input handler, it uses the live process-wide devices when there are some" )  {

        MockClockFixture  clockFixture;
        FakeLog           log;
        FakeControllerApi *pApi = nullptr;
        MockInputHandler  input;

        SDLGamepadDevices :: SetApiCreator( [&]( void ) -> std :: unique_ptr<ISDLControllerApi>  {
            std :: unique_ptr<FakeControllerApi>  pFake = std :: make_unique<FakeControllerApi>( log );

            pApi = pFake.get();
            pFake -> devices.push_back( FakeDevice { "Pad", 7, true, true } );

            return pFake;
        } );

        {
            std :: shared_ptr<SDLGamepadDevices>  pDevices = SDLGamepadDevices :: Acquire();

            REQUIRE( SDLGamepadDevices :: Current() == pDevices );
            pDevices -> Update();

            SDLGamepadHaptics  haptics( input );

            haptics.SetVibration( 0, 1.0f, 1.0f, 0.0f );
            REQUIRE( log.rumbles.size() == 1 );
            CHECK( log.rumbles[0].nInstanceId == 7 );
        }

        SDLGamepadDevices :: SetApiCreator( nullptr );
        CHECK( SDLGamepadDevices :: Current() == nullptr );
        CHECK( log.nCloses == 1 );
    }
}

TEST_SUITE( "input/GamepadInputFactory" )  {

    TEST_CASE( "The build's backend is SDL when SDL starts, raylib/GLFW when it does not" )  {

        FakeLog  log;
        bool     bReady = true;

        SDLGamepadDevices :: SetApiCreator( [&]( void ) -> std :: unique_ptr<ISDLControllerApi>  {
            std :: unique_ptr<FakeControllerApi>  pFake = std :: make_unique<FakeControllerApi>( log );

            pFake -> bReady = bReady;

            return pFake;
        } );

        {
            std :: unique_ptr<SI :: IGamepadInput>  pInput = SI :: GamepadInputFactory :: CreateGamepadInput();

            CHECK( dynamic_cast<SDLGamepadInput*>( pInput.get() ) != nullptr );
        }

        bReady = false;

        {
            std :: unique_ptr<SI :: IGamepadInput>  pInput = SI :: GamepadInputFactory :: CreateGamepadInput();

            CHECK( dynamic_cast<SunLight :: Input :: RayLib :: RayLibGamepadInput*>( pInput.get() ) != nullptr );
            CHECK( SDLGamepadDevices :: Current() == nullptr );
        }

        SDLGamepadDevices :: SetApiCreator( nullptr );
    }

    TEST_CASE( "SetCreator overrides the build's backend, and an empty one restores it" )  {

        struct Fixed : public SI :: IGamepadInput  {
            void Update( void ) override                                              {}
            const char* GetGamepadName( int ) override                               { return "fixed"; }
            bool IsGamepadAvailable( int ) override                                  { return true; }
            bool IsGamepadButtonDown( int, SI :: GamepadButton ) override            { return false; }
            bool IsGamepadButtonUp( int, SI :: GamepadButton ) override              { return true; }
            int GetGamepadButtonPressed( void ) override                             { return 0; }
            int GetGamepadAxisCount( int ) override                                  { return 0; }
            float GetGamepadAxisMovement( int, SI :: GamepadAxis ) override          { return 0.0f; }
        };

        SI :: GamepadInputFactory :: SetCreator( []( void ) -> std :: unique_ptr<SI :: IGamepadInput>  { return std :: make_unique<Fixed>(); } );

        std :: unique_ptr<SI :: IGamepadInput>  pInput = SI :: GamepadInputFactory :: CreateGamepadInput();

        CHECK( std :: string( pInput -> GetGamepadName( 0 ) ) == "fixed" );

        SI :: GamepadInputFactory :: SetCreator( nullptr );
    }
}

TEST_SUITE( "renderer/TileMapRenderer input update" )  {

    TEST_CASE( "Run updates the input handler once per frame, right after the window's EndFrame" )  {

        MockEngineFixture        engineFixture;
        MockWindowFixture        windowFixture;
        MockInputHandlerFixture  inputFixture;
        SunLight :: Renderer :: RendererConfig  config;

        config.fWidth  = 320.0f;
        config.fHeight = 240.0f;

        std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  pRenderer = SunLight :: Renderer :: TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );
        REQUIRE( inputFixture.pLastCreated != nullptr );
        REQUIRE( pRenderer -> Start() == true );

        std :: vector<int>  endFramesAtUpdate;

        inputFixture.pLastCreated -> onUpdate = [&]( void )  { endFramesAtUpdate.push_back( windowFixture.window.nEndFrameCalls ); };
        windowFixture.window.nFramesUntilShouldClose = 3;
        pRenderer -> Run();

        CHECK( inputFixture.pLastCreated -> nUpdateCalls == 3 );
        CHECK( endFramesAtUpdate == std :: vector<int> { 1, 2, 3 } );

        pRenderer -> Stop();
    }
}
