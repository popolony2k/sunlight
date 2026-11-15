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

#include "renderer/objects/objectrenderer.h"
#include "renderer/shape/shapeobjects.h"
#include "renderer/shape/shapeprimitives.h"


namespace SunLight  {
    namespace Renderer  {

        /**
         * Draw an object layer: every visible object, in the layer's order.
         * @param context The view and layer to draw for;
         * @param pLayer The object group to draw;
         * @param fonts The fonts text objects are drawn with;
         * @param drawTileObject Draws a tile object (see TILE_OBJECT_DRAWER);
         */
        void ObjectRenderer :: Draw( const DrawContext& context,
                                     tmx_layer* pLayer,
                                     FontRegistry& fonts,
                                     const TILE_OBJECT_DRAWER& drawTileObject )  {

            tmx_object* head = pLayer ->  content.objgr -> head;
            SunLight :: Base :: stColor  baseColor = Shape :: ColorOf( pLayer ->  content.objgr -> color );

            while( head )  {
                if( head -> visible )  {
                    // L2: the object's own "opacity" property, or its layer's, multiplies into the
                    // object group's own stored colour alpha - shapes have no separate alpha control
                    // of their own to begin with, the same reasoning as a text object's.
                    // I8: times every enclosing group's opacity (context), as Tiled draws a group.
                    float                        fOpacity = m_Opacity.Of( head, ( float ) pLayer -> opacity ) * context.fGroupOpacity;
                    SunLight :: Base :: stColor  color    = baseColor;

                    color.nAlpha = ( uint8_t ) ( color.nAlpha * fOpacity );

                    switch( head -> obj_type )  {
                        case OT_SQUARE :
                            m_Shapes.DrawRectangle( context, ( head -> x + pLayer -> offsetx ),
                                                    ( head -> y + pLayer -> offsety ),
                                                    head -> width,
                                                    head -> height,
                                                    Shape :: RotationOf( head ),
                                                    Shape :: ScreenLineWidth( Shape :: LineWidthOf( head ), context.fZoom ),
                                                    color );
                            break;

                        case OT_POLYGON :
                            m_Shapes.DrawPolygon( context, ( head -> x + pLayer -> offsetx ),
                                                  ( head -> y + pLayer -> offsety ),
                                                  head -> content.shape -> points,
                                                  head -> content.shape -> points_len,
                                                  Shape :: RotationOf( head ),
                                                  Shape :: ScreenLineWidth( Shape :: LineWidthOf( head ), context.fZoom ),
                                                  color );
                            break;

                        case OT_POLYLINE :
                            m_Shapes.DrawPolyline( context, ( head -> x + pLayer -> offsetx ),
                                                   ( head -> y + pLayer -> offsety ),
                                                   head -> content.shape -> points,
                                                   head -> content.shape -> points_len,
                                                   Shape :: RotationOf( head ),
                                                   Shape :: ScreenLineWidth( Shape :: LineWidthOf( head ), context.fZoom ),
                                                   color );
                            break;

                        case OT_ELLIPSE :
                            m_Shapes.DrawEllipse( context, ( head -> x + pLayer -> offsetx ),
                                                  ( head -> y + pLayer -> offsety ),
                                                  head -> width,
                                                  head -> height,
                                                  Shape :: RotationOf( head ),
                                                  Shape :: ScreenLineWidth( Shape :: LineWidthOf( head ), context.fZoom ),
                                                  color );
                            break;

                        case OT_NONE :
                            // libtmx turns an object with no shape into OT_POINT while parsing, so this
                            // kind never reaches the renderer. The case stays so the switch covers every
                            // enumerator.
                            break;

                        case OT_TILE :
                            drawTileObject( head, pLayer, fOpacity );
                            break;

                        case OT_TEXT :
                            m_Text.Draw( context, head, pLayer, fonts, m_Opacity );
                            break;

                        case OT_POINT :
                            m_Shapes.DrawPoint( context, ( head -> x + pLayer -> offsetx ),
                                                ( head -> y + pLayer -> offsety ),
                                                Shape :: PointSizeOf( head ),
                                                color );
                            break;
                    }
                }

                head = head -> next;
            }
        }

        /**
         * @brief Forget the invalid-opacity warnings already given (see ObjectOpacity::Reset).
         */
        void ObjectRenderer :: ResetWarnings( void )  {

            m_Opacity.Reset();
        }
    }
}
