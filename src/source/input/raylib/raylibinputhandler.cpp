/*
 * raylibinputhandler.cpp
 *
 *  Created on: Apr 06, 2022
 *      Author: popolony2k
 */

#include "input/raylib/raylibinputhandler.h"
#include <raylib.h>

namespace SunLight  {
    namespace Input  {
        namespace RayLib  {
            /**
             * @brief Constructor. Initialize all class data. 
             */
            RayLibInputHandler :: RayLibInputHandler( void )  {

            }

            /**
             * @brief Destructor. Finalize all class data. 
             */
            RayLibInputHandler :: ~RayLibInputHandler( void )  {

            }

            /**
             * @brief 
             * Get last key from user input selected control.
             */
            SunLight :: Input :: KeyboardKey RayLibInputHandler :: GetKeyPressed( void )  {

                /*
                * FIXME:
                * The cast below will work while SunLight :: Input :: KeyboardKey is the same as
                * raylib enum.
                */
                return ( SunLight :: Input :: KeyboardKey ) ::GetKeyPressed();
            }

            /**
             * @brief Check if a specified key is on down state. 
             * 
             * @param key Key code to be checked;
             * @return true If is pressed state;
             * @return false  If is not pressed state;
             */
            bool RayLibInputHandler :: IsKeyDown( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyDown( key );
            }

            /**
             * @brief Check if a specified key is on up state. 
             * 
             * @param key Key code to be checked;
             * @return true If is up state;
             * @return false  If is not up state;
             */
            bool RayLibInputHandler :: IsKeyUp( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyUp( key );
            }

            /**
             * @brief Check if a specified key was released;
             * 
             * @param key 
             * @return true 
             * @return false 
             */
            bool RayLibInputHandler :: IsKeyReleased( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyReleased( key );
            }

            /**
             * @brief Get the Gamepad Name for requested Game Pad Id;
             * 
             * @param nGamePadId The game pad id to check; 
             * @return const char* 
             */
            const char* RayLibInputHandler :: GetGamepadName( int nGamePadId )  {

                return ::GetGamepadName( nGamePadId );
            }

            /**
             * @brief Check if the requested gamepad is available;
             * 
             * @param nGamePadId The game pad id to check; 
             * @return true 
             * @return false 
             */
            bool RayLibInputHandler :: IsGamepadAvailable( int nGamePadId )  {

                return ::IsGamepadAvailable( nGamePadId );
            }
          
            /**
             * @brief Check if the specified button is in down state;
             * 
             * @param nGamePadId The game pad id to check; 
             * @param button Button code to check;
             * @return true If is pressed state;
             * @return false If is not in pressed state;
             */
            bool RayLibInputHandler :: IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                return ::IsGamepadButtonDown( nGamePadId, button );
            }

            /**
             * @brief Check if the specified button is in up state;
             * 
             * @param nGamePadId The game pad id to check; 
             * @param button Button code to check;
             * @return true If is pressed state;
             * @return false If is not in pressed state;
             */
            bool RayLibInputHandler :: IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                return ::IsGamepadButtonUp( nGamePadId, button );
            }

            /**
             * @brief Get the Gamepad index when the related GamePad Mid Button
             * is pressed; 
             * 
             * @return int The Gamepad axis index which was pressed;
             */
            int RayLibInputHandler :: GetGamepadButtonPressed( void )  {

                return ::GetGamepadButtonPressed();
            }

            /**
             * @brief Get the Gamepad Axis count;
             * 
             * @param nGamePadId The game pad id to check; 
             * @return int 
             */
            int RayLibInputHandler :: GetGamepadAxisCount( int nGamePadId )  {

                return ::GetGamepadAxisCount( nGamePadId );
            }

            /**
             * @brief Get the Gamepad Axis Movement position
             * 
             * @param nGamePadId The game pd id to check;
             * @param axis The axis to check;
             * @return float The returned position for requested gamepad and axis; 
             */
            float RayLibInputHandler :: GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis )  {

                return ::GetGamepadAxisMovement( nGamePadId, axis );
            }
        }
    }
}