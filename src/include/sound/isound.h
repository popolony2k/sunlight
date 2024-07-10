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

#ifndef __ISOUND_H__
#define __ISOUND_H__

#include <string>

namespace SunLight {
    namespace Sound  {

        /**
         * @brief ISound interface for handling sound data.
         */
        class ISound  {

            public:

            virtual ~ISound( void )  {}

            /**
             * @brief Must be implemented to provide audio data loading based on 
             * chosen target engine;
             * 
             * @param strFileName The audio data file name to load; 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            virtual bool Load( std :: string strFileName ) = 0;

            /**
             * @brief Must be implemented to unload previous data loaded by @see Load() 
             * method on chosen target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            virtual bool Unload( void ) = 0;

            /**
             * @brief Must be implemented to provide sound playing method on chosen 
             * target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            virtual bool Play( void ) = 0;

            /**
             * @brief Must be implemented to provide sound stop method on chosen 
             * target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            virtual bool Stop( void ) = 0;

            /**
             * @brief Must be implemented to provide sound pause method on chosen 
             * target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            virtual bool Pause( void ) = 0;

            /**
             * @brief Must be implemented to provide sound resume method on chosen 
             * target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            virtual bool Resume( void ) = 0;

            /**
             * @brief Check if audio data previously loaded is playing;
             * 
             * @return true If is playing;
             * @return false If is not playing;
             */
            virtual bool IsPlaying( void ) = 0;
        };
    }
}

#endif /* __ISOUND_H__ */