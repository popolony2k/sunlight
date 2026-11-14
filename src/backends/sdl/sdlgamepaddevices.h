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
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>


namespace SunLight  {
    namespace Input  {
        namespace SDL  {

            /**
             * @brief The SDL game controllers sunlight has open, one per gamepad id (I25 of the
             * master plan) - the one device list both the pad input (SDLGamepadInput) and the
             * rumble (SDLGamepadHaptics) use, so a pad's id names the same physical device for
             * both and no matching between two libraries' lists is needed.
             *
             * Update() is the per-frame step: it updates SDL (which also finds pads plugged in or
             * removed), closes each controller whose device went away and opens each new one, at
             * the lowest free id (0 to MAX_GAMEPAD_COUNT - 1), which it keeps until it is
             * unplugged. Opening happens here, on the frame a pad appears, not on its first use. A
             * device that fails to open is not retried while it stays plugged in.
             *
             * The real one is shared by the whole process (Acquire), since SDL's device list is
             * process-wide too; every controller is closed before its API goes.
             */
            class SDLGamepadDevices  {

                // One gamepad id's controller: none while hController is nullptr.
                struct stPad  {
                    ISDLControllerApi :: ControllerHandle  hController = nullptr;
                    int32_t                                nInstanceId = 0;
                    std :: string                          strName;
                };

                std :: unique_ptr<ISDLControllerApi>          m_pApi;
                std :: array<stPad, MAX_GAMEPAD_COUNT>        m_Pads;
                std :: set<int32_t>                           m_FailedInstances;

                bool HasInstance( int32_t nInstanceId ) const;
                int FreePad( void ) const;

                public:

                typedef std :: function<std :: unique_ptr<ISDLControllerApi>( void )>  ApiCreatorFunction;

                // With another ISDLControllerApi (the tests' fake); nullptr means the real one.
                explicit SDLGamepadDevices( std :: unique_ptr<ISDLControllerApi> pApi );
                ~SDLGamepadDevices( void );
                SDLGamepadDevices( const SDLGamepadDevices& ) = delete;
                SDLGamepadDevices& operator=( const SDLGamepadDevices& ) = delete;

                /**
                 * @brief The process-wide set on the real SDL API: the live one if there is one,
                 * a new one otherwise. It lives while someone holds it.
                 */
                static std :: shared_ptr<SDLGamepadDevices> Acquire( void );

                /**
                 * @brief The live process-wide set, if it is held and SDL started - nullptr
                 * otherwise (pads then come from another backend).
                 */
                static std :: shared_ptr<SDLGamepadDevices> Current( void );

                /**
                 * @brief The API a new set from Acquire() is made on - for tests only (a fake, to
                 * check GamepadInputFactory and SDLGamepadHaptics against it). An empty function
                 * restores the real SDL API.
                 * @param creator The replacement creator function, or an empty std::function;
                 */
                static void SetApiCreator( ApiCreatorFunction creator );

                bool IsReady( void ) const;
                void Update( void );

                bool IsAvailable( int nGamePadId ) const;
                ISDLControllerApi :: ControllerHandle Controller( int nGamePadId ) const;
                const char* Name( int nGamePadId ) const;
                ISDLControllerApi& Api( void ) const;
            };
        }
    }
}

#endif /* __SDLGAMEPADDEVICES_H__ */
