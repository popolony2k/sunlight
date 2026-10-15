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

#include "renderer/objects/textobjectrenderer.h"
#include "renderer/shape/shapeobjects.h"
#include "engines/enginefactory.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>


#define __DEFAULT_TEXT_PIXEL_SIZE       16      // Tiled's own default when a text object gives no pixelsize
#define __MISSING_FONT_PLACEHOLDER      "??????"  // drawn in place of a text object whose font is not registered


namespace SunLight  {
    namespace Renderer  {

        /**
         * @brief Draw one text object (Tiled's OT_TEXT).
         *
         * The object's box - its x, y, width and height, mapped to the
         * screen the way a shape is mapped, `(position + camera) x zoom +
         * viewport position` - is where its text is laid out: wrapped at the
         * box's width when the object asks for wrap, and aligned as Tiled's
         * halign and valign say. A box with no width or no height has no edge
         * on that side, so nothing is aligned against it. Font size is the
         * object's pixelsize, scaled by zoom.
         *
         * The font is the one registered for this object's fontfamily and
         * bold/italic flags. When there is none, the object's text is NOT
         * drawn: "??????" is drawn in its place, in the object's colour, with
         * the engine's active font, and a warning is logged once for that
         * family and style. The placeholder is deliberate - the mistake shows
         * on screen, not only in the log.
         *
         * An unturned box entirely outside the viewport is skipped. Text that
         * crosses the viewport's edge is cut there by the clip the view's pass
         * already set (the text opens no clip of its own). A turned object
         * (Tiled's rotation) turns its whole block about the box's top-left
         * corner, each line turning with it.
         *
         * @param context The view and layer to draw for;
         * @param pObject The text object to draw;
         * @param pLayer The object group it belongs to (its offset and opacity apply);
         * @param fonts The fonts registered by family and style;
         * @param opacity The per-object opacity rule (and its warnings);
         */
        void TextObjectRenderer :: Draw( const DrawContext& context,
                                         tmx_object* pObject,
                                         tmx_layer* pLayer,
                                         FontRegistry& fonts,
                                         ObjectOpacity& opacity ) const  {

            if( !pObject -> content.text )
                return;

            tmx_text                              *pText    = pObject -> content.text;
            const SunLight :: Base :: stDimension2D  &vp     = context.viewport;
            SunLight :: Base :: stColor            color    = Shape :: ColorOf( pText -> color );
            const char                            *szFamily = ( pText -> fontfamily != nullptr ) ? pText -> fontfamily : "";

            // L2: the object's own "opacity" property, or its layer's, multiplies into the text's
            // own stored colour alpha (not replaced - unlike a tile's tintcolor, a text object's
            // colour has no separate alpha control of its own to begin with).
            color.nAlpha = ( uint8_t ) ( color.nAlpha * opacity.Of( pObject, ( float ) pLayer -> opacity ) );
            bool                                   bBold    = ( pText -> bold != 0 );
            bool                                   bItalic  = ( pText -> italic != 0 );
            double                                 fZoom    = context.fZoom;

            // Same screen mapping as a shape: (position + camera) x zoom + viewport origin.
            double  fBoxX = ( ( pObject -> x + pLayer -> offsetx + context.camera.x ) * fZoom ) + vp.pos.x;
            double  fBoxY = ( ( pObject -> y + pLayer -> offsety + context.camera.y ) * fZoom ) + vp.pos.y;
            double  fBoxW = pObject -> width * fZoom;
            double  fBoxH = pObject -> height * fZoom;
            double  fRotation = Shape :: RotationOf( pObject );

            int  nFontSize   = ( pText -> pixelsize > 0 ) ? pText -> pixelsize : __DEFAULT_TEXT_PIXEL_SIZE;
            int  nScreenSize = std :: max( 1, ( int ) std :: lround( nFontSize * fZoom ) );

            // Culled when the box is entirely outside the viewport. A zero
            // width or height has no far edge, so it is not tested on that side.
            double  fVpRight  = vp.pos.x + vp.size.nWidth;
            double  fVpBottom = vp.pos.y + vp.size.nHeight;

            if( ( fRotation == 0.0 ) && ( ( fBoxX >= fVpRight ) || ( fBoxY >= fVpBottom ) ||
                ( ( fBoxW > 0 ) && ( fBoxX + fBoxW <= vp.pos.x ) ) ||
                ( ( fBoxH > 0 ) && ( fBoxY + fBoxH <= vp.pos.y ) ) ) )
                return;

            SunLight :: Font :: IFont   *pFont   = fonts.Find( szFamily, bBold, bItalic );
            std :: string                   strText = ( pText -> text != nullptr ) ? pText -> text : "";

            if( pFont == nullptr )  {
                fonts.WarnMissing( szFamily, bBold, bItalic );
                strText = __MISSING_FONT_PLACEHOLDER;   // no font: the engine's active font draws the placeholder
            }

            SunLight :: Engines :: IEngine&  engine = SunLight :: Engines :: EngineFactory :: GetEngine();
            bool                             bWrap  = ( pText -> wrap != 0 ) && ( fBoxW > 0 );

            auto  Width = [&]( const std :: string& strLine )  {
                return pFont ? pFont -> MeasureText( strLine.c_str(), nScreenSize )
                             : engine.MeasureText( strLine.c_str(), nScreenSize );
            };

            // One paragraph (text between newlines): greedy word wrap, a word
            // wider than the box stays whole on its own line.
            auto  WrapParagraph = [&]( const std :: string& strParagraph, std :: vector<std :: string>& lines )  {
                std :: vector<std :: string>  words;
                size_t                        nPos = 0;

                while( nPos <= strParagraph.size() )  {
                    size_t  nSpace = strParagraph.find( ' ', nPos );

                    if( nSpace == std :: string :: npos )
                        nSpace = strParagraph.size();

                    if( nSpace > nPos )
                        words.push_back( strParagraph.substr( nPos, nSpace - nPos ) );

                    nPos = nSpace + 1;
                }

                std :: string  strCurrent;

                for( const std :: string& strWord : words )  {
                    std :: string  strCandidate = strCurrent.empty() ? strWord : strCurrent + " " + strWord;

                    if( bWrap && !strCurrent.empty() && ( Width( strCandidate ) > fBoxW ) )  {
                        lines.push_back( strCurrent );
                        strCurrent = strWord;
                    }
                    else
                        strCurrent = strCandidate;
                }

                lines.push_back( strCurrent );   // an empty paragraph is one empty line
            };

            std :: vector<std :: string>  lines;
            size_t                        nStart = 0;

            while( true )  {
                size_t       nEnd       = strText.find( '\n', nStart );
                std :: string  strParagraph = strText.substr( nStart, ( nEnd == std :: string :: npos ) ? std :: string :: npos : nEnd - nStart );

                WrapParagraph( strParagraph, lines );

                if( nEnd == std :: string :: npos )
                    break;

                nStart = nEnd + 1;
            }

            // Line spacing is the font size; the block is aligned inside the box.
            double  fBlockH = ( double ) lines.size() * nScreenSize;
            double  fTop    = fBoxY;

            if( fBoxH > 0 )  {
                if( pText -> valign == VA_CENTER )
                    fTop = fBoxY + ( ( fBoxH - fBlockH ) / 2 );
                else if( pText -> valign == VA_BOTTOM )
                    fTop = fBoxY + fBoxH - fBlockH;
            }

            // The lines are cut at the viewport's edge by the clip the view's pass (or the
            // single-view frame) already set, so the text does not open a clip of its own.

            for( size_t nIdx = 0; nIdx < lines.size(); nIdx++ )  {
                double  fLeft = fBoxX;

                if( fBoxW > 0 )  {
                    double  fLineW = Width( lines[nIdx] );

                    if( pText -> halign == HA_CENTER )
                        fLeft = fBoxX + ( ( fBoxW - fLineW ) / 2 );
                    else if( pText -> halign == HA_RIGHT )
                        fLeft = fBoxX + fBoxW - fLineW;
                }

                int  nX = ( int ) std :: lround( fLeft );
                int  nY = ( int ) std :: lround( fTop + ( nIdx * nScreenSize ) );

                // A turned block: each line's top-left corner turns about the block's top-left corner, and the line turns with it.
                if( fRotation != 0.0 )  {
                    Shape :: MapPoint  turned = Shape :: Rotate( Shape :: MapPoint { fLeft - fBoxX, ( fTop + ( nIdx * nScreenSize ) ) - fBoxY }, fRotation );
                    int                nTurnedX = ( int ) std :: lround( fBoxX + turned.fX );
                    int                nTurnedY = ( int ) std :: lround( fBoxY + turned.fY );

                    if( pFont )
                        pFont -> DrawTextRotated( lines[nIdx].c_str(), nTurnedX, nTurnedY, nScreenSize, ( float ) fRotation, color );
                    else
                        engine.DrawTextRotated( lines[nIdx].c_str(), nTurnedX, nTurnedY, nScreenSize, ( float ) fRotation, color );

                    continue;
                }

                if( pFont )
                    pFont -> DrawText( lines[nIdx].c_str(), nX, nY, nScreenSize, color );
                else
                    engine.DrawText( lines[nIdx].c_str(), nX, nY, nScreenSize, color );
            }
        }
    }
}
