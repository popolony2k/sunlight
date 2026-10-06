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
                bool FindTagAttribute( const std :: string &strTag, const char *szName, std :: string &strValue )  {

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
            bool TagIsElement( const std :: string &strTag, const char *szElement )  {

                size_t  nLength = strlen( szElement );

                return ( strTag.compare( 0, nLength, szElement ) == 0 ) &&
                       ( strTag.size() == nLength || isspace( ( unsigned char ) strTag[nLength] ) || strTag[nLength] == '/' );
            }
            }

            /**
             * Directory part of a virtual path, trailing '/' included ("" if it has none).
             */
            std :: string DirectoryOfVirtualPath( const std :: string &strPath )  {

                std :: string  str = strPath;

                std :: replace( str.begin(), str.end(), '\\', '/' );

                size_t  nSlash = str.find_last_of( '/' );

                return ( nSlash == std :: string :: npos ) ? std :: string() : str.substr( 0, nSlash + 1 );
            }

            /**
             * strDir + strRelative as one virtual path: backslashes become '/', and "." and ".."
             * segments are collapsed LEXICALLY (the FileSystem rejects them). A ".." that would
             * climb above the root gives "" - not a legal virtual path.
             */
            std :: string JoinVirtualPath( const std :: string &strDir, const std :: string &strRelative )  {

                std :: string  joined = strDir + strRelative;

                std :: replace( joined.begin(), joined.end(), '\\', '/' );

                bool                          bAbsolute = ( !joined.empty() && joined[0] == '/' );
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
                        if( parts.empty() )
                            return std :: string();

                        parts.pop_back();
                        continue;
                    }

                    parts.push_back( part );
                }

                std :: string  result = ( bAbsolute ? "/" : "" );

                for( size_t nCount = 0; nCount < parts.size(); nCount++ )
                    result += ( nCount ? "/" : "" ) + parts[nCount];

                return result;
            }

            /**
             * Collect every external reference in an XML document: a <tileset source="...">, and an
             * <object template="...">. (A plain scan for those two tags - the documents are Tiled's own
             * output; anything it misses is left for libtmx's own lookup, anything extra is harmless.)
             */
            void ScanExternalReferences( const std :: string &strXml, std :: vector<ExternalReference> &references )  {

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
            void PreloadExternalResource( tmx_resource_manager *pRcMgr,
                                          const ExternalReference &reference,
                                          const std :: string &strReferencingDir,
                                          std :: map<std :: string, std :: string> &loadedKeys,
                                          std :: set<std :: string> &visited )  {

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

                for( const ExternalReference &inner : nested )
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
