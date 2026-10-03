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

#include "backends/raylib/raylibinputhandler.h"
#include <raylib.h>

// Our input enums are passed straight to raylib, so every value must equal raylib's.
// A difference would silently select a different key or button, so it is a build error.
// Checked at compile time: no run-time cost. Generated from iinputhandler.h and raylib.h.
// Keyboard keys
static_assert( SunLight :: Input :: KEY_NULL == ::KEY_NULL, "KEY_NULL differs from raylib" );
static_assert( SunLight :: Input :: KEY_APOSTROPHE == ::KEY_APOSTROPHE, "KEY_APOSTROPHE differs from raylib" );
static_assert( SunLight :: Input :: KEY_COMMA == ::KEY_COMMA, "KEY_COMMA differs from raylib" );
static_assert( SunLight :: Input :: KEY_MINUS == ::KEY_MINUS, "KEY_MINUS differs from raylib" );
static_assert( SunLight :: Input :: KEY_PERIOD == ::KEY_PERIOD, "KEY_PERIOD differs from raylib" );
static_assert( SunLight :: Input :: KEY_SLASH == ::KEY_SLASH, "KEY_SLASH differs from raylib" );
static_assert( SunLight :: Input :: KEY_ZERO == ::KEY_ZERO, "KEY_ZERO differs from raylib" );
static_assert( SunLight :: Input :: KEY_ONE == ::KEY_ONE, "KEY_ONE differs from raylib" );
static_assert( SunLight :: Input :: KEY_TWO == ::KEY_TWO, "KEY_TWO differs from raylib" );
static_assert( SunLight :: Input :: KEY_THREE == ::KEY_THREE, "KEY_THREE differs from raylib" );
static_assert( SunLight :: Input :: KEY_FOUR == ::KEY_FOUR, "KEY_FOUR differs from raylib" );
static_assert( SunLight :: Input :: KEY_FIVE == ::KEY_FIVE, "KEY_FIVE differs from raylib" );
static_assert( SunLight :: Input :: KEY_SIX == ::KEY_SIX, "KEY_SIX differs from raylib" );
static_assert( SunLight :: Input :: KEY_SEVEN == ::KEY_SEVEN, "KEY_SEVEN differs from raylib" );
static_assert( SunLight :: Input :: KEY_EIGHT == ::KEY_EIGHT, "KEY_EIGHT differs from raylib" );
static_assert( SunLight :: Input :: KEY_NINE == ::KEY_NINE, "KEY_NINE differs from raylib" );
static_assert( SunLight :: Input :: KEY_SEMICOLON == ::KEY_SEMICOLON, "KEY_SEMICOLON differs from raylib" );
static_assert( SunLight :: Input :: KEY_EQUAL == ::KEY_EQUAL, "KEY_EQUAL differs from raylib" );
static_assert( SunLight :: Input :: KEY_A == ::KEY_A, "KEY_A differs from raylib" );
static_assert( SunLight :: Input :: KEY_B == ::KEY_B, "KEY_B differs from raylib" );
static_assert( SunLight :: Input :: KEY_C == ::KEY_C, "KEY_C differs from raylib" );
static_assert( SunLight :: Input :: KEY_D == ::KEY_D, "KEY_D differs from raylib" );
static_assert( SunLight :: Input :: KEY_E == ::KEY_E, "KEY_E differs from raylib" );
static_assert( SunLight :: Input :: KEY_F == ::KEY_F, "KEY_F differs from raylib" );
static_assert( SunLight :: Input :: KEY_G == ::KEY_G, "KEY_G differs from raylib" );
static_assert( SunLight :: Input :: KEY_H == ::KEY_H, "KEY_H differs from raylib" );
static_assert( SunLight :: Input :: KEY_I == ::KEY_I, "KEY_I differs from raylib" );
static_assert( SunLight :: Input :: KEY_J == ::KEY_J, "KEY_J differs from raylib" );
static_assert( SunLight :: Input :: KEY_K == ::KEY_K, "KEY_K differs from raylib" );
static_assert( SunLight :: Input :: KEY_L == ::KEY_L, "KEY_L differs from raylib" );
static_assert( SunLight :: Input :: KEY_M == ::KEY_M, "KEY_M differs from raylib" );
static_assert( SunLight :: Input :: KEY_N == ::KEY_N, "KEY_N differs from raylib" );
static_assert( SunLight :: Input :: KEY_O == ::KEY_O, "KEY_O differs from raylib" );
static_assert( SunLight :: Input :: KEY_P == ::KEY_P, "KEY_P differs from raylib" );
static_assert( SunLight :: Input :: KEY_Q == ::KEY_Q, "KEY_Q differs from raylib" );
static_assert( SunLight :: Input :: KEY_R == ::KEY_R, "KEY_R differs from raylib" );
static_assert( SunLight :: Input :: KEY_S == ::KEY_S, "KEY_S differs from raylib" );
static_assert( SunLight :: Input :: KEY_T == ::KEY_T, "KEY_T differs from raylib" );
static_assert( SunLight :: Input :: KEY_U == ::KEY_U, "KEY_U differs from raylib" );
static_assert( SunLight :: Input :: KEY_V == ::KEY_V, "KEY_V differs from raylib" );
static_assert( SunLight :: Input :: KEY_W == ::KEY_W, "KEY_W differs from raylib" );
static_assert( SunLight :: Input :: KEY_X == ::KEY_X, "KEY_X differs from raylib" );
static_assert( SunLight :: Input :: KEY_Y == ::KEY_Y, "KEY_Y differs from raylib" );
static_assert( SunLight :: Input :: KEY_Z == ::KEY_Z, "KEY_Z differs from raylib" );
static_assert( SunLight :: Input :: KEY_LEFT_BRACKET == ::KEY_LEFT_BRACKET, "KEY_LEFT_BRACKET differs from raylib" );
static_assert( SunLight :: Input :: KEY_BACKSLASH == ::KEY_BACKSLASH, "KEY_BACKSLASH differs from raylib" );
static_assert( SunLight :: Input :: KEY_RIGHT_BRACKET == ::KEY_RIGHT_BRACKET, "KEY_RIGHT_BRACKET differs from raylib" );
static_assert( SunLight :: Input :: KEY_GRAVE == ::KEY_GRAVE, "KEY_GRAVE differs from raylib" );
static_assert( SunLight :: Input :: KEY_SPACE == ::KEY_SPACE, "KEY_SPACE differs from raylib" );
static_assert( SunLight :: Input :: KEY_ESCAPE == ::KEY_ESCAPE, "KEY_ESCAPE differs from raylib" );
static_assert( SunLight :: Input :: KEY_ENTER == ::KEY_ENTER, "KEY_ENTER differs from raylib" );
static_assert( SunLight :: Input :: KEY_TAB == ::KEY_TAB, "KEY_TAB differs from raylib" );
static_assert( SunLight :: Input :: KEY_BACKSPACE == ::KEY_BACKSPACE, "KEY_BACKSPACE differs from raylib" );
static_assert( SunLight :: Input :: KEY_INSERT == ::KEY_INSERT, "KEY_INSERT differs from raylib" );
static_assert( SunLight :: Input :: KEY_DELETE == ::KEY_DELETE, "KEY_DELETE differs from raylib" );
static_assert( SunLight :: Input :: KEY_RIGHT == ::KEY_RIGHT, "KEY_RIGHT differs from raylib" );
static_assert( SunLight :: Input :: KEY_LEFT == ::KEY_LEFT, "KEY_LEFT differs from raylib" );
static_assert( SunLight :: Input :: KEY_DOWN == ::KEY_DOWN, "KEY_DOWN differs from raylib" );
static_assert( SunLight :: Input :: KEY_UP == ::KEY_UP, "KEY_UP differs from raylib" );
static_assert( SunLight :: Input :: KEY_PAGE_UP == ::KEY_PAGE_UP, "KEY_PAGE_UP differs from raylib" );
static_assert( SunLight :: Input :: KEY_PAGE_DOWN == ::KEY_PAGE_DOWN, "KEY_PAGE_DOWN differs from raylib" );
static_assert( SunLight :: Input :: KEY_HOME == ::KEY_HOME, "KEY_HOME differs from raylib" );
static_assert( SunLight :: Input :: KEY_END == ::KEY_END, "KEY_END differs from raylib" );
static_assert( SunLight :: Input :: KEY_CAPS_LOCK == ::KEY_CAPS_LOCK, "KEY_CAPS_LOCK differs from raylib" );
static_assert( SunLight :: Input :: KEY_SCROLL_LOCK == ::KEY_SCROLL_LOCK, "KEY_SCROLL_LOCK differs from raylib" );
static_assert( SunLight :: Input :: KEY_NUM_LOCK == ::KEY_NUM_LOCK, "KEY_NUM_LOCK differs from raylib" );
static_assert( SunLight :: Input :: KEY_PRINT_SCREEN == ::KEY_PRINT_SCREEN, "KEY_PRINT_SCREEN differs from raylib" );
static_assert( SunLight :: Input :: KEY_PAUSE == ::KEY_PAUSE, "KEY_PAUSE differs from raylib" );
static_assert( SunLight :: Input :: KEY_F1 == ::KEY_F1, "KEY_F1 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F2 == ::KEY_F2, "KEY_F2 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F3 == ::KEY_F3, "KEY_F3 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F4 == ::KEY_F4, "KEY_F4 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F5 == ::KEY_F5, "KEY_F5 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F6 == ::KEY_F6, "KEY_F6 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F7 == ::KEY_F7, "KEY_F7 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F8 == ::KEY_F8, "KEY_F8 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F9 == ::KEY_F9, "KEY_F9 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F10 == ::KEY_F10, "KEY_F10 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F11 == ::KEY_F11, "KEY_F11 differs from raylib" );
static_assert( SunLight :: Input :: KEY_F12 == ::KEY_F12, "KEY_F12 differs from raylib" );
static_assert( SunLight :: Input :: KEY_LEFT_SHIFT == ::KEY_LEFT_SHIFT, "KEY_LEFT_SHIFT differs from raylib" );
static_assert( SunLight :: Input :: KEY_LEFT_CONTROL == ::KEY_LEFT_CONTROL, "KEY_LEFT_CONTROL differs from raylib" );
static_assert( SunLight :: Input :: KEY_LEFT_ALT == ::KEY_LEFT_ALT, "KEY_LEFT_ALT differs from raylib" );
static_assert( SunLight :: Input :: KEY_LEFT_SUPER == ::KEY_LEFT_SUPER, "KEY_LEFT_SUPER differs from raylib" );
static_assert( SunLight :: Input :: KEY_RIGHT_SHIFT == ::KEY_RIGHT_SHIFT, "KEY_RIGHT_SHIFT differs from raylib" );
static_assert( SunLight :: Input :: KEY_RIGHT_CONTROL == ::KEY_RIGHT_CONTROL, "KEY_RIGHT_CONTROL differs from raylib" );
static_assert( SunLight :: Input :: KEY_RIGHT_ALT == ::KEY_RIGHT_ALT, "KEY_RIGHT_ALT differs from raylib" );
static_assert( SunLight :: Input :: KEY_RIGHT_SUPER == ::KEY_RIGHT_SUPER, "KEY_RIGHT_SUPER differs from raylib" );
static_assert( SunLight :: Input :: KEY_KB_MENU == ::KEY_KB_MENU, "KEY_KB_MENU differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_0 == ::KEY_KP_0, "KEY_KP_0 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_1 == ::KEY_KP_1, "KEY_KP_1 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_2 == ::KEY_KP_2, "KEY_KP_2 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_3 == ::KEY_KP_3, "KEY_KP_3 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_4 == ::KEY_KP_4, "KEY_KP_4 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_5 == ::KEY_KP_5, "KEY_KP_5 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_6 == ::KEY_KP_6, "KEY_KP_6 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_7 == ::KEY_KP_7, "KEY_KP_7 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_8 == ::KEY_KP_8, "KEY_KP_8 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_9 == ::KEY_KP_9, "KEY_KP_9 differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_DECIMAL == ::KEY_KP_DECIMAL, "KEY_KP_DECIMAL differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_DIVIDE == ::KEY_KP_DIVIDE, "KEY_KP_DIVIDE differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_MULTIPLY == ::KEY_KP_MULTIPLY, "KEY_KP_MULTIPLY differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_SUBTRACT == ::KEY_KP_SUBTRACT, "KEY_KP_SUBTRACT differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_ADD == ::KEY_KP_ADD, "KEY_KP_ADD differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_ENTER == ::KEY_KP_ENTER, "KEY_KP_ENTER differs from raylib" );
static_assert( SunLight :: Input :: KEY_KP_EQUAL == ::KEY_KP_EQUAL, "KEY_KP_EQUAL differs from raylib" );
static_assert( SunLight :: Input :: KEY_BACK == ::KEY_BACK, "KEY_BACK differs from raylib" );
static_assert( SunLight :: Input :: KEY_MENU == ::KEY_MENU, "KEY_MENU differs from raylib" );
static_assert( SunLight :: Input :: KEY_VOLUME_UP == ::KEY_VOLUME_UP, "KEY_VOLUME_UP differs from raylib" );
static_assert( SunLight :: Input :: KEY_VOLUME_DOWN == ::KEY_VOLUME_DOWN, "KEY_VOLUME_DOWN differs from raylib" );

// Gamepad buttons
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN == ::GAMEPAD_BUTTON_UNKNOWN, "GAMEPAD_BUTTON_UNKNOWN differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_UP == ::GAMEPAD_BUTTON_LEFT_FACE_UP, "GAMEPAD_BUTTON_LEFT_FACE_UP differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT == ::GAMEPAD_BUTTON_LEFT_FACE_RIGHT, "GAMEPAD_BUTTON_LEFT_FACE_RIGHT differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_DOWN == ::GAMEPAD_BUTTON_LEFT_FACE_DOWN, "GAMEPAD_BUTTON_LEFT_FACE_DOWN differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_LEFT == ::GAMEPAD_BUTTON_LEFT_FACE_LEFT, "GAMEPAD_BUTTON_LEFT_FACE_LEFT differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_UP == ::GAMEPAD_BUTTON_RIGHT_FACE_UP, "GAMEPAD_BUTTON_RIGHT_FACE_UP differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT == ::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, "GAMEPAD_BUTTON_RIGHT_FACE_RIGHT differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN == ::GAMEPAD_BUTTON_RIGHT_FACE_DOWN, "GAMEPAD_BUTTON_RIGHT_FACE_DOWN differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT == ::GAMEPAD_BUTTON_RIGHT_FACE_LEFT, "GAMEPAD_BUTTON_RIGHT_FACE_LEFT differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_1 == ::GAMEPAD_BUTTON_LEFT_TRIGGER_1, "GAMEPAD_BUTTON_LEFT_TRIGGER_1 differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_2 == ::GAMEPAD_BUTTON_LEFT_TRIGGER_2, "GAMEPAD_BUTTON_LEFT_TRIGGER_2 differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1 == ::GAMEPAD_BUTTON_RIGHT_TRIGGER_1, "GAMEPAD_BUTTON_RIGHT_TRIGGER_1 differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_2 == ::GAMEPAD_BUTTON_RIGHT_TRIGGER_2, "GAMEPAD_BUTTON_RIGHT_TRIGGER_2 differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_LEFT == ::GAMEPAD_BUTTON_MIDDLE_LEFT, "GAMEPAD_BUTTON_MIDDLE_LEFT differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE == ::GAMEPAD_BUTTON_MIDDLE, "GAMEPAD_BUTTON_MIDDLE differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_RIGHT == ::GAMEPAD_BUTTON_MIDDLE_RIGHT, "GAMEPAD_BUTTON_MIDDLE_RIGHT differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_THUMB == ::GAMEPAD_BUTTON_LEFT_THUMB, "GAMEPAD_BUTTON_LEFT_THUMB differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_THUMB == ::GAMEPAD_BUTTON_RIGHT_THUMB, "GAMEPAD_BUTTON_RIGHT_THUMB differs from raylib" );

// Gamepad axes
static_assert( SunLight :: Input :: GAMEPAD_AXIS_LEFT_X == ::GAMEPAD_AXIS_LEFT_X, "GAMEPAD_AXIS_LEFT_X differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_AXIS_LEFT_Y == ::GAMEPAD_AXIS_LEFT_Y, "GAMEPAD_AXIS_LEFT_Y differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_AXIS_RIGHT_X == ::GAMEPAD_AXIS_RIGHT_X, "GAMEPAD_AXIS_RIGHT_X differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_AXIS_RIGHT_Y == ::GAMEPAD_AXIS_RIGHT_Y, "GAMEPAD_AXIS_RIGHT_Y differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_AXIS_LEFT_TRIGGER == ::GAMEPAD_AXIS_LEFT_TRIGGER, "GAMEPAD_AXIS_LEFT_TRIGGER differs from raylib" );
static_assert( SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER == ::GAMEPAD_AXIS_RIGHT_TRIGGER, "GAMEPAD_AXIS_RIGHT_TRIGGER differs from raylib" );


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