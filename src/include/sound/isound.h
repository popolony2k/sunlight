/*
 * isound.h
 *
 *  Created on: Jan 2, 2024
 *      Author: popolony2k
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