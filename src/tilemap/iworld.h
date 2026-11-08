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

#ifndef __IWORLD_H__
#define __IWORLD_H__

#include "sprite/sprite.h"
#include "collision/icollisionmanager.h"
#include "tilemap/tilemapdefs.h"


namespace SunLight  {
    namespace TileMap  {

        /**
         * @brief A WORLD (I19 of the master plan): what a game's things live in - a map, the sprites
         * registered on its layers, and the collision rules between them. A view shows one world
         * (@see IView::SetWorld); several worlds can be alive at once - split-screen players on
         * different maps, a world map beside the local one, a dungeon in a corner of the screen.
         *
         * Every renderer has a DEFAULT world (id 0, ITileMap::GetDefaultWorld): the one the
         * renderer's own single-map API acts on (LoadMap, AddSprite, GetTile, GetCollisionManager,
         * ...) and the one the default view always shows. More come from ITileMap::CreateWorld,
         * and are shown by extra views. Worlds are isolated: each has its own coordinates (its map's
         * pixels), its own sprites and its own collision rules - a sprite in one never collides
         * with anything in another. A sprite belongs to one world at a time; it moves to another by
         * being removed from one and added to the other (the game converts its position).
         *
         * Every live world updates every frame, shown by a view or not: its tile animations run, its
         * collision rules are tested, and the sprites of a world no view draws still advance once
         * per frame (the default world keeps its older rule: a sprite on a layer no view shows
         * stands still).
         *
         * Handles are std::shared_ptr<IWorld>. A held handle is always SAFE: once its world is
         * removed (ITileMap::RemoveWorld) or the renderer is destroyed, it is INERT (@see IsRemoved)
         * - every call that would act through the renderer does nothing or answers "no" - instead of
         * dangling.
         */
        class IWorld  {

            public:

            virtual ~IWorld( void ) {}

            /**
             * @brief The world's id: 0 for the default world, otherwise the value
             * ITileMap::CreateWorld gave it (ids are never reused).
             */
            virtual int GetId( void ) = 0;

            /**
             * @brief Whether the world was removed from its renderer (ITileMap::RemoveWorld), or its
             * renderer no longer exists.
             */
            virtual bool IsRemoved( void ) = 0;

            /**
             * @brief Load a map into this world, replacing the one it had (only once the new one
             * loaded; a failed load leaves the old one). Its sprites stay registered, as they do
             * across ITileMap::LoadMap. No camera moves: the views showing the world keep theirs
             * (IView::FitToMap or SetCameraPosition place them); a scroll step of -1 (the map's
             * tile size) is resolved for every view that shows it.
             * @param szTmxMapFile The map, a FileSystem path (see IFileSystem);
             * @return false when the renderer is not started, the map cannot be loaded, or the
             * world was removed;
             */
            virtual bool LoadMap( const char* szTmxMapFile ) = 0;

            /**
             * @brief Free the world's map, and DETACH its sprites: each one's registration and
             * collider leave the world, but the sprite itself is neither destroyed nor emptied - the
             * game releases or moves it. (ITileMap::UnloadMap, the older single-map call, also
             * unloads the default world's sprites' textures - Sprite::Unload - and keeps doing so.)
             * @return false when the world had no map, or was removed;
             */
            virtual bool UnloadMap( void ) = 0;

            /**
             * @brief The world's map information, as ITileMap::GetMapInfo.
             * @return false with no map loaded, or once removed;
             */
            virtual bool GetMapInfo( SunLight :: TileMap :: stMapInfo& mapInfo ) = 0;

            /**
             * @brief A layer of the world's map by id or name, as ITileMap::GetLayer.
             * @return false with no such layer, no map, or once removed;
             */
            virtual bool GetLayer( int nLayerId, SunLight :: TileMap :: stLayer& layer ) = 0;
            virtual bool GetLayer( const char* szLayerName, SunLight :: TileMap :: stLayer& layer ) = 0;

            /**
             * @brief A tile of the world's map, as ITileMap::GetTile: its rectangle in the world's
             * map pixels.
             * @return false with no tile there, no map, or once removed;
             */
            virtual bool GetTile( const SunLight :: TileMap :: stMatrixPosition& pos,
                                  const SunLight :: TileMap :: stLayer& layer,
                                  SunLight :: TileMap :: stTile& tile ) = 0;

            /**
             * @brief The cell of the world's map under a position in its map pixels, as
             * ITileMap::TileMapToTileMatrix.
             * @return false off the map's cells, with no map, or once removed;
             */
            virtual bool TileMapToTileMatrix( const SunLight :: Base :: stCoordinate2D& coord,
                                              SunLight :: TileMap :: stMatrixPosition& pos ) = 0;

            /**
             * @brief Register a sprite on a layer of this world, as ITileMap::AddSprite: drawn by
             * every view showing the world, colliding by the world's rules. Refused while the sprite
             * is registered in ANOTHER world (a sprite belongs to one world at a time).
             * @return false when refused (see ITileMap::AddSprite), or once removed;
             */
            virtual bool AddSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) = 0;

            /**
             * @brief Take a sprite off a layer of this world, as ITileMap::RemoveSprite.
             * @return false when it was not registered there, or once removed;
             */
            virtual bool RemoveSprite( int nLayerId, SunLight :: Sprite :: Sprite& sprite ) = 0;

            /**
             * @brief The world's own collision rules, colliders and listeners: a listener added
             * here hears only this world's collisions - which is how a game tells which world a hit
             * happened in. Valid for as long as the handle is held, removed or not (a removed
             * world's rules are simply never tested again).
             */
            virtual SunLight :: Collision :: ICollisionManager& GetCollisionManager( void ) = 0;
        };
    }
}

#endif /* __IWORLD_H__ */
