/*
 * scriptprocessor.cpp
 *
 *  Created on: Oct 13, 2022
 *      Author: popolony2k
 */

#include "scripting/scriptprocessor.h"
#include <chrono>

using namespace std :: chrono;


namespace SunLight {
    namespace Scripting  {

        /**
         * @brief Constructor. Initialize all class data;
         */
        ScriptProcessor :: ScriptProcessor( void )  {

            m_nWaitMilli = 0;
            m_pListener  = nullptr;
            m_bWaitSpritesQueueEmpty = false;
            Clear();
        }

        /**
         * @brief Destructor. Finalize all class data. 
         */
        ScriptProcessor :: ~ScriptProcessor( void )  {

            Clear();
        }

        /**
         * @brief Add a command to scripting queue;
         * 
         * @param pCmd Pointer to command that will be added;
         */
        void ScriptProcessor :: Add( SunLight :: Scripting :: BaseCommand *pCmd )  {

            m_CommandQueue.push_back( pCmd );
        }

        /**
         * @brief Allocates a no parameter command object and add it to processor;
         * @param cmd Command to be added;
         */
        void ScriptProcessor :: AddNoParmCmd( SunLight :: Scripting :: Commands cmd ) {

            BaseCommand *pCmd = new BaseCommand();

            pCmd -> cmd = cmd;
            Add( pCmd );
        }

        /**
         * @brief Allocates an one parameter command object and add it to processor;
         * 
         * @param cmd Command to be added;
         * @param nParm Parameter to be added;
         */
        void ScriptProcessor :: AddOneParmCmd( SunLight :: Scripting :: Commands cmd, uint16_t nParm ) {

            OneParmCommand  *pCmd = new OneParmCommand();

            pCmd -> cmd   = cmd;
            pCmd -> nParm = nParm;

            Add( pCmd );
        }

        /**
         * @brief Allocates a two parameters command object and add it to processor;
         * 
         * @param cmd Comand to be added;
         * @param nParm1 First parameter to be added;
         * @param nParm2 Second parameter to be added;
         */
        void ScriptProcessor :: AddTwoParmsCmd( SunLight :: Scripting :: Commands cmd, uint16_t nParm1, uint16_t nParm2 )  {

            TwoParmsCommand  *pCmd = new TwoParmsCommand();

            pCmd -> cmd   = cmd;
            pCmd -> nParm1 = nParm1;
            pCmd -> nParm2 = nParm2;

            Add( pCmd );
        }

        /**
         * @brief Reset the wait sprites queue empty flag;
         * 
         */
        void ScriptProcessor :: ResetWaitSpritesQueueEmptyCmd( void )  {

            m_bWaitSpritesQueueEmpty = false;
        }

        /**
         * @brief Clear the internal script processor queue, releasing 
         * the added pointer to this queue; 
         */
        void ScriptProcessor :: Clear( void )  {

            for( SunLight :: Scripting :: BaseCommand* pCmd : m_CommandQueue )  {
                delete pCmd;
            }

            m_CommandQueue.clear();
            m_CommandLabelMap.clear();
            m_CurrentCommand = m_CommandQueue.begin();
        }

        /**
         * @brief Compile all command list. Should be called before run engine.
         * 
         * @return true If program sequence is valid;
         * @return false If program sequence is invalid;
         */
        bool ScriptProcessor :: Compile( void )  {

            m_CurrentCommand = m_CommandQueue.begin();

            return ( !m_CommandQueue.empty() );
        }

        /**
         * @brief Script execute handling;
         * 
         * @return true if script processing was sucessful;
         */
        bool ScriptProcessor :: Run( void )  {

            if( !m_CommandQueue.empty() && ( m_CurrentCommand != m_CommandQueue.end() ) )  {
                
                if( !m_bWaitSpritesQueueEmpty )  {
                    /*
                    * Command processing 
                    */
                    switch( ( *m_CurrentCommand ) -> cmd )  {

                        case WAIT_SPRITES_QUEUE_EMPTY :
                            m_bWaitSpritesQueueEmpty = true;
                            break;

                        case  WAIT_CMD :  {
                            OneParmCommand   *pParm = ( OneParmCommand * ) *m_CurrentCommand;
                            uint64_t         nTime  = duration_cast<milliseconds> ( steady_clock :: now().time_since_epoch() ).count();

                            m_nWaitMilli = ( m_nWaitMilli != 0 ? m_nWaitMilli : nTime + pParm -> nParm );

                            if( nTime >= m_nWaitMilli )
                                m_nWaitMilli = 0;
                            else
                                return true;
                        }
                        break;
                        
                        case MOVE_SPRITES_TO_SCREEN_CMD :  {
                            OneParmCommand   *pParm = ( OneParmCommand * ) *m_CurrentCommand;

                            if( m_pListener != nullptr )
                                m_pListener -> OnCommand( ( *m_CurrentCommand ) -> cmd, pParm -> nParm );
                        }
                        break;

                        case LOOP_CMD :  {
                            TwoParmsCommand   *pParm = ( TwoParmsCommand * ) *m_CurrentCommand;

                            pParm -> data.nCounter = 0;
                            m_CommandLabelMap.insert( std ::make_pair( pParm -> nParm1, m_CurrentCommand ) );
                        }
                        break;

                        case END_LOOP_CMD :  {
                            OneParmCommand                  *pParm     = ( OneParmCommand * ) *m_CurrentCommand;
                            CommandLabelMap :: iterator     itLoopCmd  = m_CommandLabelMap.find( pParm ->nParm );
                            TwoParmsCommand                 *pLoopParm = ( TwoParmsCommand * ) ( *itLoopCmd -> second );

                            if( pLoopParm -> data.nCounter > 0 )  {
                                pLoopParm -> data.nCounter--;
                                m_CurrentCommand = itLoopCmd -> second;
                            }
                        }
                        break;

                        case LABEL_CMD :  {
                            OneParmCommand   *pParm = ( OneParmCommand * ) *m_CurrentCommand;

                            m_CommandLabelMap.insert( std ::make_pair( pParm -> nParm, m_CurrentCommand ) );
                        }
                        break;

                        case GOTO_LABEL_CMD :  {
                            OneParmCommand                  *pParm    = ( OneParmCommand * ) *m_CurrentCommand;
                            CommandLabelMap :: iterator     itLoopCmd = m_CommandLabelMap.find( pParm -> nParm );

                            m_CurrentCommand = itLoopCmd -> second;
                        }
                        break;

                        case LOAD_STAGE_CMD     :   // One integer parameter commands (common processing routine)
                        case PLAY_SONG_CMD  :
                        case PAUSE_SONG_CMD :
                        case RESUME_SONG_CMD:
                        case STOP_SONG_CMD  :  {
                            OneParmCommand   *pParm = ( OneParmCommand * ) *m_CurrentCommand;

                            if( m_pListener != nullptr )
                                m_pListener -> OnCommand( ( *m_CurrentCommand ) -> cmd, pParm -> nParm );
                        }
                        break;
                        
                        case PLAY_SONG_DIRECT_CMD  :  // Not handled by scripts
                        case PAUSE_SONG_DIRECT_CMD :
                        case RESUME_SONG_DIRECT_CMD:
                        case STOP_SONG_DIRECT_CMD  :
                        break;
                    }

                    m_CurrentCommand++;
                }

                return true;
            }

            return false;
        }

        /**
         * @brief Add script listener to this processor;
         * 
         * @param pListener A pointer to listener object;
         */
        void ScriptProcessor :: AddScriptListener( SunLight :: Scripting :: IScriptListener *pListener )  {

            m_pListener = pListener;
        }

        /**
         * @brief Get the Script Listener object;
         * 
         * @return IScriptListener previously assigned by @link AddScriptListener;
         */
        IScriptListener* ScriptProcessor :: GetScriptListener( void )  {

            return m_pListener;
        }
    }
}
