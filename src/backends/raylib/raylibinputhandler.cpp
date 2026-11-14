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
#include "backends/raylib/raylibcodemap.h"
#include "input/gamepadinputfactory.h"
#include <raylib.h>
#include <array>


#define __KEY_CODE_COUNT               349        // one past the highest key code (KEY_KB_MENU)


namespace  {

    // The pairs: one line per key. Both directions are generated from these.
    constexpr SunLight :: Input :: RayLib :: CodePair kKeyPairs[] = {
        { SunLight :: Input :: KEY_NULL, ::KEY_NULL },
        { SunLight :: Input :: KEY_APOSTROPHE, ::KEY_APOSTROPHE },
        { SunLight :: Input :: KEY_COMMA, ::KEY_COMMA },
        { SunLight :: Input :: KEY_MINUS, ::KEY_MINUS },
        { SunLight :: Input :: KEY_PERIOD, ::KEY_PERIOD },
        { SunLight :: Input :: KEY_SLASH, ::KEY_SLASH },
        { SunLight :: Input :: KEY_ZERO, ::KEY_ZERO },
        { SunLight :: Input :: KEY_ONE, ::KEY_ONE },
        { SunLight :: Input :: KEY_TWO, ::KEY_TWO },
        { SunLight :: Input :: KEY_THREE, ::KEY_THREE },
        { SunLight :: Input :: KEY_FOUR, ::KEY_FOUR },
        { SunLight :: Input :: KEY_FIVE, ::KEY_FIVE },
        { SunLight :: Input :: KEY_SIX, ::KEY_SIX },
        { SunLight :: Input :: KEY_SEVEN, ::KEY_SEVEN },
        { SunLight :: Input :: KEY_EIGHT, ::KEY_EIGHT },
        { SunLight :: Input :: KEY_NINE, ::KEY_NINE },
        { SunLight :: Input :: KEY_SEMICOLON, ::KEY_SEMICOLON },
        { SunLight :: Input :: KEY_EQUAL, ::KEY_EQUAL },
        { SunLight :: Input :: KEY_A, ::KEY_A },
        { SunLight :: Input :: KEY_B, ::KEY_B },
        { SunLight :: Input :: KEY_C, ::KEY_C },
        { SunLight :: Input :: KEY_D, ::KEY_D },
        { SunLight :: Input :: KEY_E, ::KEY_E },
        { SunLight :: Input :: KEY_F, ::KEY_F },
        { SunLight :: Input :: KEY_G, ::KEY_G },
        { SunLight :: Input :: KEY_H, ::KEY_H },
        { SunLight :: Input :: KEY_I, ::KEY_I },
        { SunLight :: Input :: KEY_J, ::KEY_J },
        { SunLight :: Input :: KEY_K, ::KEY_K },
        { SunLight :: Input :: KEY_L, ::KEY_L },
        { SunLight :: Input :: KEY_M, ::KEY_M },
        { SunLight :: Input :: KEY_N, ::KEY_N },
        { SunLight :: Input :: KEY_O, ::KEY_O },
        { SunLight :: Input :: KEY_P, ::KEY_P },
        { SunLight :: Input :: KEY_Q, ::KEY_Q },
        { SunLight :: Input :: KEY_R, ::KEY_R },
        { SunLight :: Input :: KEY_S, ::KEY_S },
        { SunLight :: Input :: KEY_T, ::KEY_T },
        { SunLight :: Input :: KEY_U, ::KEY_U },
        { SunLight :: Input :: KEY_V, ::KEY_V },
        { SunLight :: Input :: KEY_W, ::KEY_W },
        { SunLight :: Input :: KEY_X, ::KEY_X },
        { SunLight :: Input :: KEY_Y, ::KEY_Y },
        { SunLight :: Input :: KEY_Z, ::KEY_Z },
        { SunLight :: Input :: KEY_LEFT_BRACKET, ::KEY_LEFT_BRACKET },
        { SunLight :: Input :: KEY_BACKSLASH, ::KEY_BACKSLASH },
        { SunLight :: Input :: KEY_RIGHT_BRACKET, ::KEY_RIGHT_BRACKET },
        { SunLight :: Input :: KEY_GRAVE, ::KEY_GRAVE },
        { SunLight :: Input :: KEY_SPACE, ::KEY_SPACE },
        { SunLight :: Input :: KEY_ESCAPE, ::KEY_ESCAPE },
        { SunLight :: Input :: KEY_ENTER, ::KEY_ENTER },
        { SunLight :: Input :: KEY_TAB, ::KEY_TAB },
        { SunLight :: Input :: KEY_BACKSPACE, ::KEY_BACKSPACE },
        { SunLight :: Input :: KEY_INSERT, ::KEY_INSERT },
        { SunLight :: Input :: KEY_DELETE, ::KEY_DELETE },
        { SunLight :: Input :: KEY_RIGHT, ::KEY_RIGHT },
        { SunLight :: Input :: KEY_LEFT, ::KEY_LEFT },
        { SunLight :: Input :: KEY_DOWN, ::KEY_DOWN },
        { SunLight :: Input :: KEY_UP, ::KEY_UP },
        { SunLight :: Input :: KEY_PAGE_UP, ::KEY_PAGE_UP },
        { SunLight :: Input :: KEY_PAGE_DOWN, ::KEY_PAGE_DOWN },
        { SunLight :: Input :: KEY_HOME, ::KEY_HOME },
        { SunLight :: Input :: KEY_END, ::KEY_END },
        { SunLight :: Input :: KEY_CAPS_LOCK, ::KEY_CAPS_LOCK },
        { SunLight :: Input :: KEY_SCROLL_LOCK, ::KEY_SCROLL_LOCK },
        { SunLight :: Input :: KEY_NUM_LOCK, ::KEY_NUM_LOCK },
        { SunLight :: Input :: KEY_PRINT_SCREEN, ::KEY_PRINT_SCREEN },
        { SunLight :: Input :: KEY_PAUSE, ::KEY_PAUSE },
        { SunLight :: Input :: KEY_F1, ::KEY_F1 },
        { SunLight :: Input :: KEY_F2, ::KEY_F2 },
        { SunLight :: Input :: KEY_F3, ::KEY_F3 },
        { SunLight :: Input :: KEY_F4, ::KEY_F4 },
        { SunLight :: Input :: KEY_F5, ::KEY_F5 },
        { SunLight :: Input :: KEY_F6, ::KEY_F6 },
        { SunLight :: Input :: KEY_F7, ::KEY_F7 },
        { SunLight :: Input :: KEY_F8, ::KEY_F8 },
        { SunLight :: Input :: KEY_F9, ::KEY_F9 },
        { SunLight :: Input :: KEY_F10, ::KEY_F10 },
        { SunLight :: Input :: KEY_F11, ::KEY_F11 },
        { SunLight :: Input :: KEY_F12, ::KEY_F12 },
        { SunLight :: Input :: KEY_LEFT_SHIFT, ::KEY_LEFT_SHIFT },
        { SunLight :: Input :: KEY_LEFT_CONTROL, ::KEY_LEFT_CONTROL },
        { SunLight :: Input :: KEY_LEFT_ALT, ::KEY_LEFT_ALT },
        { SunLight :: Input :: KEY_LEFT_SUPER, ::KEY_LEFT_SUPER },
        { SunLight :: Input :: KEY_RIGHT_SHIFT, ::KEY_RIGHT_SHIFT },
        { SunLight :: Input :: KEY_RIGHT_CONTROL, ::KEY_RIGHT_CONTROL },
        { SunLight :: Input :: KEY_RIGHT_ALT, ::KEY_RIGHT_ALT },
        { SunLight :: Input :: KEY_RIGHT_SUPER, ::KEY_RIGHT_SUPER },
        { SunLight :: Input :: KEY_KB_MENU, ::KEY_KB_MENU },
        { SunLight :: Input :: KEY_KP_0, ::KEY_KP_0 },
        { SunLight :: Input :: KEY_KP_1, ::KEY_KP_1 },
        { SunLight :: Input :: KEY_KP_2, ::KEY_KP_2 },
        { SunLight :: Input :: KEY_KP_3, ::KEY_KP_3 },
        { SunLight :: Input :: KEY_KP_4, ::KEY_KP_4 },
        { SunLight :: Input :: KEY_KP_5, ::KEY_KP_5 },
        { SunLight :: Input :: KEY_KP_6, ::KEY_KP_6 },
        { SunLight :: Input :: KEY_KP_7, ::KEY_KP_7 },
        { SunLight :: Input :: KEY_KP_8, ::KEY_KP_8 },
        { SunLight :: Input :: KEY_KP_9, ::KEY_KP_9 },
        { SunLight :: Input :: KEY_KP_DECIMAL, ::KEY_KP_DECIMAL },
        { SunLight :: Input :: KEY_KP_DIVIDE, ::KEY_KP_DIVIDE },
        { SunLight :: Input :: KEY_KP_MULTIPLY, ::KEY_KP_MULTIPLY },
        { SunLight :: Input :: KEY_KP_SUBTRACT, ::KEY_KP_SUBTRACT },
        { SunLight :: Input :: KEY_KP_ADD, ::KEY_KP_ADD },
        { SunLight :: Input :: KEY_KP_ENTER, ::KEY_KP_ENTER },
        { SunLight :: Input :: KEY_KP_EQUAL, ::KEY_KP_EQUAL },
        { SunLight :: Input :: KEY_BACK, ::KEY_BACK },
        { SunLight :: Input :: KEY_MENU, ::KEY_MENU },
        { SunLight :: Input :: KEY_VOLUME_UP, ::KEY_VOLUME_UP },
        { SunLight :: Input :: KEY_VOLUME_DOWN, ::KEY_VOLUME_DOWN },
    };

    constexpr SunLight :: Input :: RayLib :: CodeMap<__KEY_CODE_COUNT>  kKeys = SunLight :: Input :: RayLib :: BuildCodeMap<__KEY_CODE_COUNT>( kKeyPairs );
}


namespace SunLight  {
    namespace Input  {
        namespace RayLib  {
            /**
             * @brief Constructor. Initialize all class data. 
             */
            RayLibInputHandler :: RayLibInputHandler( void )
                : m_pGamepads( SunLight :: Input :: GamepadInputFactory :: CreateGamepadInput() )  {

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

                return ( SunLight :: Input :: KeyboardKey ) FromRaylib( kKeys, ::GetKeyPressed() );
            }

            /**
             * @brief Check if a specified key is on down state. 
             * 
             * @param key Key code to be checked;
             * @return true If is pressed state;
             * @return false  If is not pressed state;
             */
            bool RayLibInputHandler :: IsKeyDown( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyDown( ToRaylib( kKeys, key ) );
            }

            /**
             * @brief Check if a specified key is on up state. 
             * 
             * @param key Key code to be checked;
             * @return true If is up state;
             * @return false  If is not up state;
             */
            bool RayLibInputHandler :: IsKeyUp( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyUp( ToRaylib( kKeys, key ) );
            }

            /**
             * @brief Check if a specified key was released;
             * 
             * @param key 
             * @return true 
             * @return false 
             */
            bool RayLibInputHandler :: IsKeyReleased( SunLight :: Input :: KeyboardKey key )  {

                return ::IsKeyReleased( ToRaylib( kKeys, key ) );
            }

            /**
             * @brief Reads the gamepads for the coming frame (see IInputHandler::Update). The
             * keyboard and mouse need nothing: raylib reads them itself, in EndDrawing.
             */
            void RayLibInputHandler :: Update( void )  {

                m_pGamepads -> Update();
            }

            /**
             * @brief The pad's name, from the gamepad backend (see the class's doc comment).
             * @param nGamePadId The game pad id to check;
             */
            const char* RayLibInputHandler :: GetGamepadName( int nGamePadId )  {

                return m_pGamepads -> GetGamepadName( nGamePadId );
            }

            /**
             * @brief Whether the pad is connected, from the gamepad backend.
             * @param nGamePadId The game pad id to check;
             */
            bool RayLibInputHandler :: IsGamepadAvailable( int nGamePadId )  {

                return m_pGamepads -> IsGamepadAvailable( nGamePadId );
            }

            /**
             * @brief Whether the button is down, from the gamepad backend.
             * @param nGamePadId The game pad id to check;
             * @param button Button code to check;
             */
            bool RayLibInputHandler :: IsGamepadButtonDown( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                return m_pGamepads -> IsGamepadButtonDown( nGamePadId, button );
            }

            /**
             * @brief Whether the button is up, from the gamepad backend.
             * @param nGamePadId The game pad id to check;
             * @param button Button code to check;
             */
            bool RayLibInputHandler :: IsGamepadButtonUp( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

                return m_pGamepads -> IsGamepadButtonUp( nGamePadId, button );
            }

            /**
             * @brief The last button held, from the gamepad backend.
             */
            int RayLibInputHandler :: GetGamepadButtonPressed( void )  {

                return m_pGamepads -> GetGamepadButtonPressed();
            }

            /**
             * @brief The pad's axis count, from the gamepad backend.
             * @param nGamePadId The game pad id to check;
             */
            int RayLibInputHandler :: GetGamepadAxisCount( int nGamePadId )  {

                return m_pGamepads -> GetGamepadAxisCount( nGamePadId );
            }

            /**
             * @brief The axis position, from the gamepad backend.
             * @param nGamePadId The game pad id to check;
             * @param axis The axis to check;
             */
            float RayLibInputHandler :: GetGamepadAxisMovement( int nGamePadId, SunLight :: Input :: GamepadAxis axis )  {

                return m_pGamepads -> GetGamepadAxisMovement( nGamePadId, axis );
            }
        }
    }
}
