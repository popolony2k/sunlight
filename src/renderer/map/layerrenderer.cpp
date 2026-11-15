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

#include "renderer/map/layerrenderer.h"
#include "renderer/shape/shapeobjects.h"
#include "renderer/shape/shapeprimitives.h"
#include "engines/enginefactory.h"
#include "general/helper.h"
#include "base/viewport.h"
#include <algorithm>


#define __OPAQUE_CHANNEL                0xFF    // a colour channel at full strength
#define __SCREEN_SPACE_PROPERTY        "screen_space"   // I23: the Tiled custom property (bool) of a screen-space image layer
#define __SCREEN_SPACE_SCALE           1.0f    // a screen-space image layer is drawn at its own size
#define __CULL_PADDING_TILES           1       // extra tiles either side of the visible range, for the near-edge zero-size-but-still-drawn case


namespace SunLight  {
    namespace Renderer  {

        /**
         * Draw a tile to specified position on texture.
         * @param context The view and layer to draw for;
         * @param pImage Handle of the tileset texture to draw from.
         * @param nSourceX Source tile X coordinate;
         * @param nSourceY Source tile Y coordinate;
         * @param nSourceW Source tile width;
         * @param nSourceH Source tile height;
         * @param nDestX destination X coordinate on texture;
         * @param nDestY destination Y coordinate on texture;
         * @param fOpacity opacity level to be applied on texture;
         * @param tintColor tint color to be applied on texture;
         */
        void LayerRenderer :: DrawTile( const DrawContext& context,
                                        SunLight :: Base :: TextureHandle pImage,
                                        int32_t nSourceX,
                                        int32_t nSourceY,
                                        int32_t nSourceW,
                                        int32_t nSourceH,
                                        int32_t nDestX,
                                        int32_t nDestY,
                                        float fOpacity,
                                        SunLight :: Base :: stColor tintColor ) const  {


            SunLight :: Base :: stDimension2D  clip;
            unsigned char                         op        = ( uint8_t ) ( 0xFF * fOpacity );
            SunLight :: Base :: stDimension2D  dm        =  { { ( int ) ( nDestX + context.camera.x ),
                                                                 ( int ) ( nDestY + context.camera.y ) },
                                                                 { nSourceW, nSourceH } };

            if( SunLight :: Base :: Viewport :: ClipRect( context.viewport, context.fZoom, dm, clip ) )  {

                float                                 fZoomFactor = context.fZoom;
                const SunLight :: Base :: stDimension2D& vpDm   = context.viewport;
                // Study: the whole tile is drawn at its zoomed size; the view pass's clip cuts it.
                //
                // The destination rect is rounded to the nearest whole screen pixel, half-up
                // (SunLight::General::Helper::RoundHalfUp), never std::round. Two adjacent
                // tiles' edges are always mathematically contiguous (their difference is an
                // exact multiple of the tile's own zoomed width/height), but at a non-integer
                // zoom that shared edge itself commonly lands on a fractional pixel (e.g.
                // 552.8125) - and two separately-issued draw calls whose shared edge is off
                // the pixel grid can leave the rasterizer's coverage of that boundary pixel
                // ambiguous, visible as a thin seam of the clear colour between every pair of
                // tiles at most camera positions. Rounding both tiles' edges onto the same
                // integer grid keeps them exactly abutting (round(x) + round(width) ==
                // round(x + width) whenever width is itself already a whole number AND both
                // round consistently - true for same-size tiles at a shared zoom with
                // round-half-UP, but NOT with std::round's round-half-AWAY-FROM-ZERO: found
                // live, a second time, with std::round still in place - two tiles whose exact
                // position each landed exactly on a half-pixel (an integer map position times
                // zoom 3.8125 can do that) rounded AWAY from each other across x = 0, opening
                // a real 1px gap even though they were exactly contiguous mathematically; see
                // Helper::RoundHalfUp's own doc comment. Found live (not assumed): samples/
                // sprite's own default zoom (position 60, factor 3.8125) showed both the
                // general seam and this specific half-pixel gap on ordinary scrolling.
                //
                // tintColor's own RGB multiplies into the tile's own pixels (Tiled's own
                // "tintcolor" layer property - white, the default, leaves every pixel
                // unchanged); its alpha channel is ignored here, replaced by the layer's own
                // opacity (op, above) - the two are independent controls in Tiled itself, and
                // DrawTile already had its own correct alpha source before tintColor existed.
                SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureTiled( pImage,
                                                      SunLight :: Base :: stRectangle  { ( float ) nSourceX,
                                                                   ( float ) nSourceY,
                                                                   ( float ) nSourceW,
                                                                   ( float ) nSourceH },
                                                      SunLight :: Base :: stRectangle  { SunLight :: General :: Helper :: RoundHalfUp( dm.pos.x * fZoomFactor + vpDm.pos.x ),
                                                                   SunLight :: General :: Helper :: RoundHalfUp( dm.pos.y * fZoomFactor + vpDm.pos.y ),
                                                                   SunLight :: General :: Helper :: RoundHalfUp( nSourceW * fZoomFactor ),
                                                                   SunLight :: General :: Helper :: RoundHalfUp( nSourceH * fZoomFactor ) },
                                                      SunLight :: Base :: stVector2D  { 0, 0 },
                                                      0.0f,
                                                      fZoomFactor,
                                                      SunLight :: Base :: stColor  { tintColor.nRed, tintColor.nGreen, tintColor.nBlue, op } );
            }
        }

        /**
         * @brief Draw one tile object (Tiled's OT_TILE): the tile its gid names,
         * at the object's position.
         *
         * Tiled places a tile object by its bottom-left corner, so the tile's
         * top-left is at (x, y - tile height). The tile is drawn at its own
         * size through DrawTile, which clips it to the viewport exactly as a
         * map tile is clipped. Its opacity applies, as a layer's does to tiles.
         *
         * A gid of 0, or a gid past the tileset's table, draws nothing. The
         * flip bits Tiled puts in the gid are ignored (flipping is not drawn
         * yet). An animated tile object animates as the same tile in a tile
         * layer does (MapData::AnimatedFrameOf).
         *
         * @param context The view and layer to draw for;
         * @param map The loaded map (its tiles and their animation);
         * @param pObject The tile object to draw;
         * @param pLayer The object group it belongs to (its offset and tint apply);
         * @param fOpacity The opacity to draw it with - its own "opacity" property, or its
         * layer's (ObjectRenderer works it out, ObjectOpacity);
         */
        void LayerRenderer :: DrawTileObject( const DrawContext& context, MapData& map, tmx_object* pObject, tmx_layer* pLayer, float fOpacity ) const  {

            unsigned int  nGID = pObject -> content.gid & TMX_FLIP_BITS_REMOVAL;

            tmx_map       *pTmxMap = map.Tmx();

            if( ( nGID == 0 ) || ( pTmxMap -> tiles == nullptr ) || ( nGID >= pTmxMap -> tilecount ) )
                return;

            tmx_tile  *pBase = pTmxMap -> tiles[nGID];

            if( pBase == nullptr )
                return;

            tmx_tile      *pTile   = ( pBase -> animation_len != 0 ) ? map.AnimatedFrameOf( pBase, nGID ) : pBase;
            tmx_tileset   *pTs     = pTile -> tileset;
            void          *pImage  = ( pTile -> image != nullptr ) ? pTile -> image -> resource_image : pTs -> image -> resource_image;
            SunLight :: Base :: stColor  tintColor = SunLight :: Base :: MultiplyTint( Shape :: ColorOf( pLayer -> tintcolor ), context.groupTint );   // I8: with its groups'

            int            nTileW   = ( int ) pTs -> tile_width;
            int            nTileH   = ( int ) pTs -> tile_height;

            // A turned tile object is the whole tile turned about its bottom-left corner, Tiled's pivot. The engine's clip
            // cuts it at the viewport, as it does every turned shape.
            double  fRotation = Shape :: RotationOf( pObject );

            if( fRotation != 0.0 )  {
                Shape :: PrimitiveClip  primitiveClip( context.viewport );

                const SunLight :: Base :: stDimension2D  &vpDm = context.viewport;
                double                              fZoom = context.fZoom;
                double                              fAnchorX = ( ( pObject -> x + pLayer -> offsetx + context.camera.x ) * fZoom ) + vpDm.pos.x;
                double                              fAnchorY = ( ( pObject -> y + pLayer -> offsety + context.camera.y ) * fZoom ) + vpDm.pos.y;
                double                              fBoxW    = pObject -> width * fZoom;
                double                              fBoxH    = pObject -> height * fZoom;

                // The anchor is the bottom-left corner: the destination's top-left sits one box height above it.
                SunLight :: Base :: stRectangle  source { ( float ) pTile -> ul_x, ( float ) pTile -> ul_y, ( float ) nTileW, ( float ) nTileH };
                SunLight :: Base :: stRectangle  dest   { ( float ) fAnchorX, ( float ) fAnchorY, ( float ) fBoxW, ( float ) fBoxH };
                SunLight :: Base :: stVector2D   origin { 0.0f, ( float ) fBoxH };
                SunLight :: Base :: stColor      tint   { tintColor.nRed, tintColor.nGreen, tintColor.nBlue, ( uint8_t ) ( __OPAQUE_CHANNEL * fOpacity ) };

                SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureRotated( pImage, source, dest, origin, ( float ) fRotation, tint );
                return;
            }

            // A tile object at its tile's own size is drawn exactly as a map tile is.
            if( ( ( int ) pObject -> width == nTileW ) && ( ( int ) pObject -> height == nTileH ) )  {
                DrawTile( context, pImage, pTile -> ul_x, pTile -> ul_y, nTileW, nTileH,
                          ( int ) ( pObject -> x + pLayer -> offsetx ),
                          ( int ) ( pObject -> y + pLayer -> offsety ) - nTileH,
                          fOpacity,
                          tintColor );

                return;
            }

            // A resized tile object is stretched into its box, as Tiled draws it. The box is
            // worked out in screen space (the same mapping as DrawRectangle), then cut to the viewport.
            const SunLight :: Base :: stDimension2D  &vpDm   = context.viewport;
            float                                  fZoom  = context.fZoom;

            float  fBoxX = ( ( pObject -> x + pLayer -> offsetx + context.camera.x ) * fZoom ) + vpDm.pos.x;
            float  fBoxY = ( ( pObject -> y + pLayer -> offsety - pObject -> height + context.camera.y ) * fZoom ) + vpDm.pos.y;
            float  fBoxW = pObject -> width * fZoom;
            float  fBoxH = pObject -> height * fZoom;

            float  fVpL = ( float ) vpDm.pos.x;
            float  fVpT = ( float ) vpDm.pos.y;
            float  fVpR = fVpL + ( float ) vpDm.size.nWidth;
            float  fVpB = fVpT + ( float ) vpDm.size.nHeight;

            float  fL = std :: max( fBoxX, fVpL );
            float  fT = std :: max( fBoxY, fVpT );
            float  fR = std :: min( fBoxX + fBoxW, fVpR );
            float  fB = std :: min( fBoxY + fBoxH, fVpB );

            if( ( fR <= fL ) || ( fB <= fT ) )
                return;

            // The visible part of the box, mapped back onto the matching part of the tile.
            SunLight :: Base :: stRectangle  source { pTile -> ul_x + ( ( fL - fBoxX ) / fBoxW * nTileW ),
                                                       pTile -> ul_y + ( ( fT - fBoxY ) / fBoxH * nTileH ),
                                                       ( fR - fL ) / fBoxW * nTileW,
                                                       ( fB - fT ) / fBoxH * nTileH };
            SunLight :: Base :: stRectangle  dest   { fL, fT, fR - fL, fB - fT };
            // The layer's tint, as a tile object at its own size and a turned one take it.
            SunLight :: Base :: stColor      tint   { tintColor.nRed, tintColor.nGreen, tintColor.nBlue, ( uint8_t ) ( __OPAQUE_CHANNEL * fOpacity ) };

            SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureScaled( pImage, source, dest, tint );
        }

        /**
         * Draw image layer on canvas (L1 of the master plan: now camera/parallax/zoom/
         * viewport-aware - before L1 this drew at the raw screen pixel (0,0), ignoring the
         * camera, zoom and viewport position entirely, which made adding a camera term alone
         * incoherent: the same screen mapping every tile already gets (position + effective
         * camera) x zoom + viewport origin, exactly mirroring DrawTile's own structure,
         * including its early-exit "fully offscreen" check - clipping to the viewport's own
         * rectangle is otherwise the active view's own clip region (BeginClip/EndClip), not
         * this function's job, same as DrawTile. Still only drawn for the default view (see
         * DrawAllLayers's own call site) - a separate, pre-existing restriction this item does
         * not lift. Its opacity applies (I8 of the master plan; it was always drawn opaque), times
         * its groups', and so does its tint, times its groups'. An image layer that asks for the old,
         * screen-space drawing (I23: "screen_space") is drawn by DrawScreenSpaceImageLayer instead.
         * @param context The view and layer to draw for;
         * @param pLayer The image layer to draw;
         */
        void LayerRenderer :: DrawImageLayer( const DrawContext& context, tmx_layer* pLayer ) const  {

            tmx_image                          *pTmxImage = pLayer -> content.image;
            void                               *pImage    = pTmxImage -> resource_image;
            SunLight :: Base :: stColor        tintColor  = SunLight :: Base :: MultiplyTint( Shape :: ColorOf( pLayer -> tintcolor ), context.groupTint );
            float                              fOpacity   = ( float ) pLayer -> opacity * context.fGroupOpacity;
            SunLight :: Base :: stDimension2D  clip;
            int                                nWidth     = 0;
            int                                nHeight    = 0;

            ImageSizeOf( pTmxImage, nWidth, nHeight );      // I24: the picture's real size, not the declared one

            SunLight :: Base :: stDimension2D  dm         = { { ( int ) ( pLayer -> offsetx + context.camera.x ),
                                                                ( int ) ( pLayer -> offsety + context.camera.y ) },
                                                              { nWidth, nHeight } };

            if( SunLight :: Base :: Viewport :: ClipRect( context.viewport, context.fZoom, dm, clip ) )  {

                float                                 fZoomFactor = context.fZoom;
                const SunLight :: Base :: stDimension2D& vpDm   = context.viewport;

                // Rounded to the nearest whole screen pixel, half-up - see DrawTile's own
                // identical rounding and its doc comment for why (a non-integer zoom can land
                // a tile's edge on a fractional pixel, visible as a seam against its neighbor,
                // and std::round's away-from-zero ties can open a real gap of their own).
                SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureTiled( pImage,
                                                      SunLight :: Base :: stRectangle  { 0.0f, 0.0f,
                                                                   ( float ) nWidth,
                                                                   ( float ) nHeight },
                                                      SunLight :: Base :: stRectangle  { SunLight :: General :: Helper :: RoundHalfUp( dm.pos.x * fZoomFactor + vpDm.pos.x ),
                                                                   SunLight :: General :: Helper :: RoundHalfUp( dm.pos.y * fZoomFactor + vpDm.pos.y ),
                                                                   SunLight :: General :: Helper :: RoundHalfUp( nWidth * fZoomFactor ),
                                                                   SunLight :: General :: Helper :: RoundHalfUp( nHeight * fZoomFactor ) },
                                                      SunLight :: Base :: stVector2D  { 0, 0 },
                                                      0.0f,
                                                      fZoomFactor,
                                                      SunLight :: Base :: stColor  { tintColor.nRed, tintColor.nGreen, tintColor.nBlue,
                                                                                     ( uint8_t ) ( __OPAQUE_CHANNEL * fOpacity ) } );
            }
        }

        /**
         * @brief The size an image layer's picture is drawn at (I24 of the master plan): the loaded
         * texture's REAL size (IEngine::GetTextureSize), as Tiled draws it and as sunlight drew it before
         * L1. The width and height the map's image element declares only record the picture's size when the
         * map was saved and go stale when the picture changes: drawn at a declared size larger than the
         * texture, the draw read past it and repeated it (Caravellius' cover, 1206 x 880 declared as
         * 1260 x 920). The declared size is used only when the engine cannot tell.
         * @param pTmxImage The layer's image;
         * @param nWidth Receives the width to draw;
         * @param nHeight Receives the height;
         */
        void LayerRenderer :: ImageSizeOf( tmx_image* pTmxImage, int& nWidth, int& nHeight )  {

            int  nRealWidth  = 0;
            int  nRealHeight = 0;

            if( pTmxImage -> resource_image &&
                SunLight :: Engines :: EngineFactory :: GetEngine().GetTextureSize( pTmxImage -> resource_image, nRealWidth, nRealHeight ) &&
                ( nRealWidth > 0 ) && ( nRealHeight > 0 ) )  {
                nWidth  = nRealWidth;
                nHeight = nRealHeight;
                return;
            }

            nWidth  = ( int ) pTmxImage -> width;
            nHeight = ( int ) pTmxImage -> height;
        }

        /**
         * @brief Whether an image layer is drawn in SCREEN space (I23 of the master plan): its Tiled
         * custom property "screen_space" is a bool set to true. Any other layer, or the property missing,
         * of another type or false, keeps L1's map-space drawing (DrawImageLayer).
         * @param pLayer The layer;
         * @return true for a screen-space image layer;
         */
        bool LayerRenderer :: IsScreenSpaceImageLayer( tmx_layer* pLayer )  {

            if( !pLayer || ( pLayer -> type != L_IMAGE ) || !pLayer -> properties )
                return false;

            tmx_property  *pProperty = ::tmx_get_property( pLayer -> properties, __SCREEN_SPACE_PROPERTY );

            return pProperty && ( pProperty -> type == PT_BOOL ) && ( pProperty -> value.boolean != 0 );
        }

        /**
         * @brief Draw an image layer in SCREEN space (I23 of the master plan), as every image layer was
         * drawn before L1: at its offset in window pixels, at its own size - no camera, parallax, zoom or
         * viewport origin. For a full-screen picture (Caravellius' cover) that stays put while sprites
         * on and above its layer draw over it. Its opacity and tint apply, times its groups', as for any
         * layer. Not cut to the viewport: the caller ends the view's clip around it (DrawAllLayers).
         * @param context The view and layer to draw for (its group opacity and tint);
         * @param pLayer The image layer to draw;
         */
        void LayerRenderer :: DrawScreenSpaceImageLayer( const DrawContext& context, tmx_layer* pLayer ) const  {

            tmx_image                    *pTmxImage = pLayer -> content.image;
            SunLight :: Base :: stColor  tintColor = SunLight :: Base :: MultiplyTint( Shape :: ColorOf( pLayer -> tintcolor ), context.groupTint );
            float                        fOpacity  = ( float ) pLayer -> opacity * context.fGroupOpacity;

            if( !pTmxImage || !pTmxImage -> resource_image )
                return;

            int  nWidth  = 0;
            int  nHeight = 0;

            ImageSizeOf( pTmxImage, nWidth, nHeight );      // I24: the picture's real size

            SunLight :: Engines :: EngineFactory :: GetEngine().DrawTextureTiled( pTmxImage -> resource_image,
                                                  SunLight :: Base :: stRectangle { 0.0f, 0.0f, ( float ) nWidth, ( float ) nHeight },
                                                  SunLight :: Base :: stRectangle { ( float ) pLayer -> offsetx, ( float ) pLayer -> offsety,
                                                                                    ( float ) nWidth, ( float ) nHeight },
                                                  SunLight :: Base :: stVector2D { 0, 0 },
                                                  0.0f,
                                                  __SCREEN_SPACE_SCALE,
                                                  SunLight :: Base :: stColor { tintColor.nRed, tintColor.nGreen, tintColor.nBlue,
                                                                                ( uint8_t ) ( __OPAQUE_CHANNEL * fOpacity ) } );
        }

        /**
         * Draw a tile layer: every cell the view can show (culled when the map's grid allows it,
         * see MapData::IsUniformTileGrid), each tile at its tileset's size, animated tiles at their
         * current frame, with the layer's opacity and tint.
         * @param context The view and layer to draw for;
         * @param map The loaded map;
         * @param pLayer The tile layer to draw;
         */
        void LayerRenderer :: DrawTileLayer( const DrawContext& context, MapData& map, tmx_layer* pLayer ) const  {

            // I8: the layer's own opacity and tint, with its enclosing groups' multiplied in (context).
            float                              fOpacity  = ( float ) pLayer -> opacity * context.fGroupOpacity;
            SunLight :: Base :: stColor  tintColor = SunLight :: Base :: MultiplyTint( Shape :: ColorOf( pLayer -> tintcolor ), context.groupTint );
            Projection :: stTileRange   range;

            tmx_map                            *pTmxMap  = map.Tmx();
            Projection :: IMapProjection       &projection = map.MapProjection();

            if( map.IsUniformTileGrid() )  {
                // The visible range, in the projection's own map-pixel space: the viewport's own rectangle, local
                // to the camera and this layer's own offset (DrawTile adds the camera itself; the layer offset is
                // part of the same map-pixel space TileDrawPosition answers in), padded by a whole tile either
                // side for the near-edge case (GetClippedRect still "passes" a tile trimmed to zero width there).
                const SunLight :: Base :: stDimension2D  &vpDm  = context.viewport;
                float                                fZoom = context.fZoom;
                int                                  nPadX = __CULL_PADDING_TILES * ( int ) pTmxMap -> tile_width;
                int                                  nPadY = __CULL_PADDING_TILES * ( int ) pTmxMap -> tile_height;

                SunLight :: Base :: stRectangle  visibleRect {
                    -context.camera.x - pLayer -> offsetx - nPadX,
                    -context.camera.y - pLayer -> offsety - nPadY,
                    ( vpDm.size.nWidth / fZoom ) + ( 2 * nPadX ),
                    ( vpDm.size.nHeight / fZoom ) + ( 2 * nPadY ) };

                range = projection.VisibleTileRange( pTmxMap, visibleRect );
            }
            else  {
                // A map that mixes tileset sizes: visit every cell, exactly as every map did before E0.
                range = Projection :: stTileRange { 0, ( int ) pTmxMap -> height, 0, ( int ) pTmxMap -> width };
            }

            for( int i = range.nRowStart; i < range.nRowEnd; i++ )  {
                for( int j = range.nColStart; j < range.nColEnd; j++ )  {
                    SunLight :: TileMap :: stTile            tile;
                    SunLight :: TileMap :: stMatrixPosition  pos   = { i, j };

                    // The gid and tile only (MapData::TileAt): the cell's screen rectangle, which the
                    // renderer's public GetTile also works out, is not needed to draw it.
                    if( map.TileAt( pos, pLayer, tile ) )  {
                        tmx_tile       *pTile = tile.pTile;
                        tmx_image      *pIm   = pTile -> image;
                        tmx_tileset    *pTs;
                        void           *pImage;

                        // Perform tile animation (shared with tile objects, see MapData::AnimatedFrameOf).
                        if( pTile -> animation_len )
                            pTile = map.AnimatedFrameOf( pTile, tile.nGID );

                        pTs = pTile -> tileset;

                        if( pIm )  {
                            pImage = pIm -> resource_image;
                        }
                        else  {
                            pImage = pTs -> image -> resource_image;
                        }

                        SunLight :: Base :: stCoordinate2D  dest = projection.TileDrawPosition( pos, pTmxMap, pTs -> tile_width,
                                                                                                       pTs -> tile_height,
                                                                                                       pLayer -> offsetx,
                                                                                                       pLayer -> offsety );

                        DrawTile( context,
                                pImage,
                                pTile -> ul_x,
                                pTile -> ul_y,
                                pTs -> tile_width,
                                pTs -> tile_height,
                                dest.x,
                                dest.y,
                                fOpacity,
                                tintColor );
                    }
                }
            }
        }
    }
}
