/*
 * iscriptlistener.h
 *
 *  Created on: Oct 31, 2022
 *      Author: popolony2k
 */

#ifndef __ISCRIPTLISTENER_H__
#define __ISCRIPTLISTENER_H__

#include <stdint.h>
#include <string>


namespace SunLight {
    namespace Scripting  {

        /**
         * @brief Scripting commands available for processing;
         */
        enum Commands  {
            WAIT_CMD = 0,
            MOVE_SPRITES_TO_SCREEN_CMD,
            WAIT_SPRITES_QUEUE_EMPTY,
            LOOP_CMD,
            END_LOOP_CMD,
            LABEL_CMD,
            GOTO_LABEL_CMD,
            PLAY_SONG_CMD,
            PAUSE_SONG_CMD,
            RESUME_SONG_CMD,
            STOP_SONG_CMD,
            PLAY_SONG_DIRECT_CMD,
            PAUSE_SONG_DIRECT_CMD,
            RESUME_SONG_DIRECT_CMD,
            STOP_SONG_DIRECT_CMD,
            LOAD_STAGE_CMD
        };

        /**
         * @brief Event listener interface to respond script events.
         * 
         */
        class IScriptListener  {

            public:

            /**
             * @brief Must be implemented to respond STOP_SONG_CMD command; 
             * 
             * @param cmd The command received/
             * @param nEventId The event id; 
             */
            virtual void OnCommand( Commands cmd, uint16_t nEventId ) = 0;

            /**
             * @brief Runtime error handling.
             * 
             * @param strError The engine error message received;
             */
            virtual void OnError( std :: string strError ) = 0;
        };
    }
}

#endif  /* __ISCRIPTLISTENER_H__ */
