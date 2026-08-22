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

#ifndef __IINPUTHANDLERFACTORY_H__
#define __IINPUTHANDLERFACTORY_H__

#include <memory>
#include <functional>
#include "iinputhandler.h"


namespace SunLight {
    namespace Input  {

        /**
         * @brief Input handler class factory used to create
         * @see IInputHandler objects based on configured
         * backend renderer engine.
         *
         * Like @see SunLight :: Sound :: SoundFactory (and unlike @see
         * SunLight :: Engines :: EngineFactory, which hands back a single
         * shared engine instance), each call here returns a brand new
         * @see IInputHandler.
         */
        class InputHandlerFactory  {

            public:

            typedef std :: function<std :: unique_ptr<IInputHandler>( void )>  CreatorFunction;

            static std :: unique_ptr<IInputHandler> CreateInputHandler( void );

            /**
             * @brief Override the backend used by @see CreateInputHandler() -
             * for tests only, to substitute a mock @see IInputHandler
             * without a real input device. Pass an empty @see
             * CreatorFunction to restore the default, build-time backend.
             *
             * @param creator The replacement creator function, or an empty
             * std::function to reset back to the default backend;
             */
            static void SetCreator( CreatorFunction creator );
        };
     }
}

#endif  /* __IINPUTHANDLERFACTORY_H__ */
