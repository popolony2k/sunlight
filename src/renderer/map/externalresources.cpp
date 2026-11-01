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

#include "renderer/map/externalresources.h"
#include "filesystem/filesystemfactory.h"
#include "filesystem/ifilesystem.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace SunLight  {
    namespace Renderer  {
        namespace ExternalResources  {

            namespace  {

                // Set only while an external file is being preloaded: the directory it lives in.
                std :: string  s_strExternalImageBase;

                /**
                 * The value of attribute szName inside one tag's text (everything between '<' and
                 * '>'), for either quote style, or false if it has no such attribute.
                 */
                bool FindTagAttribute( const std :: string& strTag, const char* szName, std :: string& strValue )  {

                    size_t  nNameLength = strlen( szName );
                    size_t  nPos        = 0;

                    while( ( nPos = strTag.find( szName, nPos ) ) != std :: string :: npos )  {
                        // A whole attribute name: preceded by white space, followed by '='.
                        bool    bBoundary = ( nPos > 0 ) && isspace( ( unsigned char ) strTag[nPos - 1] );
                        size_t  nCursor   = nPos + nNameLength;

                        while( nCursor < strTag.size() && isspace( ( unsigned char ) strTag[nCursor] ) )
                            nCursor++;

                        if( bBoundary && nCursor < strTag.size() && strTag[nCursor] == '=' )  {
                            nCursor++;

                            while( nCursor < strTag.size() && isspace( ( unsigned char ) strTag[nCursor] ) )
                                nCursor++;

                            if( nCursor < strTag.size() && ( strTag[nCursor] == '"' || strTag[nCursor] == '\'' ) )  {
                                size_t  nClose = strTag.find( strTag[nCursor], nCursor + 1 );

                                if( nClose != std :: string :: npos )  {
                                    strValue = strTag.substr( nCursor + 1, nClose - nCursor - 1 );

                                    return true;
                                }
                            }
                        }

                        nPos += nNameLength;
                    }

                    return false;
            }

            /**
             * Whether a tag's text starts with the element name szElement (and nothing longer).
             */
            bool TagIsElement( const std :: string& strTag, const char* szElement )  {

                size_t  nLength = strlen( szElement );

                return ( strTag.compare( 0, nLength, szElement ) == 0 ) &&
                       ( strTag.size() == nLength || isspace( ( unsigned char ) strTag[nLength] ) || strTag[nLength] == '/' );
            }
            }

            namespace  {

                const size_t  __DRIVE_LENGTH = 2;     // a Windows drive segment: a letter and ':' ("C:")

                // The scheme a path starts with, as written ("vpath://", "hpath://", ...; "" for none or an
                // unknown one), and the path after it (I12 of the master plan: the path helpers work on
                // that rest only, and put the scheme back, so "//" is never mistaken for an empty segment).
                std :: string SchemeOf( const std :: string& strPath, std :: string& strRest, bool& bHost )  {

                    SunLight :: FileSystem :: IFileSystem :: PathScheme  scheme = SunLight :: FileSystem :: IFileSystem :: SplitPath( strPath, strRest );

                    bHost = ( scheme == SunLight :: FileSystem :: IFileSystem :: PATH_SCHEME_HOST );

                    if( scheme == SunLight :: FileSystem :: IFileSystem :: PATH_SCHEME_UNKNOWN )
                        strRest = strPath;

                    return strPath.substr( 0, strPath.size() - strRest.size() );
                }

                // A Windows drive segment ("C:"), which only a host path can start with.
                bool IsDrive( const std :: string& strPart )  {

                    return ( strPart.size() == __DRIVE_LENGTH ) && isalpha( ( unsigned char ) strPart[0] ) && ( strPart[1] == ':' );
                }

                // backslashes become '/', and "." and ".." are collapsed LEXICALLY; "" when ".." would climb
                // above the root (or above a host path's drive).
                std :: string CollapsePath( const std :: string& strPath, bool bHost )  {

                    std :: string  joined = strPath;

                    std :: replace( joined.begin(), joined.end(), '\\', '/' );

                    bool                          bAbsolute = ( !joined.empty() && joined[0] == '/' );
                    bool                          bDrive    = false;
                    std :: vector<std :: string>  parts;
                    size_t                        nStart = 0;

                    while( nStart <= joined.size() )  {
                        size_t         nEnd  = joined.find( '/', nStart );
                        std :: string  part;

                        if( nEnd == std :: string :: npos )
                            nEnd = joined.size();

                        part   = joined.substr( nStart, nEnd - nStart );
                        nStart = nEnd + 1;

                        if( part.empty() || part == "." )
                            continue;

                        if( part == ".." )  {
                            if( parts.empty() || ( bDrive && ( parts.size() == 1 ) ) )
                                return std :: string();

                            parts.pop_back();
                            continue;
                        }

                        if( bHost && parts.empty() && !bAbsolute && IsDrive( part ) )
                            bDrive = true;

                        parts.push_back( part );
                    }

                    std :: string  result = ( bAbsolute ? "/" : "" );

                    for( size_t nCount = 0; nCount < parts.size(); nCount++ )
                        result += ( nCount ? "/" : "" ) + parts[nCount];

                    return result;
                }
            }

            /**
             * Directory part of a path, trailing '/' included ("" if it has none). A scheme stays in front
             * (I12 of the master plan): "vpath://maps/level.tmx" gives "vpath://maps/", and
             * "vpath://level.tmx" gives "vpath://", so a join keeps the scheme.
             */
            std :: string DirectoryOfVirtualPath( const std :: string& strPath )  {

                std :: string  strRest;
                bool           bHost;
                std :: string  strScheme = SchemeOf( strPath, strRest, bHost );

                std :: replace( strRest.begin(), strRest.end(), '\\', '/' );

                size_t  nSlash = strRest.find_last_of( '/' );

                return strScheme + ( ( nSlash == std :: string :: npos ) ? std :: string() : strRest.substr( 0, nSlash + 1 ) );
            }

            /**
             * strDir + strRelative as one path: backslashes become '/', and "." and ".." segments are
             * collapsed LEXICALLY (the FileSystem rejects them). A ".." that would climb above the root gives
             * "" - not a legal path. The scheme of strDir stays in front and the collapse works on the rest
             * only (I12 of the master plan: it used to drop the "//" of "vpath://" as an empty segment, so a
             * map loaded through a scheme could not find its tilesets or images - found by Scarab); a host
             * path keeps the OS's rules: absolute stays absolute, and ".." never climbs above a drive
             * ("C:"). A strRelative with a scheme of its own already says where it lives: it is used alone.
             */
            std :: string JoinVirtualPath( const std :: string& strDir, const std :: string& strRelative )  {

                std :: string  strRest;
                bool           bHost;
                std :: string  strScheme = SchemeOf( strRelative, strRest, bHost );
                std :: string  strJoined;

                if( strScheme.empty() )  {
                    strScheme  = SchemeOf( strDir, strJoined, bHost );
                    strJoined += strRelative;
                }
                else
                    strJoined = strRest;

                std :: string  strCollapsed = CollapsePath( strJoined, bHost );

                return strCollapsed.empty() ? std :: string() : ( strScheme + strCollapsed );
            }

            /**
             * Collect every external reference in an XML document: a `<tileset source="...">`, and an
             * `<object template="...">`. (A plain scan for those two tags - the documents are Tiled's own
             * output; anything it misses is left for libtmx's own lookup, anything extra is harmless.)
             */
            void ScanExternalReferences( const std :: string& strXml, std :: vector<ExternalReference>& references )  {

                size_t  nPos = 0;

                while( ( nPos = strXml.find( '<', nPos ) ) != std :: string :: npos )  {
                    size_t  nEnd = strXml.find( '>', nPos );

                    if( nEnd == std :: string :: npos )
                        break;

                    std :: string  strTag = strXml.substr( nPos + 1, nEnd - nPos - 1 );
                    std :: string  strValue;

                    if( TagIsElement( strTag, "tileset" ) )  {
                        if( FindTagAttribute( strTag, "source", strValue ) && !strValue.empty() )
                            references.push_back( ExternalReference { false, strValue } );
                    }
                    else if( TagIsElement( strTag, "object" ) )  {
                        if( FindTagAttribute( strTag, "template", strValue ) && !strValue.empty() )
                            references.push_back( ExternalReference { true, strValue } );
                    }

                    nPos = nEnd + 1;
                }
            }

            /**
             * Read one external tileset/template through the FileSystem and give it to libtmx's
             * Resource Manager under its raw attribute text. The things IT references are loaded first
             * (a template's tileset must be known before the template is parsed), each resolved against
             * the directory of the file that names it. A file the FileSystem doesn't have is skipped.
             */
            void PreloadExternalResource( tmx_resource_manager* pRcMgr,
                                          const ExternalReference& reference,
                                          const std :: string& strReferencingDir,
                                          std :: map<std :: string, std :: string>& loadedKeys,
                                          std :: set<std :: string>& visited )  {

                std :: string  strPath = JoinVirtualPath( strReferencingDir, reference.strKey );

                if( strPath.empty() )
                    return;

                // Each file once (also stops a reference cycle); and libtmx caches by the RAW key, so a
                // second file under an already-provided key would only replace what maps already point at.
                if( !visited.insert( ( reference.bTemplate ? "T:" : "S:" ) + strPath ).second )
                    return;

                if( loadedKeys.count( reference.strKey ) )
                    return;

                std :: vector<unsigned char>  data;

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strPath, data ) )
                    return;

                std :: string                     strText( data.begin(), data.end() );
                std :: string                     strDir = DirectoryOfVirtualPath( strPath );
                std :: vector<ExternalReference>  nested;

                ScanExternalReferences( strText, nested );

                for( const ExternalReference& inner : nested )
                    PreloadExternalResource( pRcMgr, inner, strDir, loadedKeys, visited );

                // The images this file names are relative to ITS directory; libtmx hands them to the
                // texture callback as written, so tell the callback where they live.
                s_strExternalImageBase = strDir;

                int  nLoaded = reference.bTemplate ?
                               ::tmx_load_template_buffer( pRcMgr, ( const char * ) data.data(), ( int ) data.size(), reference.strKey.c_str() ) :
                               ::tmx_load_tileset_buffer( pRcMgr, ( const char * ) data.data(), ( int ) data.size(), reference.strKey.c_str() );

                s_strExternalImageBase.clear();

                if( nLoaded )
                    loadedKeys[reference.strKey] = strPath;
            }

            std :: string ImageBase( void )  {
                return s_strExternalImageBase;
            }
        }
    }
}
