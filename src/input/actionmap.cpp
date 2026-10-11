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

#include "input/actionmap.h"
#include <fstream>
#include <sstream>

namespace SunLight  {
    namespace Input  {

        namespace  {

            // The file format's own type tags (F3 of the master plan): one line per binding,
            // "Action=Tag:code" or, for an axis, "Action=Tag:code:direction".
            const char* const  __TAG_KEY             = "KEY";
            const char* const  __TAG_GAMEPAD_BUTTON   = "BUTTON";
            const char* const  __TAG_GAMEPAD_AXIS     = "AXIS";
            const char         __FIELD_SEPARATOR      = ':';
            const char         __COMMENT_PREFIX       = '#';

            const char* TagForType( SunLight :: Input :: ActionBindingType type )  {

                switch( type )  {
                    case SunLight :: Input :: ACTION_BINDING_KEY             : return __TAG_KEY;
                    case SunLight :: Input :: ACTION_BINDING_GAMEPAD_BUTTON  : return __TAG_GAMEPAD_BUTTON;
                    case SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS    : return __TAG_GAMEPAD_AXIS;
                    default                                                  : return nullptr;
                }
            }
        }

        std :: string ActionMap :: FindActionBoundTo( SunLight :: Input :: ActionBindingType type, int nCode, int nDirection ) const  {

            for( const std :: pair<const std :: string, SunLight :: Input :: stActionBinding>& entry : m_Bindings )  {
                const SunLight :: Input :: stActionBinding&  binding = entry.second;

                if( ( binding.type == type ) && ( binding.nCode == nCode )
                    && ( ( type != SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS ) || ( binding.nDirection == nDirection ) ) )
                    return entry.first;
            }

            return std :: string();
        }

        bool ActionMap :: BindKey( const std :: string& strAction, SunLight :: Input :: KeyboardKey key )  {

            std :: string  strConflict = FindActionBoundTo( SunLight :: Input :: ACTION_BINDING_KEY, ( int ) key, 0 );

            if( !strConflict.empty() && ( strConflict != strAction ) )
                return false;

            m_Bindings[ strAction ] = { SunLight :: Input :: ACTION_BINDING_KEY, ( int ) key, 0 };

            return true;
        }

        bool ActionMap :: BindGamepadButton( const std :: string& strAction, SunLight :: Input :: GamepadButton button )  {

            std :: string  strConflict = FindActionBoundTo( SunLight :: Input :: ACTION_BINDING_GAMEPAD_BUTTON, ( int ) button, 0 );

            if( !strConflict.empty() && ( strConflict != strAction ) )
                return false;

            m_Bindings[ strAction ] = { SunLight :: Input :: ACTION_BINDING_GAMEPAD_BUTTON, ( int ) button, 0 };

            return true;
        }

        bool ActionMap :: BindGamepadAxis( const std :: string& strAction, SunLight :: Input :: GamepadAxis axis, int nDirection )  {

            std :: string  strConflict = FindActionBoundTo( SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS, ( int ) axis, nDirection );

            if( !strConflict.empty() && ( strConflict != strAction ) )
                return false;

            m_Bindings[ strAction ] = { SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS, ( int ) axis, nDirection };

            return true;
        }

        void ActionMap :: Unbind( const std :: string& strAction )  {

            m_Bindings.erase( strAction );
        }

        SunLight :: Input :: stActionBinding ActionMap :: GetBinding( const std :: string& strAction ) const  {

            std :: map<std :: string, SunLight :: Input :: stActionBinding> :: const_iterator  itItem = m_Bindings.find( strAction );

            return ( itItem != m_Bindings.end() ) ? itItem -> second : SunLight :: Input :: stActionBinding();
        }

        bool ActionMap :: IsActionActive( const std :: string& strAction, SunLight :: Input :: IInputHandler& input,
                                          int nGamePadId, float fAxisThreshold ) const  {

            std :: map<std :: string, SunLight :: Input :: stActionBinding> :: const_iterator  itItem = m_Bindings.find( strAction );

            if( itItem == m_Bindings.end() )
                return false;

            const SunLight :: Input :: stActionBinding&  binding = itItem -> second;

            switch( binding.type )  {
                case SunLight :: Input :: ACTION_BINDING_KEY :
                    return input.IsKeyDown( ( SunLight :: Input :: KeyboardKey ) binding.nCode );

                case SunLight :: Input :: ACTION_BINDING_GAMEPAD_BUTTON :
                    return input.IsGamepadButtonDown( nGamePadId, ( SunLight :: Input :: GamepadButton ) binding.nCode );

                case SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS :  {
                    float  fValue = input.GetGamepadAxisMovement( nGamePadId, ( SunLight :: Input :: GamepadAxis ) binding.nCode );

                    return ( binding.nDirection > 0 ) ? ( fValue >= fAxisThreshold ) : ( fValue <= -fAxisThreshold );
                }

                default :
                    return false;
            }
        }

        bool ActionMap :: Save( const std :: string& strPath ) const  {

            std :: ofstream  file( strPath );

            if( !file.is_open() )
                return false;

            file << __COMMENT_PREFIX << " sunlight action map\n";

            for( const std :: pair<const std :: string, SunLight :: Input :: stActionBinding>& entry : m_Bindings )  {
                const SunLight :: Input :: stActionBinding&  binding = entry.second;
                const char                                  *szTag  = TagForType( binding.type );

                if( szTag == nullptr )
                    continue;

                file << entry.first << '=' << szTag << __FIELD_SEPARATOR << binding.nCode;

                if( binding.type == SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS )
                    file << __FIELD_SEPARATOR << binding.nDirection;

                file << '\n';
            }

            return true;
        }

        // Parses one non-comment, non-blank line into an action name and its binding. Returns
        // false, leaving outAction/outBinding untouched, for anything malformed - Load() treats
        // that as the whole file failing, never a partial load.
        namespace  {

            bool ParseLine( const std :: string& strLine, std :: string& strOutAction, SunLight :: Input :: stActionBinding& outBinding )  {

                size_t  nEquals = strLine.find( '=' );

                if( nEquals == std :: string :: npos )
                    return false;

                std :: string        strAction = strLine.substr( 0, nEquals );
                std :: string        strRest   = strLine.substr( nEquals + 1 );
                std :: istringstream stream( strRest );
                std :: string        strTag;
                std :: string        strCode;

                if( !std :: getline( stream, strTag, __FIELD_SEPARATOR ) || !std :: getline( stream, strCode, __FIELD_SEPARATOR ) )
                    return false;

                SunLight :: Input :: stActionBinding  binding;

                try  {
                    binding.nCode = std :: stoi( strCode );
                }
                catch( ... )  {
                    return false;
                }

                if( strTag == __TAG_KEY )
                    binding.type = SunLight :: Input :: ACTION_BINDING_KEY;
                else if( strTag == __TAG_GAMEPAD_BUTTON )
                    binding.type = SunLight :: Input :: ACTION_BINDING_GAMEPAD_BUTTON;
                else if( strTag == __TAG_GAMEPAD_AXIS )  {
                    std :: string  strDirection;

                    if( !std :: getline( stream, strDirection, __FIELD_SEPARATOR ) )
                        return false;

                    try  {
                        binding.nDirection = std :: stoi( strDirection );
                    }
                    catch( ... )  {
                        return false;
                    }

                    binding.type = SunLight :: Input :: ACTION_BINDING_GAMEPAD_AXIS;
                }
                else
                    return false;

                strOutAction = strAction;
                outBinding   = binding;

                return true;
            }
        }

        bool ActionMap :: Load( const std :: string& strPath )  {

            std :: ifstream  file( strPath );

            if( !file.is_open() )
                return false;

            std :: map<std :: string, SunLight :: Input :: stActionBinding>  newBindings;
            std :: string                                                    strLine;

            while( std :: getline( file, strLine ) )  {
                if( strLine.empty() || ( strLine[ 0 ] == __COMMENT_PREFIX ) )
                    continue;

                std :: string                         strAction;
                SunLight :: Input :: stActionBinding  binding;

                if( !ParseLine( strLine, strAction, binding ) )
                    return false;

                newBindings[ strAction ] = binding;
            }

            m_Bindings = std :: move( newBindings );

            return true;
        }
    }
}
