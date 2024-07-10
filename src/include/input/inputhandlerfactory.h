/*
 * iinputhandlerfactory.h
 *
 *  Created on: Jan 05, 2024
 *      Author: popolony2k
 */

#ifndef __IINPUTHANDLERFACTORY_H__
#define __IINPUTHANDLERFACTORY_H__

#include "iinputhandler.h"


namespace SunLight {
    namespace Input  {

        /**
         * @brief Input handler class factory used to create
         * @see IInputHandler objects based on configured 
         * backend renderer engine.
         */
        class InputHandlerFactory  {

            public:

            static IInputHandler* CreateInputHandler( void );
        };
     }
}

#endif  /* __IINPUTHANDLERFACTORY_H__ */
