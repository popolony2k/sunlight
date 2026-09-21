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

#include "renderer/rendererconfig.h"

#ifndef DEFAULT_ENGINE
    #error "Unexpected value of DEFAULT_ENGINE"
#endif


namespace SunLight {
    namespace Renderer  {

        /**
         * @brief Whether a backend is compiled into this build. raylib
         * exists when DEFAULT_ENGINE == 1; the null backend has no
         * dependencies, so it is part of every build.
         */
        bool RendererConfig :: IsBackendAvailable( RendererBackend backend )  {

            switch( backend )  {
                case RENDERER_BACKEND_DEFAULT :
                    return true;

                case RENDERER_BACKEND_RAYLIB :
                    return ( DEFAULT_ENGINE == 1 );

                case RENDERER_BACKEND_NULL :
                    return true;

                default :
                    return false;
            }
        }

        /**
         * @brief Name of a backend, for messages.
         */
        const char* RendererConfig :: BackendName( RendererBackend backend )  {

            switch( backend )  {
                case RENDERER_BACKEND_DEFAULT :  return "default";
                case RENDERER_BACKEND_RAYLIB  :  return "raylib";
                case RENDERER_BACKEND_NULL    :  return "null";
                default                       :  return "unknown";
            }
        }

        /**
         * @brief Check the config can produce a renderer.
         */
        bool RendererConfig :: Validate( std :: string *pError ) const  {

            std :: string  strError;

            if( ( backend < RENDERER_BACKEND_DEFAULT ) || ( backend >= RENDERER_BACKEND_LAST ) )
                strError = "unknown renderer backend value " + std :: to_string( ( int ) backend );
            else if( !IsBackendAvailable( backend ) )
                strError = std :: string( "renderer backend '" ) + BackendName( backend ) + "' is not available in this build";
            else if( ( fWidth < 1.0f ) || ( fHeight < 1.0f ) )
                strError = "renderer width and height must be at least 1 pixel";
            else if( ( fullscreenStrategy != SunLight :: Window :: FULLSCREEN_STRATEGY_REAL ) &&
                     ( fullscreenStrategy != SunLight :: Window :: FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED ) )
                strError = "unknown fullscreen strategy value " + std :: to_string( ( int ) fullscreenStrategy );

            if( pError )
                *pError = strError;

            return strError.empty();
        }
    }
}
