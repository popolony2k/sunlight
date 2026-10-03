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
#include <array>


namespace {

    // Our code -> raylib's code. A code outside the table maps to 0 (no key / no button).
    template<std :: size_t N>
    constexpr int ToRaylib( const std :: array<int, N>& table, int nCode )  {

        return ( ( nCode >= 0 ) && ( nCode < ( int ) N ) ) ? table[nCode] : 0;
    }

    // raylib's code -> ours: the first of our codes that maps to it. A raylib code we have no
    // name for (or no code at all, 0) gives 0.
    template<std :: size_t N>
    constexpr int FromRaylib( const std :: array<int, N>& table, int nRaylibCode )  {

        if( nRaylibCode == 0 )
            return 0;

        for( std :: size_t i = 0; i < N; i++ )  {
            if( table[i] == nRaylibCode )
                return ( int ) i;
        }

        return 0;
    }

        /*
         * Our input enums are passed to raylib as they are, so each key, gamepad button
         * and axis is translated to raylib's own code by NAME in these tables. Raylib
         * renumbering a code cannot break this, and a misspelled or removed name is a
         * compile error. Tables are built at compile time (a constexpr lambda).
         * Entries we don't map stay 0, which is KEY_NULL / GAMEPAD_BUTTON_UNKNOWN.
         */
        constexpr std :: array<int, 349> kRaylibKeys = []  {
            std :: array<int, 349> t {};
            t[SunLight :: Input :: KEY_NULL] = ::KEY_NULL;
            t[SunLight :: Input :: KEY_APOSTROPHE] = ::KEY_APOSTROPHE;
            t[SunLight :: Input :: KEY_COMMA] = ::KEY_COMMA;
            t[SunLight :: Input :: KEY_MINUS] = ::KEY_MINUS;
            t[SunLight :: Input :: KEY_PERIOD] = ::KEY_PERIOD;
            t[SunLight :: Input :: KEY_SLASH] = ::KEY_SLASH;
            t[SunLight :: Input :: KEY_ZERO] = ::KEY_ZERO;
            t[SunLight :: Input :: KEY_ONE] = ::KEY_ONE;
            t[SunLight :: Input :: KEY_TWO] = ::KEY_TWO;
            t[SunLight :: Input :: KEY_THREE] = ::KEY_THREE;
            t[SunLight :: Input :: KEY_FOUR] = ::KEY_FOUR;
            t[SunLight :: Input :: KEY_FIVE] = ::KEY_FIVE;
            t[SunLight :: Input :: KEY_SIX] = ::KEY_SIX;
            t[SunLight :: Input :: KEY_SEVEN] = ::KEY_SEVEN;
            t[SunLight :: Input :: KEY_EIGHT] = ::KEY_EIGHT;
            t[SunLight :: Input :: KEY_NINE] = ::KEY_NINE;
            t[SunLight :: Input :: KEY_SEMICOLON] = ::KEY_SEMICOLON;
            t[SunLight :: Input :: KEY_EQUAL] = ::KEY_EQUAL;
            t[SunLight :: Input :: KEY_A] = ::KEY_A;
            t[SunLight :: Input :: KEY_B] = ::KEY_B;
            t[SunLight :: Input :: KEY_C] = ::KEY_C;
            t[SunLight :: Input :: KEY_D] = ::KEY_D;
            t[SunLight :: Input :: KEY_E] = ::KEY_E;
            t[SunLight :: Input :: KEY_F] = ::KEY_F;
            t[SunLight :: Input :: KEY_G] = ::KEY_G;
            t[SunLight :: Input :: KEY_H] = ::KEY_H;
            t[SunLight :: Input :: KEY_I] = ::KEY_I;
            t[SunLight :: Input :: KEY_J] = ::KEY_J;
            t[SunLight :: Input :: KEY_K] = ::KEY_K;
            t[SunLight :: Input :: KEY_L] = ::KEY_L;
            t[SunLight :: Input :: KEY_M] = ::KEY_M;
            t[SunLight :: Input :: KEY_N] = ::KEY_N;
            t[SunLight :: Input :: KEY_O] = ::KEY_O;
            t[SunLight :: Input :: KEY_P] = ::KEY_P;
            t[SunLight :: Input :: KEY_Q] = ::KEY_Q;
            t[SunLight :: Input :: KEY_R] = ::KEY_R;
            t[SunLight :: Input :: KEY_S] = ::KEY_S;
            t[SunLight :: Input :: KEY_T] = ::KEY_T;
            t[SunLight :: Input :: KEY_U] = ::KEY_U;
            t[SunLight :: Input :: KEY_V] = ::KEY_V;
            t[SunLight :: Input :: KEY_W] = ::KEY_W;
            t[SunLight :: Input :: KEY_X] = ::KEY_X;
            t[SunLight :: Input :: KEY_Y] = ::KEY_Y;
            t[SunLight :: Input :: KEY_Z] = ::KEY_Z;
            t[SunLight :: Input :: KEY_LEFT_BRACKET] = ::KEY_LEFT_BRACKET;
            t[SunLight :: Input :: KEY_BACKSLASH] = ::KEY_BACKSLASH;
            t[SunLight :: Input :: KEY_RIGHT_BRACKET] = ::KEY_RIGHT_BRACKET;
            t[SunLight :: Input :: KEY_GRAVE] = ::KEY_GRAVE;
            t[SunLight :: Input :: KEY_SPACE] = ::KEY_SPACE;
            t[SunLight :: Input :: KEY_ESCAPE] = ::KEY_ESCAPE;
            t[SunLight :: Input :: KEY_ENTER] = ::KEY_ENTER;
            t[SunLight :: Input :: KEY_TAB] = ::KEY_TAB;
            t[SunLight :: Input :: KEY_BACKSPACE] = ::KEY_BACKSPACE;
            t[SunLight :: Input :: KEY_INSERT] = ::KEY_INSERT;
            t[SunLight :: Input :: KEY_DELETE] = ::KEY_DELETE;
            t[SunLight :: Input :: KEY_RIGHT] = ::KEY_RIGHT;
            t[SunLight :: Input :: KEY_LEFT] = ::KEY_LEFT;
            t[SunLight :: Input :: KEY_DOWN] = ::KEY_DOWN;
            t[SunLight :: Input :: KEY_UP] = ::KEY_UP;
            t[SunLight :: Input :: KEY_PAGE_UP] = ::KEY_PAGE_UP;
            t[SunLight :: Input :: KEY_PAGE_DOWN] = ::KEY_PAGE_DOWN;
            t[SunLight :: Input :: KEY_HOME] = ::KEY_HOME;
            t[SunLight :: Input :: KEY_END] = ::KEY_END;
            t[SunLight :: Input :: KEY_CAPS_LOCK] = ::KEY_CAPS_LOCK;
            t[SunLight :: Input :: KEY_SCROLL_LOCK] = ::KEY_SCROLL_LOCK;
            t[SunLight :: Input :: KEY_NUM_LOCK] = ::KEY_NUM_LOCK;
            t[SunLight :: Input :: KEY_PRINT_SCREEN] = ::KEY_PRINT_SCREEN;
            t[SunLight :: Input :: KEY_PAUSE] = ::KEY_PAUSE;
            t[SunLight :: Input :: KEY_F1] = ::KEY_F1;
            t[SunLight :: Input :: KEY_F2] = ::KEY_F2;
            t[SunLight :: Input :: KEY_F3] = ::KEY_F3;
            t[SunLight :: Input :: KEY_F4] = ::KEY_F4;
            t[SunLight :: Input :: KEY_F5] = ::KEY_F5;
            t[SunLight :: Input :: KEY_F6] = ::KEY_F6;
            t[SunLight :: Input :: KEY_F7] = ::KEY_F7;
            t[SunLight :: Input :: KEY_F8] = ::KEY_F8;
            t[SunLight :: Input :: KEY_F9] = ::KEY_F9;
            t[SunLight :: Input :: KEY_F10] = ::KEY_F10;
            t[SunLight :: Input :: KEY_F11] = ::KEY_F11;
            t[SunLight :: Input :: KEY_F12] = ::KEY_F12;
            t[SunLight :: Input :: KEY_LEFT_SHIFT] = ::KEY_LEFT_SHIFT;
            t[SunLight :: Input :: KEY_LEFT_CONTROL] = ::KEY_LEFT_CONTROL;
            t[SunLight :: Input :: KEY_LEFT_ALT] = ::KEY_LEFT_ALT;
            t[SunLight :: Input :: KEY_LEFT_SUPER] = ::KEY_LEFT_SUPER;
            t[SunLight :: Input :: KEY_RIGHT_SHIFT] = ::KEY_RIGHT_SHIFT;
            t[SunLight :: Input :: KEY_RIGHT_CONTROL] = ::KEY_RIGHT_CONTROL;
            t[SunLight :: Input :: KEY_RIGHT_ALT] = ::KEY_RIGHT_ALT;
            t[SunLight :: Input :: KEY_RIGHT_SUPER] = ::KEY_RIGHT_SUPER;
            t[SunLight :: Input :: KEY_KB_MENU] = ::KEY_KB_MENU;
            t[SunLight :: Input :: KEY_KP_0] = ::KEY_KP_0;
            t[SunLight :: Input :: KEY_KP_1] = ::KEY_KP_1;
            t[SunLight :: Input :: KEY_KP_2] = ::KEY_KP_2;
            t[SunLight :: Input :: KEY_KP_3] = ::KEY_KP_3;
            t[SunLight :: Input :: KEY_KP_4] = ::KEY_KP_4;
            t[SunLight :: Input :: KEY_KP_5] = ::KEY_KP_5;
            t[SunLight :: Input :: KEY_KP_6] = ::KEY_KP_6;
            t[SunLight :: Input :: KEY_KP_7] = ::KEY_KP_7;
            t[SunLight :: Input :: KEY_KP_8] = ::KEY_KP_8;
            t[SunLight :: Input :: KEY_KP_9] = ::KEY_KP_9;
            t[SunLight :: Input :: KEY_KP_DECIMAL] = ::KEY_KP_DECIMAL;
            t[SunLight :: Input :: KEY_KP_DIVIDE] = ::KEY_KP_DIVIDE;
            t[SunLight :: Input :: KEY_KP_MULTIPLY] = ::KEY_KP_MULTIPLY;
            t[SunLight :: Input :: KEY_KP_SUBTRACT] = ::KEY_KP_SUBTRACT;
            t[SunLight :: Input :: KEY_KP_ADD] = ::KEY_KP_ADD;
            t[SunLight :: Input :: KEY_KP_ENTER] = ::KEY_KP_ENTER;
            t[SunLight :: Input :: KEY_KP_EQUAL] = ::KEY_KP_EQUAL;
            t[SunLight :: Input :: KEY_BACK] = ::KEY_BACK;
            t[SunLight :: Input :: KEY_MENU] = ::KEY_MENU;
            t[SunLight :: Input :: KEY_VOLUME_UP] = ::KEY_VOLUME_UP;
            t[SunLight :: Input :: KEY_VOLUME_DOWN] = ::KEY_VOLUME_DOWN;
            return t;
        }();

        constexpr std :: array<int, 18> kRaylibPadButtons = []  {
            std :: array<int, 18> t {};
            t[SunLight :: Input :: GAMEPAD_BUTTON_UNKNOWN] = ::GAMEPAD_BUTTON_UNKNOWN;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_UP] = ::GAMEPAD_BUTTON_LEFT_FACE_UP;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = ::GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_DOWN] = ::GAMEPAD_BUTTON_LEFT_FACE_DOWN;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_LEFT] = ::GAMEPAD_BUTTON_LEFT_FACE_LEFT;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_UP] = ::GAMEPAD_BUTTON_RIGHT_FACE_UP;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT] = ::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN] = ::GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT] = ::GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_1] = ::GAMEPAD_BUTTON_LEFT_TRIGGER_1;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_TRIGGER_2] = ::GAMEPAD_BUTTON_LEFT_TRIGGER_2;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_1] = ::GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_TRIGGER_2] = ::GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
            t[SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_LEFT] = ::GAMEPAD_BUTTON_MIDDLE_LEFT;
            t[SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE] = ::GAMEPAD_BUTTON_MIDDLE;
            t[SunLight :: Input :: GAMEPAD_BUTTON_MIDDLE_RIGHT] = ::GAMEPAD_BUTTON_MIDDLE_RIGHT;
            t[SunLight :: Input :: GAMEPAD_BUTTON_LEFT_THUMB] = ::GAMEPAD_BUTTON_LEFT_THUMB;
            t[SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_THUMB] = ::GAMEPAD_BUTTON_RIGHT_THUMB;
            return t;
        }();

        constexpr std :: array<int, 6> kRaylibPadAxes = []  {
            std :: array<int, 6> t {};
            t[SunLight :: Input :: GAMEPAD_AXIS_LEFT_X] = ::GAMEPAD_AXIS_LEFT_X;
            t[SunLight :: Input :: GAMEPAD_AXIS_LEFT_Y] = ::GAMEPAD_AXIS_LEFT_Y;
            t[SunLight :: Input :: GAMEPAD_AXIS_RIGHT_X] = ::GAMEPAD_AXIS_RIGHT_X;
            t[SunLight :: Input :: GAMEPAD_AXIS_RIGHT_Y] = ::GAMEPAD_AXIS_RIGHT_Y;
            t[SunLight :: Input :: GAMEPAD_AXIS_LEFT_TRIGGER] = ::GAMEPAD_AXIS_LEFT_TRIGGER;
            t[SunLight :: Input :: GAMEPAD_AXIS_RIGHT_TRIGGER] = ::GAMEPAD_AXIS_RIGHT_TRIGGER;
            return t;
        }();
}


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

                return ( SunLight :: Input :: KeyboardKey ) FromRaylib( kRaylibKeys, ::GetKeyPressed() );
            }

            /**
             * @brief Check if a specified key is on down state. 
             * 
             * @param key Key code to be checked;
             * @return true If is pressed state;
             * @return false  If is not pressed state;
             */
            bool RayLibInputHandler :: IsKeyDown( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyDown( ToRaylib( kRaylibKeys, key ) );
            }

            /**
             * @brief Check if a specified key is on up state. 
             * 
             * @param key Key code to be checked;
             * @return true If is up state;
             * @return false  If is not up state;
             */
            bool RayLibInputHandler :: IsKeyUp( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyUp( ToRaylib( kRaylibKeys, key ) );
            }

            /**
             * @brief Check if a specified key was released;
             * 
             * @param key 
             * @return true 
             * @return false 
             */
            bool RayLibInputHandler :: IsKeyReleased( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyReleased( ToRaylib( kRaylibKeys, key ) );
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

                return ::IsGamepadButtonDown( nGamePadId, ( ::GamepadButton ) ToRaylib( kRaylibPadButtons, button ) );
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

                return ::IsGamepadButtonUp( nGamePadId, ( ::GamepadButton ) ToRaylib( kRaylibPadButtons, button ) );
            }

            /**
             * @brief Get the Gamepad index when the related GamePad Mid Button
             * is pressed; 
             * 
             * @return int The Gamepad axis index which was pressed;
             */
            int RayLibInputHandler :: GetGamepadButtonPressed( void )  {

                return FromRaylib( kRaylibPadButtons, ::GetGamepadButtonPressed() );
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

                return ::GetGamepadAxisMovement( nGamePadId, ( ::GamepadAxis ) ToRaylib( kRaylibPadAxes, axis ) );
            }
        }
    }
}