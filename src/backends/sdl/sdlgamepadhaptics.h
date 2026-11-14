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

#ifndef __SDLGAMEPADHAPTICS_H__
#define __SDLGAMEPADHAPTICS_H__

#include "input/igamepadhaptics.h"
#include "input/iinputhandler.h"
#include "backends/sdl/isdlcontrollerapi.h"
#include "backends/sdl/sdlgamepaddevices.h"
#include <cstdint>
#include <map>
#include <memory>


namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief Real gamepad rumble via SDL2's GameController API (F7b of the master plan) -
             * raylib/GLFW has no vibration support at all.
             *
             * Two ways to find a pad's SDL controller:
             * - When the pads themselves are read through SDL (I25: SDLGamepadInput, the default),
             *   the controller is the one SDLGamepadDevices opened for that pad id - the same device
             *   by construction, opened when it was plugged in, not on the first rumble.
             * - Otherwise (DEFAULT_GAMEPAD_ENGINE 2, or SDL's pads could not start), nGamePadId is
             *   raylib/GLFW's own device index, correlated with SDL's own, separately-enumerated
             *   device list on every call via MatchGamepadsByName - exactly one gamepad on each side
             *   is always the same device; with more than one on either side it falls back to a
             *   best-effort name match, not a guaranteed-unique device identity (see
             *   gamepadidentitymatcher.h for why - confirmed live that even a single,
             *   correctly-detected real controller can have disagreeing names between the two
             *   libraries). Each pad's controller is then opened once and kept open
             *   (m_Controllers): reused while it stays attached and is still the device that
             *   matches the pad, reopened otherwise, closed by the destructor.
             *
             * Why controllers stay open: measured live (an Xbox One S Controller on macOS, SDL's
             * HIDAPI driver), opening one and sending a rumble each take well under 0.1 ms, but
             * closing one right after a rumble took about 11 ms on the calling thread - SDL's close
             * sends "motors off" (SDL_JoystickClose) and the HIDAPI close then waits, in 10 ms
             * SDL_Delay steps (up to 30 ms), for that pending rumble to go out
             * (HIDAPI_JoystickClose) - and it cut every rumble short. SDL's macOS HID open is not
             * exclusive (kIOHIDOptionsTypeNone), so raylib/GLFW keeps reading the same pad while
             * it is open.
             *
             * SDL2's own rumble expiration only runs inside SDL_JoystickUpdate(), which only
             * SDLGamepadDevices calls (once per frame, through the input handler's Update) - so on
             * the name-matching path nothing would ever stop a rumble. m_VibrationDeadlines tracks
             * each vibrating pad's own stop time (SunLight::General::Clock, not SDL's own clock)
             * on both paths, and Update() (the owner's own per-frame tick) calls the real Stop()
             * once it passes.
             */
            class SDLGamepadHaptics : public SunLight :: Input :: IGamepadHaptics  {

                // One opened controller, and the SDL instance id of the device it was opened on.
                struct OpenController  {
                    ISDLControllerApi :: ControllerHandle  hController;
                    int32_t                                nInstanceId;
                };

                SunLight :: Input :: IInputHandler&           m_InputHandler;
                std :: shared_ptr<SDLGamepadDevices>          m_pDevices;       // the pads' own devices (I25), or nullptr
                std :: unique_ptr<ISDLControllerApi>          m_pApi;           // the name-matching path's API, when m_pDevices is nullptr
                std :: map<int, int64_t>                      m_VibrationDeadlines;
                mutable std :: map<int, OpenController>       m_Controllers;    // by nGamePadId; IsSupported fills it too

                // The open controller for nGamePadId, opened (or reopened) when needed - nullptr if
                // the pad has no matching SDL device or it cannot be opened.
                ISDLControllerApi :: ControllerHandle ControllerFor( int nGamePadId ) const;
                void CloseController( int nGamePadId ) const;
                ISDLControllerApi& Api( void ) const;

                // The SDL device index matching nGamePadId's raylib/GLFW identity, or -1 if
                // nGamePadId is unavailable or no SDL device matched it.
                int FindMatchingSDLIndex( int nGamePadId ) const;

                public:

                // The pads' own SDL devices when they are read through SDL (SDLGamepadDevices ::
                // Current), the name-matching path on the real SDL API otherwise.
                explicit SDLGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler );

                // The name-matching path on another ISDLControllerApi (the tests' fake); nullptr
                // means the real one.
                SDLGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler, std :: unique_ptr<ISDLControllerApi> pApi );

                // On these devices (the tests' own, on a fake API).
                SDLGamepadHaptics( SunLight :: Input :: IInputHandler& inputHandler, std :: shared_ptr<SDLGamepadDevices> pDevices );
                ~SDLGamepadHaptics( void ) override;

                bool IsSupported( int nGamePadId ) const override;
                void SetVibration( int nGamePadId, float fLeftMotor, float fRightMotor, float fDuration ) override;
                void Stop( int nGamePadId ) override;
                void Update( void ) override;
            };
        }
    }
}

#endif /* __SDLGAMEPADHAPTICS_H__ */
