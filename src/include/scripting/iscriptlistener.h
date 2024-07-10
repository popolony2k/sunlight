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
