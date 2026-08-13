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

#ifndef __IENGINE_H__
#define __IENGINE_H__

#include <string>
#include "base/color.h"
#include "base/primitives.h"


namespace SunLight  {
    namespace Engines  {

        /**
         * @brief Backend rendering engine generic interface.
         * Every graphics backend (raylib, SDL, ...) implements this using
         * its own native types internally, exposing only SunLight native
         * primitives here.
         */
        class IEngine  {

            public:

            virtual ~IEngine( void )  {}

            /**
             * @brief Must be implemented to provide texture loading from disk based on
             * chosen target engine;
             *
             * @param szFileName The texture file name to load;
             * @param nWidth Output parameter receiving the loaded texture width;
             * @param nHeight Output parameter receiving the loaded texture height;
             * @return Opaque handle to the loaded texture, or nullptr if operation failed;
             */
            virtual SunLight :: Base :: TextureHandle LoadTexture( const char *szFileName,
                                                                    int& nWidth,
                                                                    int& nHeight ) = 0;

            /**
             * @brief Must be implemented to unload previous texture data loaded by
             * @see LoadTexture() method on chosen target engine;
             *
             * @param hTexture The texture handle to unload;
             */
            virtual void UnloadTexture( SunLight :: Base :: TextureHandle hTexture ) = 0;

            /**
             * @brief Must be implemented to draw a pixel according the specified
             * position on chosen target engine;
             *
             * @param nPosX The X coordinate to plot pixel;
             * @param nPosY The Y coordinate to plot pixel;
             * @param color Color of pixel;
             */
            virtual void SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color ) = 0;

            /**
             * @brief Must be implemented to draw a texture at the specified position
             * on chosen target engine;
             *
             * @param hTexture The texture handle to draw;
             * @param nPosX X coordinate to draw texture;
             * @param nPosY Y coordinate to draw texture;
             * @param tint Color tint applied to texture;
             */
            virtual void DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                      int nPosX,
                                      int nPosY,
                                      SunLight :: Base :: stColor tint ) = 0;

            /**
             * @brief Must be implemented to draw part of a texture (defined by a
             * rectangle) with rotation and scale tiled into dest on chosen target
             * engine;
             *
             * @param hTexture The texture handle to draw;
             * @param source Source rectangle defining the texture area to be drawn;
             * @param dest Destination rectangle where texture will be tiled into;
             * @param origin Origin point used as rotation/scale reference;
             * @param rotation Rotation angle to be applied;
             * @param scale Scale factor to be applied;
             * @param tint Color tint applied to texture;
             */
            virtual void DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                                           SunLight :: Base :: stRectangle source,
                                           SunLight :: Base :: stRectangle dest,
                                           SunLight :: Base :: stVector2D origin,
                                           float rotation,
                                           float scale,
                                           SunLight :: Base :: stColor tint ) = 0;

            /**
             * @brief Must be implemented to fill a rectangle with a solid/
             * alpha-blended color on chosen target engine. Draws exactly
             * the rectangle it's given, with no viewport/camera awareness
             * of its own - same as every other draw method on this
             * interface, any clipping against a viewport must already be
             * done by the caller before the coordinates reach here.
             *
             * @param nPosX X coordinate of the rectangle's top-left corner;
             * @param nPosY Y coordinate of the rectangle's top-left corner;
             * @param nWidth Rectangle width;
             * @param nHeight Rectangle height;
             * @param color Fill color (including alpha);
             */
            virtual void DrawFilledRectangle( int nPosX,
                                              int nPosY,
                                              int nWidth,
                                              int nHeight,
                                              SunLight :: Base :: stColor color ) = 0;

            /**
             * @brief Must be implemented to return the directory the running
             * executable lives in (trailing separator included), so callers
             * can resolve resource paths relative to the binary instead of
             * hardcoding an absolute, machine-specific path. Does not depend
             * on the backend's window/render context having been
             * initialized yet.
             *
             * @return The application's own directory, or an empty string
             * if it could not be determined;
             */
            virtual std :: string GetApplicationDirectory( void ) = 0;

            /**
             * @brief Must be implemented to enter or leave fullscreen on
             * chosen target engine. Implementations are free to pick
             * whichever fullscreen strategy suits their backend best (e.g.
             * borderless-windowed at the current monitor's native
             * resolution); callers should only rely on @see GetFullscreen
             * reflecting the resulting state.
             *
             * @param bFullscreen true to enter fullscreen, false to return
             * to windowed mode;
             */
            virtual void SetFullscreen( bool bFullscreen ) = 0;

            /**
             * @brief Must be implemented to report whether the window is
             * currently fullscreen (see @see SetFullscreen).
             *
             * @return true if the window is fullscreen, false if windowed;
             */
            virtual bool GetFullscreen( void ) = 0;

            /**
             * @brief Must be implemented to allow or disallow the user
             * resizing the window by dragging it's edges/corners, on an
             * already-created window. Only meaningful to call once the
             * window exists - callers are responsible for not calling this
             * before that (see TileMapRenderer::SetWindowResizeable, which
             * uses it's own pre-window-creation config-flag path instead
             * for the initial state).
             *
             * @param bResizeable true to allow resizing, false to disallow it;
             */
            virtual void SetWindowResizeable( bool bResizeable ) = 0;

            /**
             * @brief Must be implemented to return the current width, in
             * pixels, of the actual window/screen on chosen target engine -
             * as opposed to any fixed internal rendering resolution a
             * caller may be using, this always reflects the real, current
             * (and possibly just resized) window size.
             *
             * @return Current window/screen width, in pixels;
             */
            virtual int GetScreenWidth( void ) = 0;

            /**
             * @brief Must be implemented to return the current height, in
             * pixels, of the actual window/screen on chosen target engine
             * (see @see GetScreenWidth).
             *
             * @return Current window/screen height, in pixels;
             */
            virtual int GetScreenHeight( void ) = 0;

            /**
             * @brief Must be implemented to allocate an offscreen render
             * target of the given size on chosen target engine, that
             * @see BeginRenderTarget/@see EndRenderTarget can later draw
             * into, and @see GetRenderTargetTexture can draw out of like
             * any other loaded texture.
             *
             * @param nWidth Render target width, in pixels;
             * @param nHeight Render target height, in pixels;
             * @return Opaque handle to the render target, to be released
             * with @see UnloadRenderTarget once no longer needed;
             */
            virtual SunLight :: Base :: TextureHandle LoadRenderTarget( int nWidth, int nHeight ) = 0;

            /**
             * @brief Must be implemented to release a render target
             * previously allocated by @see LoadRenderTarget.
             *
             * @param hRenderTarget The render target handle to release;
             */
            virtual void UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) = 0;

            /**
             * @brief Must be implemented to redirect all subsequent drawing
             * on chosen target engine into the given render target, until
             * the matching @see EndRenderTarget call.
             *
             * @param hRenderTarget The render target to draw into;
             */
            virtual void BeginRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget ) = 0;

            /**
             * @brief Must be implemented to stop redirecting drawing into
             * whichever render target the matching @see BeginRenderTarget
             * call started, resuming normal drawing.
             */
            virtual void EndRenderTarget( void ) = 0;

            /**
             * @brief Must be implemented to return a texture handle for the
             * given render target's own contents, usable with
             * @see DrawTexture/@see DrawTextureTiled/@see DrawTextureScaled
             * exactly like any texture returned by @see LoadTexture - so a
             * render target can be drawn out of using the same draw calls
             * as everything else, once done being drawn into.
             *
             * @param hRenderTarget The render target handle whose texture
             * is being requested;
             * @return Texture handle for the render target's contents; not
             * separately owned, and not valid to use with
             * @see UnloadTexture (release it via @see UnloadRenderTarget
             * instead, once the render target itself is no longer needed);
             */
            virtual SunLight :: Base :: TextureHandle GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget ) = 0;

            /**
             * @brief Must be implemented to draw a texture stretched from
             * a source rectangle into a destination rectangle on chosen
             * target engine, with no tiling and no viewport/camera
             * awareness of its own - same as every other draw method on
             * this interface, any clipping/letterboxing/scaling math must
             * already be done by the caller before the coordinates reach
             * here. Unlike @see DrawTextureTiled, a negative source width/
             * height is valid here and flips the drawn result along that
             * axis - useful for render targets, whose contents are
             * typically stored bottom-up.
             *
             * @param hTexture The texture handle to draw;
             * @param source Source rectangle defining the texture area to
             * be drawn;
             * @param dest Destination rectangle the source is stretched
             * into;
             * @param tint Color tint applied to texture;
             */
            virtual void DrawTextureScaled( SunLight :: Base :: TextureHandle hTexture,
                                            SunLight :: Base :: stRectangle source,
                                            SunLight :: Base :: stRectangle dest,
                                            SunLight :: Base :: stColor tint ) = 0;
        };
    }
}

#endif  /* __IENGINE_H__ */
