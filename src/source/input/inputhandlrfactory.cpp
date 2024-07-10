/*
 * iinputhandlerfactory.cpp
 *
 *  Created on: Jan 05, 2024
 *      Author: popolony2k
 */

#ifndef DEFAULT_ENGINE
    #error "Unexpected value of DEFAULT_ENGINE"
#endif

#if DEFAULT_ENGINE == 1    /* USES RAYLIB */
    #include "input/raylib/raylibinputhandler.h"   
    #define __DEFAULT_INPUT_HANDLER  SunLight :: Input :: RayLib :: RayLibInputHandler
#else
    #error "Unknown value of DEFAULT_ENGINE"
#endif

#include "input/inputhandlerfactory.h"


namespace SunLight {
    namespace Input  {

        /**
         * @brief Create a Input Handler object
         * 
         * @return Pointer to created IInputHandler; 
         */
        IInputHandler* InputHandlerFactory :: CreateInputHandler( void )  {

            IInputHandler* pInputHandler = new __DEFAULT_INPUT_HANDLER();

            return pInputHandler;
        } 
    }
}