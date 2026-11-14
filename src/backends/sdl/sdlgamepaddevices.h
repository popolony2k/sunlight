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

#ifndef __SDLGAMEPADDEVICES_H__
#define __SDLGAMEPADDEVICES_H__

#include "backends/sdl/isdlcontrollerapi.h"
#include "input/iinputhandler.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>


#define SDL_GAMEPAD_BUTTON_COUNT       15         // ISDLControllerApi :: BUTTON_A .. BUTTON_DPAD_RIGHT
#define SDL_GAMEPAD_AXIS_COUNT         6          // ISDLControllerApi :: AXIS_LEFTX .. AXIS_TRIGGERRIGHT

namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief The SDL game controllers sunlight has open, one per gamepad id (I25 of the
             * master plan) - the one device list both the pad input (SDLGamepadInput) and the
             * rumble (SDLGamepadHaptics) use, so a pad's id names the same physical device for
             * both and no matching between two libraries' lists is needed.
             *
             * Every SDL call is made by ONE thread of its own, from starting SDL to stopping it
             * (I25b). SDL's HIDAPI drivers set a pad up by writing commands to it and waiting for
             * the replies on the calling thread - the Switch driver waits up to 100 ms per reply,
             * retries failed writes and waits 250 ms more when the pad is opened
             * (SDL_hidapi_switch.c) - and they do it while SDL starts (HIDAPI_JoystickInit), while
             * it finds a new device (inside SDL_JoystickUpdate) and while it opens one, all with
             * SDL's joystick lock held. Caravellius measured 2 to 4 s per Bluetooth pad on macOS. On
             * the game's thread that froze the game; here it only delays the pad. One thread for
             * everything also suits SDL's macOS drivers, which tie their devices to the run loop of
             * the thread that started them and run it themselves (SDL_iokitjoystick.c,
             * hidapi/mac/hid.c).
             *
             * That thread's pass (Step) updates SDL, closes each controller whose device went away,
             * opens each new one at the lowest free id (0 to MAX_GAMEPAD_COUNT - 1, kept until it
             * is unplugged; a device that fails to open is not retried while it stays listed),
             * carries out the rumble requests, and publishes a snapshot of every pad. The game's
             * thread never calls SDL: it reads the latest snapshot (Read, PadInfo) and queues
             * rumble requests (Rumble). A pad being set up simply appears a few passes later.
             *
             * RUN_MODE_MANUAL runs no thread: whoever calls Step makes the SDL calls - the tests,
             * on a fake API.
             */
            class SDLGamepadDevices  {

                public:

                typedef std :: function<std :: unique_ptr<ISDLControllerApi>( void )>  ApiCreatorFunction;

                enum RunMode  {
                    RUN_MODE_THREAD,       // a thread of its own makes every SDL call
                    RUN_MODE_MANUAL        // the caller of Step does (tests)
                };

                enum State  {
                    STATE_STARTING,        // SDL is starting (on the thread, this can take seconds)
                    STATE_READY,           // SDL started: pads are read through it
                    STATE_FAILED           // SDL's game-controller subsystem could not start
                };

                // One pad, as the last pass saw it.
                struct stPadState  {
                    bool                                          bAvailable = false;
                    bool                                          bHasRumble = false;
                    std :: string                                 strName;
                    std :: array<bool, SDL_GAMEPAD_BUTTON_COUNT>  buttons {};       // by ISDLControllerApi :: Button
                    std :: array<int16_t, SDL_GAMEPAD_AXIS_COUNT> axes {};          // by ISDLControllerApi :: Axis, SDL's raw values
                };

                typedef std :: array<stPadState, MAX_GAMEPAD_COUNT>  Snapshot;

                private:

                // One gamepad id's controller, the thread's own: none while hController is nullptr.
                struct stPad  {
                    ISDLControllerApi :: ControllerHandle  hController = nullptr;
                    int32_t                                nInstanceId = 0;
                    std :: string                          strName;
                };

                struct stRumbleRequest  {
                    int       nGamePadId;
                    uint16_t  nLowFrequency;
                    uint16_t  nHighFrequency;
                    uint32_t  nDurationMs;
                };

                // The stepping thread's own (the gamepad thread, or the RUN_MODE_MANUAL caller).
                ApiCreatorFunction                            m_ApiCreator;
                std :: unique_ptr<ISDLControllerApi>          m_pApi;
                std :: array<stPad, MAX_GAMEPAD_COUNT>        m_Pads;
                std :: set<int32_t>                           m_FailedInstances;

                // Shared between the two sides.
                mutable std :: mutex                          m_Mutex;
                Snapshot                                      m_Published;       // guarded by m_Mutex
                std :: deque<stRumbleRequest>                 m_Requests;        // guarded by m_Mutex
                std :: atomic<int>                            m_nState;
                std :: atomic<bool>                           m_bStop;
                std :: thread                                 m_Thread;

                void Start( void );
                void Shutdown( void );
                void Run( void );
                bool HasInstance( int32_t nInstanceId ) const;
                int FreePad( void ) const;

                public:

                /**
                 * @brief A device set on the API creator makes (nullptr: the real SDL API). With
                 * RUN_MODE_THREAD the thread starts here and makes the API itself; with
                 * RUN_MODE_MANUAL the API is made here, on the calling thread.
                 * @param creator Makes the API, or nullptr for the real one;
                 * @param mode Who makes the SDL calls;
                 */
                SDLGamepadDevices( ApiCreatorFunction creator, RunMode mode );

                // Stops the thread (waiting for SDL's current call to finish, which can be a pad's
                // setup: seconds, by the owner's choice), then closes every pad and stops SDL there.
                ~SDLGamepadDevices( void );
                SDLGamepadDevices( const SDLGamepadDevices& ) = delete;
                SDLGamepadDevices& operator=( const SDLGamepadDevices& ) = delete;

                /**
                 * @brief The process-wide set: the live one if there is one, a new one on its own
                 * thread otherwise. It lives while someone holds it.
                 */
                static std :: shared_ptr<SDLGamepadDevices> Acquire( void );

                /**
                 * @brief The live process-wide set unless SDL failed to start for it (pads then come
                 * from another backend) - nullptr otherwise.
                 */
                static std :: shared_ptr<SDLGamepadDevices> Current( void );

                /**
                 * @brief The API, and the run mode, a new set from Acquire() is made with - for tests
                 * only (a fake, stepped by hand). An empty function restores the real SDL API on its
                 * own thread.
                 * @param creator The replacement API creator, or an empty std::function;
                 * @param mode The run mode for the sets Acquire() makes with it;
                 */
                static void SetApiCreator( ApiCreatorFunction creator, RunMode mode = RUN_MODE_MANUAL );

                // The stepping side: one pass (see the class's doc comment). False once SDL failed.
                bool Step( void );

                // The game's side: never calls SDL.
                State GetState( void ) const;
                void Read( Snapshot& snapshot ) const;
                void PadInfo( int nGamePadId, bool& bAvailable, bool& bHasRumble ) const;
                void Rumble( int nGamePadId, uint16_t nLowFrequency, uint16_t nHighFrequency, uint32_t nDurationMs );
            };
        }
    }
}

#endif /* __SDLGAMEPADDEVICES_H__ */
