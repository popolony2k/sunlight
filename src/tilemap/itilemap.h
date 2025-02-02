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

#ifndef __ITILEMAP_H__
#define __ITILEMAP_H__

#include "sprite/sprite.h"
#include "input/iinputhandler.h"
#include "collision/icollisionmanager.h"


namespace SunLight {
    namespace TileMap  {

        /**
         * @brief TileMap interface with basic operations 
         * inside the game world.
         */
        class ITileMap : public SunLight :: Canvas :: BaseCanvas {

            public:

            /**
             * @brief Viewport based map alignment.
             */
            enum MapAlignment  {
                MAP_ALIGNMENT_CENTER              = 0,
                MAP_ALIGNMENT_TOP_RIGHT           = 1,
                MAP_ALIGNMENT_TOP_LEFT            = 2,
                MAP_ALIGNMENT_BOTTOM_RIGHT        = 4,
                MAP_ALIGNMENT_BOTTOM_LEFT         = 8,
                MAP_ALIGNMENT_CENTER_WIDTH_TOP    = 16,
                MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM = 32,
                MAP_ALIGNMENT_CENTER_HEIGHT_LEFT  = 64,
                MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT = 128
            };


            virtual ~ITileMap( void ) {};

            /**
             * @brief Set the Game Pad Id that will be handked by this renderer;
             * 
             * @param nGamePadId The gme pad id to set;
             */
            virtual void AddGamePad( int nGamePadId ) = 0;

            /**
             * @brief Calibrate all registered GamePads through @see AddGamePad.
             */
            virtual void CalibrateGamePads( void ) = 0;

            /**
             * @brief Set the User Key Event Handler callback routine.
             * 
             * @param evt The event code to call related routine;
             * @param handler The callback routine that will be called for key event;
             */
            virtual void SetUserKeyEventHandler( SunLight :: Input :: KeyboardKey evt, 
                                                 SunLight :: Input :: INPUT_EVENT_HANDLER handler ) = 0;

           /**
             * @brief Set the User Game Pad button event Handler callback routine.
             * 
             * @param evt The event code to call related routine;
             * @param handler The callback routine that will be called for key event;
             */
            virtual void SetUserGamePadEventHandler( SunLight :: Input :: GamepadButton evt, 
                                                     SunLight :: Input :: INPUT_EVENT_HANDLER handler ) = 0;

            /**
             * @brief Get the Input Handler object
             * 
             * @return S\The reference to IInputHandler object;
             */
            virtual SunLight :: Input :: IInputHandler& GetInputHandler( void ) = 0;

            /**
             * @brief Reset zoom to it's default state.
             */
            virtual void ResetZoom( void ) = 0;

            /**
             * @brief Performs Zoom In effect.
             */
            virtual void ZoomIn( void ) = 0 ;

            /**
             * @brief Performs Zoom Out effect.
             */
            virtual void ZoomOut( void ) = 0;

            /**
             * Reset the camera position.
             */
            virtual void ResetCamera( void ) = 0;

            /**
             * Move view camera up.
             */
            virtual void MoveCameraUp( void ) = 0;

            /**
             * Move view camera down.
             */
            virtual void MoveCameraDown( void ) = 0;

            /**
             * Move view camera left.
             */
            virtual void MoveCameraLeft( void ) = 0;

            /**
             * Move view camera right.
             */
            virtual void MoveCameraRight( void ) = 0;

            /**
             * Set layer parameters.
             * @param nLayerId The layer id to set layer parameters;
             * @param layer reference to layer parameters structure to set;
             */
            virtual bool SetLayer( int nLayerId, SunLight :: TileMap :: stLayer &layer ) = 0;

            /**
             * Set layer parameters.
             * @param szLayerName The layer name to set layer parameters;
             * @param layer reference to layer parameters structure to set;
             */
            virtual bool SetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer ) = 0;

            /**
             * Get the layer parameters.
             * @param nLayerId The layer id to get layer parameters;
             * @param layer reference to layer parameters structure to get;
             */
            virtual bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer &layer ) = 0;

            /**
             * Get the layer parameters.
             * @param szLayerName The layer name to get layer parameters;
             * @param layer reference to layer parameters structure to get;
             */
            virtual bool GetLayer( const char *szLayerName, SunLight :: TileMap :: stLayer &layer ) = 0;

            /**
             * Get a tile based row and column on specified map layer;
             * @param pos The tile position on layer map.
             * @param layer Reference to the layer whose tile will be
             * retrieved;
             * @param tile reference to @link stTile object to receive
             * tile information;
             */
            virtual bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                                  const SunLight :: TileMap :: stLayer& layer,
                                  SunLight :: TileMap :: stTile& tile ) = 0;

            /**
             * Get the tile position based on TileMap absolute coordinate passed as parameter.
             * @param coord The TileMap absolute coordinate to translate to tile position;
             * @param pos Reference to struct @link stTilePosition to receive the
             * tile position based on world coordinate passed as parameter;
             */
            virtual bool TileMapToTileMatrix( const SunLight :: TileMap :: stCoordinate2D& coord,
                                              SunLight :: TileMap :: stMatrixPosition& pos ) = 0;

            /**
             * Get the current map information data.
             * @param mapInfo Reference to the struct @link stMapInfo that will
             * receive the map information.
             */
            virtual bool GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo ) = 0;

            /**
             * Return the reference to internal renderer collision manager.
             */
            virtual SunLight :: Collision :: ICollisionManager& GetCollisionManager( void ) = 0;

            /**
             * Add a sprite to world.
             * @param nLayerId Id of Layer to add sprite;
             * @param sprite Reference to the sprite that will be added;
             */
            virtual bool AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) = 0;

            /**
             * Remove a sprite from world.
             * @param nLayerId Id of Layer to remove sprite;
             * @param sprite Reference to the sprite that will be removed;
             */
            virtual bool RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) = 0;

            /**
             * Load the specified map to renderer.
             * @param szTmxMapFile Renderer map file;
             * @param alignment Map alignment according @link MapAlignment enumerator;
             */
            virtual bool LoadMap( const char *szTxMapFile, 
                                  SunLight :: TileMap :: ITileMap :: MapAlignment alignment = SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_CENTER ) = 0;

            /**
             * Unload a previously loaded map and it's related data (animations,
             * etc...);
             */
            virtual bool UnloadMap( void ) = 0;
        };
    }
}

#endif /* __ITILEMAP_H__ */
