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

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdint.h>


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
         * @brief Base command used for all other defined commands;
         */
        struct BaseCommand  {
            SunLight :: Scripting :: Commands   cmd;

            /*
             * The queue owns its commands through BaseCommand pointers, but the
             * concrete commands are larger (they add parameters), so deleting one
             * through a BaseCommand* without a virtual destructor is undefined
             * behaviour - AddressSanitizer reports it as new-delete-type-mismatch.
             */
            virtual ~BaseCommand( void )  {}
        };

        /**
         * @brief One parameter command data struct;
         */
        struct OneParmCommand : public SunLight :: Scripting :: BaseCommand  {
            uint16_t      nParm;
        };

        /**
         * @brief Two parameters command data struct;
         */
        struct TwoParmsCommand : public SunLight :: Scripting :: BaseCommand  {
            uint16_t      nParm1;
            uint16_t      nParm2;

            /**
             * @brief Internal command data;
             */
            struct CommandData  {
                uint16_t      nCounter;
            } data;
        };

    }
}

#endif  /* __COMMAND_H__ */
