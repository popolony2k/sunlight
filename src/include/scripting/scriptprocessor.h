/*
 * scriptprocessor.h
 *
 *  Created on: Oct 13, 2022
 *      Author: popolony2k
 */

#ifndef __SCRIPTPROCESSOR_H__
#define __SCRIPTPROCESSOR_H__

#include <deque>
#include <map>
#include "base/object.h"
#include "scripting/iscriptlistener.h"


namespace SunLight {
    namespace Scripting  {

        /**
         * @brief Base command used for all other defined commands;
         */
        struct BaseCommand  {
            SunLight :: Scripting :: Commands   cmd;
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

        /**
         * @brief Command queue definition;
         */
        typedef std :: deque<BaseCommand*> CommandQueue;

        /**
         * @brief Command reference map definition;
         */
        typedef std :: map<uint16_t, CommandQueue :: iterator> CommandLabelMap;

        /**
         * @brief Script processor class implementation for script 
         * commands processing;
         */
        class ScriptProcessor : public SunLight :: Base :: Object  {

            CommandQueue                             m_CommandQueue;
            CommandLabelMap                          m_CommandLabelMap;
            CommandQueue :: iterator                 m_CurrentCommand;
            uint64_t                                 m_nWaitMilli;
            SunLight :: Scripting :: IScriptListener *m_pListener;
            bool                                     m_bWaitSpritesQueueEmpty;

            public:

            ScriptProcessor( void );
            virtual ~ScriptProcessor( void );

            void Add( SunLight :: Scripting :: BaseCommand *pCmd );
            void AddNoParmCmd( SunLight :: Scripting :: Commands cmd );
            void AddOneParmCmd( SunLight :: Scripting :: Commands cmd, uint16_t nParm );
            void AddTwoParmsCmd( SunLight :: Scripting :: Commands cmd, uint16_t nParm1, uint16_t nParm2 );
            void ResetWaitSpritesQueueEmptyCmd( void );
            void Clear( void );
            bool Compile( void );
            bool Run( void );

            void AddScriptListener( SunLight :: Scripting :: IScriptListener *pListener );
            IScriptListener* GetScriptListener( void );
        };
    }
}

#endif /* __SCRIPTPROCESSOR_H__ */
