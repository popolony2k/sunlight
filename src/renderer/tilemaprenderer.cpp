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

#ifndef DEFAULT_ENGINE
    #error "Unexpected value of DEFAULT_ENGINE"
#endif

/*
 * In the future a better abstract way to handle raylib will be
 * implemented and the method below will be changed.
 */
#if DEFAULT_ENGINE == 1    /* USES RAYLIB */
    #include "engines/raylib/raylibengine.h"
    
    #define __DEFAULT_ENGINE         RaylibEngine
#else
    #error "Unknown value of DEFAULT_ENGINE"
#endif

#include "input/inputhandlerfactory.h"
#include "tilemaprenderer.h"
#include <memory.h>
#include <cstring>
#include <chrono>
#include <cmath>
#include <algorithm>

/*
 * Engine defaults.
 */
#define __DEFAULT_FPS                   30
#define __DEFAULT_SCROLL_STEP_WIDTH     -1
#define __DEFAULT_SCROLL_STEP_HEIGHT    -1
#define __DEFAULT_VISIBLE_STATUS        true
#define __DEFAULT_CLEAR_BACKGROUND      true
#define __DEFAULT_RESIZEABLE_STATUS     false
#define __DEFAULT_DRAW_FPS_STATUS       false
#define __DEFAULT_VIEW_CONTROL_MODE     VIEW_CONTROL_MODE_ACTIVE
#define __DEFAULT_EXIT_KEY              KEY_ESCAPE
#define __DEFAULT_WINDOW_BK_COLOR       0xFF000000


/*
 * Engine limits.
 */
#define __MAX_OPACITY_LEVEL            0xFF

using namespace std :: chrono;
using namespace SunLight :: Engines :: Raylib;


namespace SunLight {
    namespace Renderer  {

        /*
         * Static class initialization.
         */
        bool TileMapRenderer :: m_bInitialized = false;


        /**
         * TxmLib texture loader callback implementation.
         * @param szFileName Texture file name;
         */
        void* TileMapRenderer :: TextureLoaderCallback( const char *szFileName )  {

            Texture2D *pTexture = new Texture2D;

            *pTexture = ::LoadTexture( szFileName );

            return pTexture;
        }

        /**
         * TxmLib texture deallocation callback implementation.
         * @param pTexture Pointer to the texture that will be deallocated;
         */
        void TileMapRenderer :: TextureFreeCallback( void *pTexture )  {

            Texture2D    *pTexture2D = ( Texture2D * ) pTexture;

            ::UnloadTexture( *pTexture2D );

            delete pTexture2D;
        }

        /**
         * Retrieve which tmx_tileset_list contains the @see tmx_tileset passed as parameter;
         * @param pMap The map to search the tmx_tileset;
         * @param pTilesetSearch The tmx_tileset to retrieve the tmx_tileset_list;
         */
        tmx_tileset_list* TileMapRenderer :: GetTilesetList( tmx_map *pMap, tmx_tileset *pTilesetSearch )  {
            for( tmx_tileset_list *pItem = pMap -> ts_head; pItem != NULL; pItem = pMap -> ts_head -> next )  {
                if( pItem -> tileset == pTilesetSearch )  {
                    return pItem;           
                }
            }

            return nullptr;
        }

        /**
         * Get a pointer to a loaded layer based on it's Id.
         * @param nLayerId The Layer Id to retrieve the layer;
         */
        tmx_layer* TileMapRenderer :: GetLayer( int nLayerId )  {

            return ::tmx_find_layer_by_id( m_pTmxMap, nLayerId );
        }

        /**
         * Get a pointer to a loaded layer based on it's Id or name.
         * @param szLayerName The Layer name used to retrieve the layer. If the layer
         * name is NULL parameter nLayerId will be used for searching layer;
         */
        tmx_layer* TileMapRenderer :: GetLayer( const char *szLayerName )  {

            return ::tmx_find_layer_by_name( m_pTmxMap, szLayerName );
        }

        /**
         * Convert integer color representation to @link Color object;
         */
        Color TileMapRenderer :: IntToColor( uint32_t color ) {

            tmx_col_bytes res = ::tmx_col_to_bytes( color );

            return *( ( Color * ) &res );
        }

        /**
         * Draw  pixel according the specified position.
         * @param nCoordX The X coordinate to plot pixel;
         * @param nCoordY The Y coordinate to plot pixel;
         * @param color Color of pixel;
         */
        void TileMapRenderer :: SetPixel( int nCoordX, int nCoordY, Color color )  {

            SunLight :: TileMap :: stDimension2D& vp = GetViewport().GetDimension2D();

            if( ( nCoordX > vp.pos.x ) && ( nCoordX < vp.size. nWidth) &&
                ( nCoordY  > vp.pos. y ) && ( nCoordY < vp.size.nHeight ) ) {
                __DEFAULT_ENGINE :: SetPixel( nCoordX, nCoordY, color );
            }
        }

        /**
         * Midpoint ellipse drawing algorithm based on implementation found at
         * https://www.geeksforgeeks.org/midpoint-ellipse-drawing-algorithm/
         * @param fCoordX Ellipse X coordinate;
         * @param fCoordY Ellipse Y coordinate;
         * @param fRadiusX X radius;
         * @param fRadiusX Y radius;
         * @param color Ellipse color;
         */
        void TileMapRenderer :: MidPointEllipse( double fCoordX,
                                                 double fCoordY,
                                                 double fRadiusX,
                                                 double fRadiusY,
                                                 Color color ) {

            double          dx, dy;
            double          d1, d2;
            double          x = 0;
            double          y = fRadiusY;

            // Initial decision parameter of region 1
            d1 = ( fRadiusY * fRadiusY ) -
                 ( fRadiusX * fRadiusX * fRadiusY ) +
                 ( 0.25 * fRadiusX * fRadiusX );
            dx = ( 2 * fRadiusX * fRadiusY * x );
            dy = ( 2 * fRadiusX * fRadiusX * y );

            // For region 1
            while( dx < dy )  {
                int nXPos = ( int ) ( x + fCoordX );
                int nYPos = ( int ) ( y + fCoordY );
                int nXNeg = ( int ) ( -x + fCoordX );
                int nYNeg = ( int ) ( -y + fCoordY );

                // Print points based on 4-way symmetry
                SetPixel( nXPos, nYPos, color );
                SetPixel( nXNeg, nYPos, color );
                SetPixel( nXPos, nYNeg, color );
                SetPixel( nXNeg, nYNeg, color );

                /* 
                * Checking and updating value of decision parameter 
                * based on algorithm.
                */
                if( d1 < 0 )  {
                    x++;
                    dx = ( dx + (2 * fRadiusY * fRadiusY ) );
                    d1 = ( d1 + dx + ( fRadiusY * fRadiusY ) );
                }
                else  {
                    x++;
                    y--;
                    dx = ( dx + ( 2 * fRadiusY * fRadiusY ) );
                    dy = ( dy - ( 2 * fRadiusX * fRadiusX ) );
                    d1 = ( d1 + dx - dy + ( fRadiusY * fRadiusY ) );
                }
            }

            // Decision parameter of region 2
            d2 = ( ( fRadiusY * fRadiusY ) * ( ( x + 0.5 ) * ( x + 0.5 ) ) ) +
                 ( ( fRadiusX * fRadiusX ) * ( ( y - 1 ) * ( y - 1 ) ) ) -
                 ( fRadiusX * fRadiusX * fRadiusY * fRadiusY );

            // Plotting points of region 2
            while( y >= 0 ) {

                int nXPos = ( int ) ( x + fCoordX );
                int nYPos = ( int ) ( y + fCoordY );
                int nXNeg = ( int ) ( -x + fCoordX );
                int nYNeg = ( int ) ( -y + fCoordY );

                // Print points based on 4-way symmetry
                SetPixel( nXPos, nYPos, color );
                SetPixel( nXNeg, nYPos, color );
                SetPixel( nXPos, nYNeg, color );
                SetPixel( nXNeg, nYNeg, color );

                /*
                * Checking and updating parameter value based
                * on algorithm.
                */
                if( d2 > 0 ) {
                    y--;
                    dy = ( dy - ( 2 * fRadiusX * fRadiusX ) );
                    d2 = ( d2 + ( fRadiusX * fRadiusX ) - dy );
                }
                else  {
                    y--;
                    x++;
                    dx = ( dx + ( 2 * fRadiusY * fRadiusY ) );
                    dy = ( dy - ( 2 * fRadiusX * fRadiusX ) );
                    d2 = ( d2 + dx - dy + ( fRadiusX * fRadiusX ) );
                }
            }
        }

        /**
         * Bresenham line generation algorithm based on implementation found at
         * https://gist.github.com/bert/1085538.
         * @param nX0 Initial X line coordinate;
         * @param nY0 Initial Y line coordinate;
         * @param nX1 Final X line coordinate;
         * @param nY1 Final Y line coordinate;
         * @param color line color;
         */
        void TileMapRenderer :: LineBresenham( int nX0,
                                               int nY0,
                                               int nX1,
                                               int nY1,
                                               Color color )  {

            int             nE2; /* error value e_xy */
            int             nDx  = std :: abs( nX1 - nX0 );
            int             nSx  = ( nX0 < nX1 ? 1 : -1 );
            int             nDy  = -std :: abs( nY1 - nY0 );
            int             nSy  = ( nY0 < nY1 ? 1 : -1 );
            int             nErr = nDx + nDy;

            while( true )  {
                // Print points based on 4-way symmetry
                SetPixel( nX0, nY0, color );

                if( ( nX0 == nX1 ) && ( nY0 == nY1 ) )
                    break;

                nE2 = ( 2 * nErr );

                if( nE2 >= nDy ) {
                    nErr+=nDy;
                    nX0+=nSx;
                } /* e_xy+e_x > 0 */

                if( nE2 <= nDx ) {
                    nErr+=nDx;
                    nY0+=nSy;
                } /* e_xy+e_y < 0 */
            }
        }

        /**
         * Draw a polygon line (without close the polygon).
         * @param fOffset_x X polygon coordinate;
         * @param fOffset_y Y polygon coordinate;
         * @param points array of points for this polygon;
         * @param points_count Number of items of points array;
         */
        void TileMapRenderer :: DrawPolyline( double fOffset_x,
                                              double fOffset_y,
                                              double **fPoints,
                                              int nPointsCount,
                                              Color color ) {

            SunLight :: Base :: stZoomProperties&  zp = GetViewport().GetZoomProperties();
            SunLight :: TileMap :: stDimension2D&  vp = GetViewport().GetDimension2D();

            fOffset_x = ( ( fOffset_x + m_CameraPos.x ) * zp.fZoomFactor ) + vp.pos.x;
            fOffset_y = ( ( fOffset_y + m_CameraPos.y ) * zp.fZoomFactor ) + vp.pos.y;

            for( int i=1; i < nPointsCount; i++ ) {
                LineBresenham( ( int ) ( fOffset_x + ( fPoints[i-1][0] * zp.fZoomFactor ) ),
                               ( int ) ( fOffset_y + ( fPoints[i-1][1] * zp.fZoomFactor ) ),
                               ( int ) ( fOffset_x + ( fPoints[i][0] * zp.fZoomFactor ) ),
                               ( int ) ( fOffset_y + ( fPoints[i][1] * zp.fZoomFactor ) ),
                               color );
            }
        }

        /**
         * Draw a polygon line (without close the polygon).
         * @param fOffset_x X polygon coordinate;
         * @param fOffset_y Y polygon coordinate;
         * @param points array of points for this polygon;
         * @param points_count Number of items of points array;
         */
        void TileMapRenderer :: DrawPolygon( double fOffset_x,
                                         double fOffset_y,
                                         double **fPoints,
                                         int nPointsCount,
                                         Color color ) {

            SunLight :: TileMap :: stDimension2D&  vp = GetViewport().GetDimension2D();

            DrawPolyline( fOffset_x,
                        fOffset_y,
                        fPoints,
                        nPointsCount,
                        color );

            if( nPointsCount > 2 ) {
                SunLight :: Base :: stZoomProperties& zp = GetViewport().GetZoomProperties();

                fOffset_x = ( ( fOffset_x + m_CameraPos.x ) *
                            zp.fZoomFactor ) + vp.pos.x;
                fOffset_y = ( ( fOffset_y + m_CameraPos.y ) *
                            zp.fZoomFactor ) + vp.pos.y;

                LineBresenham( ( int ) ( fOffset_x + ( fPoints[0][0] * zp.fZoomFactor ) ),
                               ( int ) ( fOffset_y + ( fPoints[0][1] * zp.fZoomFactor ) ),
                               ( int ) ( fOffset_x + ( fPoints[nPointsCount-1][0] * zp.fZoomFactor ) ),
                               ( int ) ( fOffset_y + ( fPoints[nPointsCount-1][1] * zp.fZoomFactor ) ),
                               color );
            }
        }

        /**
         * Draw a square primitive to specified position on texture.
         * @param fOffset_x X square coordinate;
         * @param fOffset_y Y square coordinate;
         * @param fWidth The square width;
         * @param fHeight The square height;
         * @param color Rectangle color;
         */
        void TileMapRenderer :: DrawRectangle( double fOffset_x,
                                               double fOffset_y,
                                               double fWidth,
                                               double fHeight,
                                               Color color )  {

            SunLight :: TileMap :: stDimension2D&  vp          = GetViewport().GetDimension2D();
            SunLight :: Base :: stZoomProperties&  zp          = GetViewport().GetZoomProperties();
            double                                 fViewStartX = ( ( fOffset_x + m_CameraPos.x ) *
                                                                   zp.fZoomFactor ) + vp.pos.x;
            double                                 fViewStartY = ( ( fOffset_y + m_CameraPos.y ) *
                                                                   zp.fZoomFactor ) + vp.pos.y;
            double                                 fViewEndX   = ( ( fOffset_x + fWidth + m_CameraPos.x ) *
                                                                   zp.fZoomFactor ) + vp.pos.x;
            double                                 fViewEndY   = ( ( fOffset_y + fHeight + m_CameraPos.y ) *
                                                                   zp.fZoomFactor ) + vp.pos.y;

            // Top line
            LineBresenham( ( int ) fViewStartX,
                           ( int ) fViewStartY,
                           ( int ) fViewEndX,
                           ( int ) fViewStartY,
                           color );
            // Bottom line
            LineBresenham( ( int ) fViewStartX,
                           ( int ) fViewEndY,
                           ( int ) fViewEndX,
                           ( int ) fViewEndY,
                           color );

            // Left line
            LineBresenham( ( int ) fViewStartX,
                           ( int ) fViewStartY,
                           ( int ) fViewStartX,
                           ( int ) fViewEndY,
                           color );

            // Right line
            LineBresenham( ( int ) fViewEndX,
                           ( int ) fViewStartY,
                           ( int ) fViewEndX,
                           ( int ) fViewEndY,
                           color );
        }

        /**
         * Draw an ellipse primitive to specified position on texture.
         * @param fOffset_x X ellipse coordinate;
         * @param fOffset_y Y ellipse coordinate;
         * @param fWidth The ellipse width;
         * @param fHeight The ellipse height;
         * @param color ellipse color;
         */
        void TileMapRenderer :: DrawEllipse( double fOffset_x,
                                             double fOffset_y,
                                             double fWidth,
                                             double fHeight,
                                             Color color )  {

            SunLight :: TileMap :: stDimension2D&  vp = GetViewport().GetDimension2D();
            SunLight :: Base :: stZoomProperties&  zp = GetViewport().GetZoomProperties();

            fWidth-=( fWidth / 2.0 );
            fHeight-=( fHeight / 2.0 );
            fOffset_x = ( ( fOffset_x + fWidth + m_CameraPos.x ) *
                          zp.fZoomFactor ) + vp.pos.x;
            fOffset_y = ( ( fOffset_y + fHeight + m_CameraPos.y ) *
                          zp.fZoomFactor ) + vp.pos.y;

            MidPointEllipse( fOffset_x,
                             fOffset_y,
                             ( fWidth * zp.fZoomFactor ),
                             ( fHeight * zp.fZoomFactor ),
                             color );
        }

        /**
         * Draw a tile to specified position on texture.
         * @param pImage Pointer to a @link Texture2D object used as renderer.
         * @param uSourceX Source tile X coordinate;
         * @param uSourceY Source tile Y coordinate;
         * @param uSourceW Source tile width;
         * @param uSourceH Source tile height;
         * @param uDestX destination X coordinate on texture;
         * @param uDestY destination Y coordinate on texture;
         * @param opacity opacity level to be applied on texture;
         */
        void TileMapRenderer :: DrawTile( void *pImage,
                                          int32_t nSourceX,
                                          int32_t nSourceY,
                                          int32_t nSourceW,
                                          int32_t nSourceH,
                                          int32_t nDestX,
                                          int32_t nDestY,
                                          float fOpacity ) {

            SunLight :: TileMap :: stDimension2D  clip;
            SunLight :: Base :: Viewport&         vp        = GetViewport();
            Texture2D                             *pTexture = ( Texture2D * ) pImage;
            unsigned char                         op        = ( uint8_t ) ( 0xFF * fOpacity );
            SunLight :: TileMap :: stDimension2D  dm        =  { { ( int ) ( nDestX + m_CameraPos.x ),
                                                                 ( int ) ( nDestY + m_CameraPos.y ) },
                                                                 { nSourceW, nSourceH } };

            if( vp.GetClippedRect( dm, clip ) )  {

                float                                 fZoomFactor = vp.GetZoomProperties().fZoomFactor;
                SunLight :: TileMap :: stDimension2D& vpDm        = vp.GetDimension2D();
                int32_t                               nClipX      = ( int32_t ) ( clip.pos.x == vpDm.pos.x ? nSourceX +
                                                                                  std :: abs( ( clip.size.nWidth /
                                                                                                fZoomFactor ) -
                                                                                  dm.size.nWidth ) : nSourceX );
                int32_t                               nClipY      = ( int32_t ) ( clip.pos.y == vpDm.pos.y ? nSourceY +
                                                                                  std :: abs( ( clip.size.nHeight /
                                                                                                fZoomFactor ) -
                                                                                  dm.size.nHeight ) : nSourceY );

                __DEFAULT_ENGINE :: DrawTextureTiled( *pTexture,
                                                      Rectangle  { ( float ) nClipX,
                                                                   ( float ) nClipY,
                                                                   ( float ) nSourceW,
                                                                   ( float ) nSourceH },
                                                      Rectangle  { ( float ) clip.pos.x,
                                                                   ( float ) clip.pos.y,
                                                                   ( float ) clip.size.nWidth,
                                                                   ( float ) clip.size.nHeight },
                                                      Vector2  { 0, 0 },
                                                      0.0f,
                                                      fZoomFactor,
                                                      Color  { 0xFF, 0xFF, 0xFF, op } );
            }
        }

        /**
         * Draw objects on canvas;
         * @param pLayer Pointer to layer containing object group to draw;
         */
        void TileMapRenderer :: DrawObjects( tmx_layer *pLayer ) {

            tmx_object *head = pLayer ->  content.objgr -> head;
            Color      color = IntToColor( pLayer ->  content.objgr -> color );

            while( head ) {
                if( head -> visible ) {
                    switch( head -> obj_type )  {
                        case OT_SQUARE :
                            DrawRectangle( ( head -> x + pLayer -> offsetx ),
                                           ( head -> y + pLayer -> offsety ),
                                           head -> width,
                                           head -> height,
                                           color );
                            break;

                        case OT_POLYGON :
                            DrawPolygon( ( head -> x + pLayer -> offsetx ),
                                         ( head -> y + pLayer -> offsety ),
                                         head -> content.shape -> points,
                                         head -> content.shape -> points_len,
                                         color );
                            break;

                        case OT_POLYLINE :
                            DrawPolyline( ( head -> x + pLayer -> offsetx ),
                                          ( head -> y + pLayer -> offsety ),
                                          head -> content.shape -> points,
                                          head -> content.shape -> points_len,
                                          color );
                            break;

                        case OT_ELLIPSE :
                            DrawEllipse( ( head -> x + pLayer -> offsetx ),
                                         ( head -> y + pLayer -> offsety ),
                                         head -> width,
                                         head -> height,
                                         color );
                            break;
                        
                        case OT_NONE :
                            // TODO: FINISH HIM !!!
                            break;
                        
                        case OT_TILE :
                            // TODO: FINISH HIM !!!
                            break;

                        case OT_TEXT :
                            // TODO: FINISH HIM !!!
                            break;

                        case OT_POINT :
                        // TODO: FINISH HIM !!!
                        break;
                    }
                }

                head = head -> next;
            }
        }

        /**
         * Draw image layer on canvas;
         * @param pImage Pointer to layer containing image to draw;
         */
        void TileMapRenderer :: DrawImageLayer( tmx_layer *pLayer ) {

            Texture2D *pTexture = ( Texture2D * ) pLayer -> content.image -> resource_image;

            DrawTexture( *pTexture, 0, 0, WHITE );
        }

        /**
         * Draw layer on screen canvas;
         * @param pMap Pointer to layer map;
         * @param pLayer Pointer to layer with objects to draw;
         */
        void TileMapRenderer :: DrawLayer( tmx_map *pMap, tmx_layer *pLayer ) {

            float         fOpacity = ( float ) pLayer -> opacity;


            for( unsigned long i = 0; i < pMap -> height; i++ ) {
                for( unsigned long j = 0; j < pMap -> width; j++ ) {
                    SunLight :: TileMap :: stTile            tile;
                    SunLight :: TileMap :: stMatrixPosition  pos   = { ( int ) i, ( int ) j };
                    SunLight :: TileMap :: stLayer           layer = { false, 0, {0, 0}, pLayer };

                    if( GetTile( pos, layer, tile ) )  {
                        tmx_tile       *pTile = tile.pTile;
                        tmx_image      *pIm   = pTile -> image;
                        tmx_tileset    *pTs;
                        void           *pImage;

                        /*
                        * Perform tile animation
                        */
                        if( pTile -> animation_len )  {
                            steady_clock :: time_point now     = steady_clock :: now();
                            int64_t                    nMillis = duration_cast<milliseconds>( now.time_since_epoch() ).count();
                            __stTileAnimInfo           *pAnimInfo = ( __stTileAnimInfo * ) pTile -> user_data.pointer;
                            tmx_tileset_list           *pTilesetList;

                            if( !pAnimInfo )  {
                                pAnimInfo = new __stTileAnimInfo;
                                memset( pAnimInfo, 0, sizeof( __stTileAnimInfo ) );
                                pAnimInfo -> nMillis   = nMillis;
                                pAnimInfo -> pNextTile = pTile;
                                pTile -> user_data.pointer = pAnimInfo;
                                m_AnimInfoList.push_back( pAnimInfo );
                            }

                            if( pAnimInfo -> nMillis <= nMillis )  {
                                unsigned int     nNextFrmGID;
                                _tmx_frame       *pTmxAnimFrm;

                                if( pAnimInfo -> nCounter < pTile -> animation_len )  {
                                    pTmxAnimFrm = &pTile -> animation[pAnimInfo -> nCounter];
                                    pAnimInfo -> nCounter++;
                                }
                                else  {
                                    pAnimInfo -> nCounter = 0;
                                    pTmxAnimFrm = &pTile -> animation[0];
                                }
                              
                                pTilesetList = GetTilesetList( pMap, pTile -> tileset );
                                pTile = nullptr;

                                if( pTilesetList )  {
                                    nNextFrmGID = ( pTilesetList -> firstgid + pTmxAnimFrm -> tile_id );
                                    pAnimInfo -> pNextTile = pMap -> tiles[nNextFrmGID];
                                    pAnimInfo -> nMillis   = ( pTmxAnimFrm -> duration + nMillis );
                                    pTile = pAnimInfo -> pNextTile;
                                }

                                if( !pTile )
                                    pTile = pMap -> tiles[tile.nGID];
                            }
                            else  {
                                pTile = ( ( __stTileAnimInfo * ) pTile -> user_data.pointer ) -> pNextTile;
                            }
                        }

                        pTs = pTile -> tileset;

                        if( pIm ) {
                            pImage = pIm -> resource_image;
                        }
                        else {
                            pImage = pTs -> image -> resource_image;
                        }

                        DrawTile( pImage,
                                pTile -> ul_x,
                                pTile -> ul_y,
                                pTs -> tile_width,
                                pTs -> tile_height,
                                ( ( j * pTs -> tile_width ) + pLayer -> offsetx ),
                                ( ( i * pTs -> tile_height ) + pLayer -> offsety ),
                                fOpacity );
                    }
                }
            }
        }

        /**
         * Draw all layers on screen canvas;
         * @param pMap Pointer to layers map;
         * @param pLayer Array of layer objects to draw;
         */
        void TileMapRenderer :: DrawAllLayers( tmx_map *pMap, tmx_layer *pLayer ) {

            while( pLayer ) {
                if( pLayer -> visible ) {
                    switch( pLayer -> type )  {
                        case L_GROUP :
                            DrawAllLayers( pMap, pLayer -> content.group_head ); // recursive call
                            break;
                        case L_OBJGR :
                            DrawObjects( pLayer );
                            break;
                        case L_IMAGE :
                            DrawImageLayer( pLayer );
                            break;
                        case L_LAYER :
                            DrawLayer( pMap, pLayer );
                            break;
                        case L_NONE :
                            // TODO: FINISH HIM !!!
                            break;
                    }
                }

                pLayer = pLayer -> next;
            }
        }

        /**
         * Render all map objects.
         */
        void TileMapRenderer :: RenderMap( void ) {

            if( m_bClearBackground )
                ClearBackground( IntToColor( m_pTmxMap ? m_pTmxMap -> backgroundcolor : m_nWindowBackgroundColor ) );

            if( m_pTmxMap )
                DrawAllLayers( m_pTmxMap, m_pTmxMap -> ly_head );

            if( m_bDrawFPS )
                DrawFPS( 0, 0 );
        }

        /**
         * Initialize default input event handlers.
         */
        void TileMapRenderer :: InitalizeDefaultUserInputHandlers( void )  {

            // Default Keyboard handlers 
            SetUserKeyEventHandler( SunLight :: Input :: KEY_UP, std :: bind( &TileMapRenderer :: InputKeyUp, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_DOWN, std :: bind( &TileMapRenderer :: InputKeyDown, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_LEFT, std :: bind( &TileMapRenderer :: InputKeyLeft, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_RIGHT, std :: bind( &TileMapRenderer :: InputKeyRight, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_UP, std :: bind( &TileMapRenderer :: InputKeyPageUp, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_PAGE_DOWN, std :: bind( &TileMapRenderer :: InputKeyPageDown, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_HOME, std :: bind( &TileMapRenderer :: InputKeyHome, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserKeyEventHandler( SunLight :: Input :: KEY_END, std :: bind( &TileMapRenderer :: InputKeyEnd, this, std ::placeholders::_1, std ::placeholders::_2 ) );

            // Default GamePad handlers
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_UP, std :: bind( &TileMapRenderer :: InputKeyUp, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_DOWN, std :: bind( &TileMapRenderer :: InputKeyDown, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_LEFT, std :: bind( &TileMapRenderer :: InputKeyLeft, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT, std :: bind( &TileMapRenderer :: InputKeyRight, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_UP, std :: bind( &TileMapRenderer :: InputKeyPageUp, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN, std :: bind( &TileMapRenderer :: InputKeyPageDown, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_LEFT, std :: bind( &TileMapRenderer :: InputKeyHome, this, std ::placeholders::_1, std ::placeholders::_2 ) );
            SetUserGamePadEventHandler( SunLight :: Input :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, std :: bind( &TileMapRenderer :: InputKeyEnd, this, std ::placeholders::_1, std ::placeholders::_2 ) );
        }

        /**
         * @brief Handle default key Up event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyUp( SunLight :: Input :: ControllerType type, int nId )  {

            MoveCameraUp();
        }

        /**
         * @brief Handle default key Down event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyDown( SunLight :: Input :: ControllerType type, int nId )  {

            MoveCameraDown();
        }

        /**
         * @brief Handle default key Left event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyLeft( SunLight :: Input :: ControllerType type, int nId )  {

            MoveCameraLeft();
        }

        /**
         * @brief Handle default key Right event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyRight( SunLight :: Input :: ControllerType type, int nId )  {

            MoveCameraRight();
        }

        /**
         * @brief Handle default key Page Up event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyPageUp( SunLight :: Input :: ControllerType type, int nId )  {

            ZoomIn();
        }

        /**
         * @brief Handle default key Page Down event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyPageDown( SunLight :: Input :: ControllerType type, int nId )  {

            ZoomOut();
        }

        /**
         * @brief Handle default key Home event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyHome( SunLight :: Input :: ControllerType type, int nId )  {

            ResetZoom();
        }

        /**
         * @brief Handle default key End event;
         * 
         * @param type The controller that is triggering this event;
         * @param Id Rhe Id of of this controller type (eg. Game Pad Id); 
         */
        void TileMapRenderer :: InputKeyEnd( SunLight :: Input :: ControllerType type, int nId )  {

            ResetCamera();
        }

        /**
         * @brief Handle user keyboard events.
         * @param evt The event to be handled;
         */
        void TileMapRenderer :: HandleKeyEvent( SunLight :: Input :: KeyboardKey evt )  {

            for( InputEventHandlerList :: iterator itItem = m_KeyInputEventHandlerList.begin(); itItem != m_KeyInputEventHandlerList.end(); itItem++ )  {
                if( ( *itItem ) -> nEvent == evt )
                    ( *itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_KEYBOARD, KEYBOARD_ID );
            }
        }
       
        /**
         * Check user input selected previously by user (mouse,
         * joystick, keyboard, etc...)
         */
        void TileMapRenderer :: HandleUserInput( void )  {
            /*
             *  Controller Axis dead zones
             */
            const float ctfLeftStickDeadzoneX  = 0.1f;
            const float ctfLeftStickDeadzoneY  = 0.1f;
            const float ctfRightStickDeadzoneX = 0.1f;
            const float ctfRightStickDeadzoneY = 0.1f;
            const float ctLeftTriggerDeadzone  = -0.9f;
            const float ctRightTriggerDeadzone = -0.9f;

            bool    bEventHandled = false;

            for( int nGamePadId : m_GamePadList )  {
                SunLight :: Input :: GamepadButton button[]    = { SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_UNKNOWN, 
                                                                   SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_UNKNOWN };
                float    fPosX = m_pInputHandler -> GetGamepadAxisMovement( nGamePadId, 
                                                                            SunLight :: Input :: GamepadAxis :: GAMEPAD_AXIS_LEFT_X );
                float    fPosY = m_pInputHandler -> GetGamepadAxisMovement( nGamePadId, 
                                                                            SunLight :: Input :: GamepadAxis :: GAMEPAD_AXIS_LEFT_Y );

                // Calculate deadzones
                if( ( fPosX > -ctfLeftStickDeadzoneX ) && ( fPosX < ctfLeftStickDeadzoneX ) ) 
                    fPosX = 0.0f;
                if( ( fPosY > -ctfRightStickDeadzoneY ) && ( fPosY < ctfRightStickDeadzoneY ) ) 
                    fPosY = 0.0f;

                // TODO: Code sample for future use when add support to Right stick and trigger handling  
                // if (rightStickX > -ctfRightStickDeadzoneX && rightStickX < ctfRightStickDeadzoneX) rightStickX = 0.0f;
                // if (rightStickY > -rightStickDeadzoneY && rightStickY < rightStickDeadzoneY) rightStickY = 0.0f;
                // if (leftTrigger < leftTriggerDeadzone) leftTrigger = -1.0f;
                // if (rightTrigger < rightTriggerDeadzone) rightTrigger = -1.0f;

                // Handle Analog GamePad control stick
                if( fPosX > 0.0 )  {
                    button[0] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
                    bEventHandled = true;
                }
                else  {
                    if( fPosX < 0.0 )  {
                        button[0] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_LEFT;
                        bEventHandled = true;
                    }
                }

                if( fPosY > 0.0 )  {
                    button[1] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_DOWN;
                    bEventHandled = true;
                }
                else  {
                    if( fPosY < 0.0 )  {
                        button[1] = SunLight :: Input :: GamepadButton :: GAMEPAD_BUTTON_LEFT_FACE_UP;
                        bEventHandled = true;
                    }
                }

                // Handle DPad control stick
                for( InputEventHandlerList :: iterator itItem = m_GPadInputEventHandlerList.begin(); itItem != m_GPadInputEventHandlerList.end(); itItem++ )  {
                    SunLight :: Input :: GamepadButton   evt = ( SunLight :: Input :: GamepadButton ) ( * itItem ) -> nEvent;
                    
                    /*
                     * DPad and Analog MUST be processed in the same handler list iteration.
                     * The order of events can affect main renderer processing behavior 
                     * (depending on final developer game logic);
                     */
                    if( m_pInputHandler -> IsGamepadButtonDown( nGamePadId, evt ) )  {
                        ( * itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_GAMEPAD, nGamePadId );
                        bEventHandled = true;
                    }
                    else  {
                        if( ( evt == button[0] ) || ( evt == button[1] ) )  {
                            ( * itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_GAMEPAD, nGamePadId );
                            bEventHandled = true;
                        }
                    }
                }
            }

            // Handle keyboard if there's no response from game pad
            if( !bEventHandled )  {
                switch( m_ViewControlMode )  {
                    case VIEW_CONTROL_MODE_REACTIVE :
                        HandleKeyEvent( m_pInputHandler -> GetKeyPressed() );
                        bEventHandled = true;
                    break;

                    case VIEW_CONTROL_MODE_ACTIVE :  {
                        for( InputEventHandlerList :: iterator itItem = m_KeyInputEventHandlerList.begin(); itItem != m_KeyInputEventHandlerList.end(); itItem++ )  {
                            if( m_pInputHandler -> IsKeyDown( ( SunLight :: Input :: KeyboardKey ) ( * itItem ) -> nEvent ) )  {
                                ( * itItem ) -> pHandler( SunLight :: Input :: ControllerType :: CONTROLLER_KEYBOARD, KEYBOARD_ID );
                                bEventHandled = true;
                            }
                        }
                    }
                }
            }

            if( !bEventHandled && ( m_pNullInputEventHandler != nullptr ) )  {
                m_pNullInputEventHandler( SunLight :: Input  :: CONTROLLER_NULL, -1 );
            }
        }

        /**
         * Handle user updates from user listeners;
         */
        void TileMapRenderer :: HandleUserUpdate( void )  {

            for( SunLight :: TileMap :: ITileMapListener* pListener : m_TileMapListenerList )  {
                pListener -> OnUpdate( *this );

                // Update all sprites owned by this renderer
                for( SunLight :: Sprite :: Sprite* pSprite : m_SpriteList )  {
                    pSprite -> Update();
                }
            }
        }

        /**
         * Handle user collisions;
         */
        void TileMapRenderer :: HandleUserCollisions( void )  {

            m_CollisionManager.Update();
        }

        /**
         * Copy user layer to tmx layer.
         * @param pTmxLayer Pointer to Tmx that data will be copied to;
         * @param layer User struct whose layer data will be copied from;
         */
        void TileMapRenderer :: CopyLayerToTmx( tmx_layer *pTmxLayer, SunLight :: TileMap :: stLayer& layer )  {

            pTmxLayer -> opacity = ( __MAX_OPACITY_LEVEL - layer.nOpacity );
            pTmxLayer -> offsetx = layer.offset.x;
            pTmxLayer -> offsety = layer.offset.y;
            pTmxLayer -> visible = layer.bVisible;
            layer.pLayer         = pTmxLayer;
        }

        /**
         * Copy tmx layer to user layer.
         * @param pTmxLayer Reference to user layer that data will be copied to;
         * @param layer Pointer whose tmx layer data will be copied from;
         */
        void TileMapRenderer :: CopyTmxToLayer( SunLight :: TileMap :: stLayer& layer, tmx_layer *pTmxLayer )  {

            layer.nOpacity = ( int ) ( pTmxLayer -> opacity + __MAX_OPACITY_LEVEL );
            layer.offset.x = pTmxLayer -> offsetx;
            layer.offset.y = pTmxLayer -> offsety;
            layer.bVisible = pTmxLayer -> visible;
            layer.pLayer         = pTmxLayer;
        }

        /**
         * Constructor. Initialize all class data.
         * @param fWidth Screen renderer width;
         * @param fHeight Screen renderer height;
         * @param szTitle Screen renderer title;
         * @param nTargetFps Renderer desired FPS;
         * @param bUseDefaultKeyHandler Inform if the default keyboard and
         * camera handler will be used (at the end this parameter will be remove
         * when user interaction be a little bit more clear.
         */
        TileMapRenderer :: TileMapRenderer( float fWidth,
                                            float fHeight,
                                            const char *szTitle,
                                            int nTargetFps,
                                            bool bUseDefaultKeyHandler) :
                                            m_CollisionManager( this )  {

            GetDimension2D().size.nWidth  = ( int ) fWidth;
            GetDimension2D().size.nHeight = ( int ) fHeight;
            m_nMapWidth                   = 0;
            m_nMapHeight                  = 0;
            m_fWindowWidth                = fWidth;
            m_fWindowHeight               = fHeight;
            m_nTargetFps                  = nTargetFps;
            m_strTitle                    = szTitle;
            m_pTmxMap                     = NULL;
            m_bIsStarted                  = false;
            m_bWindowResizeable           = __DEFAULT_RESIZEABLE_STATUS;
            m_bClearBackground            = __DEFAULT_CLEAR_BACKGROUND;
            m_bDrawFPS                    = __DEFAULT_DRAW_FPS_STATUS;
            m_nScrollStepWidth            = __DEFAULT_SCROLL_STEP_WIDTH;
            m_nScrollStepHeight           = __DEFAULT_SCROLL_STEP_HEIGHT;
            m_ViewControlMode             = __DEFAULT_VIEW_CONTROL_MODE;
            m_nWindowBackgroundColor      = __DEFAULT_WINDOW_BK_COLOR;
            m_pInputHandler               = SunLight :: Input :: InputHandlerFactory :: CreateInputHandler();
            m_pNullInputEventHandler      = nullptr;
            m_TileMapListenerList.clear();
            m_KeyInputEventHandlerList.clear();
            m_GPadInputEventHandlerList.clear();
            m_SpriteList.clear();
            m_GamePadList.clear();

            memset( &m_CameraPos, 0, sizeof( m_CameraPos ) );
            ResetZoom();
            SetVisible( __DEFAULT_VISIBLE_STATUS );

            if( bUseDefaultKeyHandler )  {
                InitalizeDefaultUserInputHandlers();
            }
         
            /*
            * Set the raylib callback texture handlers (this call is protected
            * to be called just only one time in whole program execution).
            */
            if( !m_bInitialized )  {
                tmx_img_load_func = TextureLoaderCallback;
                tmx_img_free_func = TextureFreeCallback;
                m_bInitialized    = true;
            }
        }

        /**
         * Destructor. Finalize all class data.
         */
        TileMapRenderer :: ~TileMapRenderer( void )  {

            UnloadMap();
            m_TileMapListenerList.clear();

            for( InputEventHandlerList :: iterator itItem = m_KeyInputEventHandlerList.begin(); itItem != m_KeyInputEventHandlerList.end(); itItem++ )  {
                delete  ( * itItem );
            }

            for( InputEventHandlerList :: iterator itItem = m_GPadInputEventHandlerList.begin(); itItem != m_GPadInputEventHandlerList.end(); itItem++ )  {
                delete  ( * itItem );
            }

            m_KeyInputEventHandlerList.clear();
            m_GPadInputEventHandlerList.clear();

            delete m_pInputHandler;
        }

        /**
         * Add a TileMap listener to internal listener list renderer.
         * All listeners will be called for each update step;
         * @param pListener Pointer to the @ITileMapListener object to add;
         */
        void TileMapRenderer :: AddTileMapListener( SunLight :: TileMap :: ITileMapListener *pListener )  {

            TileMapListenerList :: iterator itItem = std :: find( m_TileMapListenerList.begin(),
                                                                  m_TileMapListenerList.end(),
                                                                  pListener );

            if( itItem == m_TileMapListenerList.end() )
                m_TileMapListenerList.push_back( pListener );
        }

        /**
         * Remove a TileMap listener from internal listener list renderer.
         * @param pListener Pointer to the @ITileMapListener object to remove;
         */
        void TileMapRenderer :: RemoveTileMapListener( SunLight :: TileMap :: ITileMapListener *pListener )  {

            TileMapListenerList :: iterator itItem = std :: find( m_TileMapListenerList.begin(),
                                                                  m_TileMapListenerList.end(),
                                                                  pListener );

            if( itItem != m_TileMapListenerList.end() )
                m_TileMapListenerList.erase( itItem );
        }

        /**
         * Set exit key to leave the renderer when it is in running state;
         * @param key The key code representing the exit key (check raylib
         * KeyboardKey enum);
         * The default exit is ESC Key;
         */
        void TileMapRenderer :: SetExitKey( KeyboardKey key )  {

            ::SetExitKey( key );
        }

        /**
         * Set window resizeable status.
         * @param bResizeable The new resizeable status for window;
         */
        void TileMapRenderer :: SetWindowResizeable( bool bResizeable )  {

            m_bWindowResizeable = bResizeable;
        }

        /**
         * Set the window background color. This color is used when there's no color on any layer or
         * when there's no map loaded.
         * @param nWindowBackgroundColor The background color to set;
         */
        void TileMapRenderer :: SetWindowBackgroundColor( uint32_t nWindowBackgroundColor )  {

            m_nWindowBackgroundColor = nWindowBackgroundColor;
        }

        /**
         * Set the status of background cleaning for each
         * rendering cycle;
         * @param bStatus The new background clear status settings;
         */
        void TileMapRenderer :: SetClearBackground( bool bStatus )  {

            m_bClearBackground = bStatus;
        }

        /**
         * Enable/disable draw FPS information on top left corner of screen
         * @param bDrawFPS The new draw FPS status;
         */
        void TileMapRenderer :: SetDrawFPS( bool bDrawFPS )  {

            m_bDrawFPS = bDrawFPS;
        }

        /**
         * Set the view port control mode;
         * Viewport control mode (active and reactive)
         * Active, the view port reacts to a single key pressing continuously;
         * Reactive, the view port reacts only for each key pressing;
         * @param mode The new view control mode;
         */
        void TileMapRenderer :: SetViewControlMode( SunLight :: Renderer :: ViewControlMode mode )  {

            m_ViewControlMode = mode;
        }

        /**
         * Set the default scroll step size.
         * @param nStepWidth The new scroll step size Width
         * (-1 uses the map tile size width);
         * @param nStepHeight The new scroll step size Height
         * (-1 uses the map tile size height);
         */
        void TileMapRenderer :: SetScrollStepSize( int nStepWidth, int nStepHeight )  {

            m_nScrollStepWidth  = nStepWidth;
            m_nScrollStepHeight = nStepHeight;
        }

        /**
         * @brief Set the Game Pad Id that will be handled by this renderer;
         * 
         * @param nIndex The GamePad device index to set GamePadId;  
         * @param nGamePadId The gme pad id to set;
         */
        void TileMapRenderer :: AddGamePad( int nGamePadId )  {

            m_GamePadList.push_back( nGamePadId );
        }

        /**
         * @brief Set the User Key Event Handler callback routine.
         * 
         * @param evt The event code to call related routine;
         * @param handler The callback routine that will be called for key event;
         */
        void TileMapRenderer :: SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt, 
                                                        SunLight :: Input :: INPUT_EVENT_HANDLER handler )  {

            if( evt != SunLight :: Input :: KeyboardKey :: KEY_NULL )  {
                __stInputEventData *pEvtData = new __stInputEventData;

                pEvtData -> nEvent   = evt;
                pEvtData -> pHandler = handler;
                m_KeyInputEventHandlerList.push_back( pEvtData );
            }
            else  {
                m_pNullInputEventHandler = handler;
            }
        }

        /**
          * @brief Set the User Game Pad button event Handler callback routine.
          * 
          * @param evt The event code to call related routine;
          * @param handler The callback routine that will be called for key event;
          */
        void TileMapRenderer :: SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt, 
                                                            SunLight :: Input :: INPUT_EVENT_HANDLER handler )  {

            __stInputEventData *pEvtData = new __stInputEventData;

            pEvtData -> nEvent   = evt;
            pEvtData -> pHandler = handler;
            m_GPadInputEventHandlerList.push_back( pEvtData );
        }

        /**
         * @brief Get the Input Handler object
         * 
         * @return S\The reference to IInputHandler object;
         */
        SunLight :: Input :: IInputHandler& TileMapRenderer :: GetInputHandler( void )  {

            return *m_pInputHandler;
        }

        /**
         * Reset zoom to it's default state.
         */
        void TileMapRenderer :: ResetZoom( void ) {

            GetViewport().ResetZoom();
        }

        /**
         * Performs Zoom In effect.
         */
        void TileMapRenderer :: ZoomIn( void ) {

            GetViewport().ZoomIn();
        }

        /**
         * Performs Zoom Out effect.
         */
        void TileMapRenderer :: ZoomOut( void ) {

            GetViewport().ZoomOut();
        }

        /**
         * Reset the camera position.
         */
        void TileMapRenderer :: ResetCamera( void )  {

            m_CameraPos.x = 0;
            m_CameraPos.y = 0;
        }

        /**
         * Move view camera up.
         */
        void TileMapRenderer :: MoveCameraUp( void )  {

            SunLight :: Base :: Viewport&  vp = GetViewport();
            int                  nMapBoundary = ( int ) std :: round( ( m_CameraPos.y - 
                                                                        m_nScrollStepHeight + 
                                                                        m_nMapHeight +
                                                                        m_pTmxMap -> tile_height ) * 
                                                                        vp.GetZoomProperties().fZoomFactor );

            if( vp.GetDimension2D().size.nWidth < nMapBoundary )
                m_CameraPos.y-=m_nScrollStepHeight;
        }

        /**
         * Move view camera down.
         */
        void TileMapRenderer :: MoveCameraDown( void )  {

            int    nMapBoundary = ( int ) ( m_CameraPos.y + m_nScrollStepHeight );

            if( nMapBoundary < 0 )
                m_CameraPos.y+=m_nScrollStepHeight;
        }

        /**
         * Move view camera left.
         */
        void TileMapRenderer :: MoveCameraLeft( void )  {

            SunLight :: Base :: Viewport&  vp = GetViewport();
            int                  nErrorFix    = ( m_pTmxMap -> tile_width / std :: round( vp.GetZoomProperties().fZoomFactor ) );
            int                  nMapBoundary = ( int ) std :: round( ( m_CameraPos.x - 
                                                                        m_nScrollStepWidth + 
                                                                        m_nMapWidth
                                                                        + nErrorFix ) * 
                                                                        vp.GetZoomProperties().fZoomFactor );

            if( vp.GetDimension2D().size.nWidth < nMapBoundary )
                m_CameraPos.x-=m_nScrollStepWidth;
        }

        /**
         * Move view camera right.
         */
        void TileMapRenderer :: MoveCameraRight( void )  {

            int nMapBoundary = ( int ) ( m_CameraPos.x + m_nScrollStepWidth );

            if( nMapBoundary < 0 )
                m_CameraPos.x+=m_nScrollStepWidth;
        }

        /**
         * Set layer parameters.
         * @param nLayerId The layer id to set layer parameters;
         * @param layer reference to layer parameters structure to set;
         */
        bool TileMapRenderer :: SetLayer( int nLayerId, SunLight :: TileMap :: stLayer &layer )  {

            tmx_layer *pTmxLayer = GetLayer( nLayerId );

            if( pTmxLayer )  {
                CopyLayerToTmx( pTmxLayer, layer );
                return true;
            }

            return false;
        }

        /**
         * Set layer parameters.
         * @param szLayerName The layer name to set layer parameters;
         * @param layer reference to layer parameters structure to set;
         */
        bool TileMapRenderer :: SetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer )  {

            tmx_layer *pTmxLayer = GetLayer( szLayerName );

            if( pTmxLayer )  {
                CopyLayerToTmx( pTmxLayer, layer );
                return true;
            }

            return false;
        }

        /**
         * Get the layer parameters.
         * @param nLayerId The layer id to get layer parameters;
         * @param layer reference to layer parameters structure to get;
         */
        bool TileMapRenderer :: GetLayer( int nLayerId, SunLight :: TileMap :: stLayer &layer )  {

            tmx_layer *pTmxLayer = GetLayer( nLayerId );

            if( pTmxLayer )  {
                CopyTmxToLayer( layer, pTmxLayer );
                return true;
            }

            return false;
        }

        /**
         * Get the layer parameters.
         * @param szLayerName The layer name to get layer parameters;
         * @param layer reference to layer parameters structure to get;
         */
        bool TileMapRenderer :: GetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer )  {

            tmx_layer *pTmxLayer = GetLayer( szLayerName );

            if( pTmxLayer )  {
                CopyTmxToLayer( layer, pTmxLayer );
                return true;
            }

            return false;
        }

        /**
         * Get a tile based row and column on specified map layer;
         * @param pos The tile position on layer map.
         * @param layer Reference to the layer whose tile will be
         * retrieved;
         * @param tile reference to @link stTile object to receive
         * tile information;
         */
        bool TileMapRenderer :: GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                                         const SunLight :: TileMap :: stLayer& layer,
                                         SunLight :: TileMap :: stTile& tile ) {

            tile.nGID = layer.pLayer -> content.gids[( pos.nTileRow *
                                                     m_pTmxMap -> width ) +
                                                     pos.nTileCol] &
                                                    TMX_FLIP_BITS_REMOVAL;
            tile.pTile = ( m_pTmxMap -> tiles ? m_pTmxMap -> tiles[tile.nGID] : NULL );

            if( !tile.pTile )  {
                tile.nGID  = 0;
                tile.pTile = NULL;

                return false;
            }

            SunLight :: TileMap :: stDimension2D&  vp = GetDimension2D();

            tile.dimension.pos.x        = ( ( pos.nTileCol * m_pTmxMap -> tile_width ) +
                                            vp.pos.x ) + ( int ) m_CameraPos.x;
            tile.dimension.pos.y        = ( ( pos.nTileRow * m_pTmxMap -> tile_height ) +
                                            vp.pos.y ) + ( int ) m_CameraPos.y;
            tile.dimension.size.nWidth  = ( m_pTmxMap -> tile_width );
            tile.dimension.size.nHeight = ( m_pTmxMap -> tile_height );

            return true;
        }

        /**
         * Convert world coordinate to tile matrix coordinate.
         * @param coord The World coordinate to translate to tile matrix coordinate;
         * @param pos Reference to struct @link stMatrixPosition to receive the
         * tile matrix position based on world coordinate passed as parameter;
         */
        bool TileMapRenderer :: TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                                     SunLight :: TileMap :: stMatrixPosition& pos )  {

            if( m_pTmxMap )  {
                SunLight :: TileMap :: stDimension2D&  vp      = GetViewport().GetDimension2D();
                SunLight :: Base :: stZoomProperties&  zp      = GetViewport().GetZoomProperties();
                int                                    nCoordX = ( int ) ( coord.x / zp.fZoomFactor );
                int                                    nCoordY = ( int ) ( coord.y / zp.fZoomFactor );


                if( ( ( nCoordX >= 0 ) && ( nCoordX < m_nMapWidth ) ) &&
                    ( ( nCoordY >= 0 ) && ( nCoordY < m_nMapHeight ) ) ) {
                    pos.nTileCol = ( int ) ( ( ( coord.x + vp.pos.x ) -
                                             m_CameraPos.x ) / m_pTmxMap -> tile_width );
                    pos.nTileRow = ( int ) ( ( ( coord.y + vp.pos.x ) -
                                             m_CameraPos.y ) / m_pTmxMap -> tile_height );

                    return ( ( pos.nTileCol >= 0 ) && ( pos.nTileRow >=0 ) );
                }
            }

            return false;
        }

        /**
         * Load the specified map to renderer.
         * @param szTmxMapFile Renderer map file;
         * @param alignment Map alignment according @link MapAlignment enumerator;
         */
        bool TileMapRenderer :: LoadMap( const char *szTmxMapFile, SunLight :: TileMap :: ITileMap :: MapAlignment alignment )  {

            if( m_bIsStarted )  {
                m_pTmxMap = ::tmx_load( szTmxMapFile );

                if( !m_pTmxMap ) {
                    ::tmx_perror( "Cannot load map" );
                    return false;
                }

                m_nMapWidth  = ( m_pTmxMap -> width * m_pTmxMap -> tile_width );
                m_nMapHeight = ( m_pTmxMap -> height * m_pTmxMap -> tile_height );

                SunLight :: TileMap :: stDimension2D vp          = GetViewport().GetDimension2D();
                float                                fZoomFactor = GetViewport().GetZoomProperties().fZoomFactor;
                int                                  nVpHeight   = std :: abs( ( ( m_nMapHeight / vp.size.nHeight ) * vp.size.nHeight ) - m_nMapHeight );


                // Setup alignment
                switch( alignment & 0xFF )  {
                    case MAP_ALIGNMENT_TOP_RIGHT :
                        m_CameraPos.x = ( float ) -( m_nMapWidth - vp.size.nWidth + vp.pos.x );
                        break;

                    case MAP_ALIGNMENT_TOP_LEFT : // Nothing to do
                        break;

                    case MAP_ALIGNMENT_BOTTOM_RIGHT :
                        m_CameraPos.x = ( float ) -( m_nMapWidth - vp.size.nWidth + vp.pos.x );
                        m_CameraPos.y = ( float ) -( m_nMapHeight - nVpHeight + vp.pos.y );
                        break;

                    case MAP_ALIGNMENT_BOTTOM_LEFT :
                        m_CameraPos.y = ( float ) -( m_nMapHeight - vp.size.nHeight + vp.pos.y );
                        break;

                    case MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM :
                        m_CameraPos.x = -round( ( ( ( ( m_nMapWidth * fZoomFactor ) -
                                                    vp.size.nWidth + vp.pos.x ) / 2 ) / fZoomFactor ) );
                        m_CameraPos.y = ( float ) -( m_nMapHeight - nVpHeight + vp.pos.y );
                        break;

                    case MAP_ALIGNMENT_CENTER_WIDTH_TOP :
                        m_CameraPos.x = -round( ( ( ( ( m_nMapWidth * fZoomFactor ) -
                                                    vp.size.nWidth + vp.pos.x ) / 2 ) / fZoomFactor ) );
                        break;

                    case MAP_ALIGNMENT_CENTER_HEIGHT_LEFT :
                        m_CameraPos.y = -round( ( ( ( ( m_nMapHeight * fZoomFactor ) -
                                                    vp.size.nHeight + vp.pos.y ) / 2 ) / fZoomFactor ) );
                        break;

                    case MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT :
                        m_CameraPos.x = ( float ) -( m_nMapWidth - vp.size.nWidth + vp.pos.x );
                        m_CameraPos.y = -round( ( ( ( ( m_nMapHeight * fZoomFactor ) -
                                                    vp.size.nHeight + vp.pos.y ) / 2 ) / fZoomFactor ) );
                        break;

                    case MAP_ALIGNMENT_CENTER :
                        m_CameraPos.x = -round( ( ( ( ( m_nMapWidth * fZoomFactor ) -
                                                    vp.size.nWidth + vp.pos.x ) / 2 ) / fZoomFactor ) );
                        m_CameraPos.y = -round( ( ( ( ( m_nMapHeight * fZoomFactor ) -
                                                    vp.size.nHeight + vp.pos.y ) / 2 ) / fZoomFactor ) );
                        break;
                }

                /*
                * Set scrolling properties.
                */
                SetScrollStepSize( ( m_nScrollStepWidth < 0 ? m_pTmxMap -> tile_width :
                                                            m_nScrollStepWidth ),
                                   ( m_nScrollStepHeight < 0 ? m_pTmxMap -> tile_height :
                                                            m_nScrollStepHeight ) );

                return true;
            }

            return false;
        }

        /**
         * Unload a previously loaded map and it's related data (animations, etc...);
         */
        bool TileMapRenderer :: UnloadMap( void )  {

            if( m_pTmxMap )  {

                for( SunLight :: Sprite :: Sprite* pSprite : m_SpriteList )  {
                    pSprite -> Unload();
                }

                ::tmx_map_free( m_pTmxMap );

                /*
                * Release all allocated animations data structure.
                */
                __AnimInfoList :: iterator itItem = m_AnimInfoList.begin();

                while( itItem != m_AnimInfoList.end() )  {
                    delete *itItem;
                    itItem = m_AnimInfoList.erase( itItem );
                }

                m_pTmxMap    = NULL;
                m_nMapWidth  = 0;
                m_nMapHeight = 0;

                return true;
            }

            return false;
        }

        /**
         * Get the current map information data.
         * @param mapInfo Reference to the struct @link stMapInfo that will
         * receive the map information.
         */
        bool TileMapRenderer :: GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo )  {

            if( m_pTmxMap )  {
                mapInfo.mapSize.nWidth   = m_pTmxMap -> width;
                mapInfo.mapSize.nHeight  = m_pTmxMap -> height;
                mapInfo.tileSize.nWidth  = m_pTmxMap -> tile_width;
                mapInfo.tileSize.nHeight = m_pTmxMap -> tile_height;
                mapInfo.pMap = m_pTmxMap;

                return true;
            }

            return false;
        }

        /**
         * Return the reference to internal renderer collision manager.
         */
        SunLight :: Collision :: ICollisionManager& TileMapRenderer :: GetCollisionManager( void )  {

            return m_CollisionManager;
        }

        /**
         * Add a sprite to world.
         * @param nLayerId Id of an existing Layer on tiled map whose sprite will be added;
         * @param sprite Reference to the sprite that will be added;
         */
        bool TileMapRenderer :: AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite )  {

            if( m_bIsStarted && GetLayer( nLayerId ) )  {
                SpriteList :: iterator itItem = std :: find( m_SpriteList.begin(),
                                                            m_SpriteList.end(),
                                                            &sprite );

                if( itItem == m_SpriteList.end() )  {
                    sprite.SetParent( this );
                    m_CollisionManager.AddCollider( nLayerId, &sprite.GetCollider() );
                    m_SpriteList.push_back( &sprite );
                    return true;
                }
            }

            return false;
        }

        /**
         * Remove a sprite from world.
         * @param nLayerId Id of an existing layer on tiled map whose sprite was previously added by 
         * @see AddSprite and now will be removed;
         * @param sprite Reference to the sprite that will be removed;
         */
        bool TileMapRenderer :: RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite )  {

            if( m_bIsStarted )  {
                SpriteList :: iterator itItem = std :: find( m_SpriteList.begin(),
                                                            m_SpriteList.end(),
                                                            &sprite );

                if( itItem != m_SpriteList.end() )  {
                    m_CollisionManager.RemoveCollider( nLayerId, &sprite.GetCollider() );
                    m_SpriteList.erase( itItem );

                    return true;
                }
            }

            return false;
        }

        /**
         * Start engine renderer.
         */
        bool TileMapRenderer :: Start( void )  {

            if( m_bWindowResizeable )
                SetConfigFlags( FLAG_WINDOW_RESIZABLE );

            InitWindow( ( int ) m_fWindowWidth,
                        ( int ) m_fWindowHeight,
                        m_strTitle.c_str() );

            if( !IsWindowReady() ) {
                ::tmx_perror( "Cannot create a window" );
                return false;
            }

            SetExitKey( __DEFAULT_EXIT_KEY );
            SetTargetFPS( m_nTargetFps != -1 ? m_nTargetFps : __DEFAULT_FPS );
            m_bIsStarted = true;

            return m_bIsStarted;
        }

        /**
         * Stop renderer freeing all allocated resources.
         */
        void TileMapRenderer :: Stop( void )  {

            for( SunLight :: TileMap :: ITileMapListener* pListener : m_TileMapListenerList )  {
                pListener -> OnStop();
            }

            if( m_bIsStarted )  {
                UnloadMap();
                CloseWindow();
                m_bIsStarted = false;
            }
        }

        /**
         * Run renderer.
         */
        bool TileMapRenderer :: Run( void )  {

            if( m_bIsStarted )  {
                while ( !WindowShouldClose() ) {
                    BeginDrawing();
                    if( GetVisible() )  {
                        RenderMap();
                        HandleUserInput();
                        HandleUserUpdate();
                        HandleUserCollisions();
                    }
                    EndDrawing();
                }

                return true;
            }

            return false;
        }
    }
}
