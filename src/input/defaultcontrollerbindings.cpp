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

#include "input/defaultcontrollerbindings.h"

namespace SunLight  {
    namespace Input  {

        void DefaultControllerBindings :: Install( SunLight :: TileMap :: ITileMap &map,
                                                    SunLight :: Input :: IControllerManager &controllerManager )  {

            // Keyboard
            controllerManager.SetUserKeyEventHandler( KEY_UP, [&map]( ControllerType, int ) { map.MoveCameraUp(); } );
            controllerManager.SetUserKeyEventHandler( KEY_DOWN, [&map]( ControllerType, int ) { map.MoveCameraDown(); } );
            controllerManager.SetUserKeyEventHandler( KEY_LEFT, [&map]( ControllerType, int ) { map.MoveCameraLeft(); } );
            controllerManager.SetUserKeyEventHandler( KEY_RIGHT, [&map]( ControllerType, int ) { map.MoveCameraRight(); } );
            controllerManager.SetUserKeyEventHandler( KEY_PAGE_UP, [&map]( ControllerType, int ) { map.ZoomIn(); } );
            controllerManager.SetUserKeyEventHandler( KEY_PAGE_DOWN, [&map]( ControllerType, int ) { map.ZoomOut(); } );
            controllerManager.SetUserKeyEventHandler( KEY_HOME, [&map]( ControllerType, int ) { map.ResetZoom(); } );
            controllerManager.SetUserKeyEventHandler( KEY_END, [&map]( ControllerType, int ) { map.ResetCamera(); } );

            // GamePad
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_UP, [&map]( ControllerType, int ) { map.MoveCameraUp(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_DOWN, [&map]( ControllerType, int ) { map.MoveCameraDown(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_LEFT, [&map]( ControllerType, int ) { map.MoveCameraLeft(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_LEFT_FACE_RIGHT, [&map]( ControllerType, int ) { map.MoveCameraRight(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_RIGHT_FACE_UP, [&map]( ControllerType, int ) { map.ZoomIn(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_RIGHT_FACE_DOWN, [&map]( ControllerType, int ) { map.ZoomOut(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_RIGHT_FACE_LEFT, [&map]( ControllerType, int ) { map.ResetZoom(); } );
            controllerManager.SetUserGamePadEventHandler( GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, [&map]( ControllerType, int ) { map.ResetCamera(); } );
        }
    }
}
