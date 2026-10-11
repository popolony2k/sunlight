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

#ifndef __DEFAULTCONTROLLERBINDINGS_H__
#define __DEFAULTCONTROLLERBINDINGS_H__

#include "input/icontrollermanager.h"
#include "tilemap/itilemap.h"


namespace SunLight {
    namespace Input  {

        /**
         * @brief An opt-in preset (F0b of the master plan) wiring the arrow keys and a
         * gamepad's DPad to camera panning, and PageUp/PageDown/Home/End (mirrored on a
         * gamepad's right face buttons) to zoom and reset - exactly the bindings
         * TileMapRenderer used to install automatically before F0a/F0b, now external and only
         * installed when a game explicitly asks for it. Stateless: a static method, not an
         * instance, since nothing here varies by backend or needs mocking - map and
         * controllerManager are both already mockable through their own interfaces.
         */
        class DefaultControllerBindings  {

            public:

            /**
             * @brief Register the default camera/zoom bindings into controllerManager, acting
             * on map. Registering twice (e.g. calling this more than once against the same
             * controllerManager) registers the handlers again, each firing independently - the
             * same "no deduplication" behavior SetUserKeyEventHandler itself already has.
             * @param map The map whose camera/zoom methods the bindings call;
             * @param controllerManager The controller manager to register the bindings into;
             */
            static void Install( SunLight :: TileMap :: ITileMap &map,
                                 SunLight :: Input :: IControllerManager &controllerManager );
        };
    }
}

#endif  /* __DEFAULTCONTROLLERBINDINGS_H__ */
