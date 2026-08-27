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

#ifndef __IDRAWSURFACE_H__
#define __IDRAWSURFACE_H__

#include <string>
#include "engines/iengine.h"


namespace SunLight {
    namespace DrawSurface  {

        /**
         * @brief Screen-space drawing/window-state surface - every
         * primitive here operates in screen space (no viewport/camera
         * transform) and is independent of whether any tile map is
         * currently loaded, unlike everything on @link
         * SunLight::TileMap::ITileMap. Split out from ITileMap (which
         * TileMapRenderer still also implements) once it became clear
         * these had all ended up there for one reason only - ITileMap was
         * the sole public interface a consumer could get a handle to,
         * not because any of them are genuine tile-map operations; every
         * one of these is a pure pass-through straight to IEngine
         * underneath, with zero access to map data. Modeled after
         * a GDI-style drawing surface/device-context object: something a
         * caller draws directly onto, orthogonal to whatever "document"
         * (here, a tile map) may or may not currently be loaded.
         *
         * A consumer holding only an IDrawSurface (not also an ITileMap)
         * can draw text/shapes and query/toggle window state with no tile
         * map ever having been loaded - the missing piece for genuinely
         * mapless rendering is sprite drawing (@link
         * SunLight::TileMap::ITileMap::AddSprite still requires a loaded
         * map to resolve a layer id), not this.
         */
        class IDrawSurface {

            public:

            virtual ~IDrawSurface( void ) {};

            /**
             * @brief Set the application window's title, replacing whatever
             * title it was created with.
             *
             * @param strTitle The new window title;
             */
            virtual void SetWindowTitle( const std :: string &strTitle ) = 0;

            /**
             * @brief Set a whole-screen fade overlay, drawn on top of every
             * other rendered element (tilemap, sprites, everything) - not a
             * per-texture effect.
             *
             * @param fAlpha Fade amount, 0.0 (fully visible, no overlay) to
             * 1.0 (fully black);
             */
            virtual void SetScreenFade( float fAlpha ) = 0;

            /**
             * @brief Enter or leave fullscreen, defaulting to a real,
             * OS-level fullscreen space (see RaylibEngine::SetFullscreen
             * for why that's the default and when the
             * FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED fallback is worth
             * using instead). The game itself always renders at its own
             * fixed internal resolution regardless of this setting - the
             * renderer is responsible for scaling that up/down and
             * letterboxing to whatever the actual window size ends up
             * being.
             *
             * Switching strategy while already fullscreen in the other one
             * is unsupported - call SetFullscreen( false ) first, then
             * re-enter fullscreen with the new strategy.
             *
             * @param bFullscreen true to enter fullscreen, false to return
             * to windowed mode;
             * @param strategy Which fullscreen strategy to use when
             * entering fullscreen (ignored when bFullscreen is false);
             */
            virtual void SetFullscreen( bool bFullscreen,
                                       SunLight :: Engines :: IEngine :: FullscreenStrategy strategy =
                                           SunLight :: Engines :: IEngine :: FULLSCREEN_STRATEGY_REAL ) = 0;

            /**
             * @brief Query whether the window is currently fullscreen,
             * regardless of which strategy is active (see @link
             * SetFullscreen).
             */
            virtual bool GetFullscreen( void ) = 0;

            /**
             * @brief Show or hide the on-screen FPS counter, drawn at the
             * top-left corner of the window every frame.
             *
             * @param bDrawFPS true to draw the FPS counter, false to hide it;
             */
            virtual void SetDrawFPS( bool bDrawFPS ) = 0;

            /**
             * @brief Query whether the FPS counter is currently being drawn
             * (see @link SetDrawFPS).
             */
            virtual bool GetDrawFPS( void ) = 0;

            /**
             * @brief Allow or disallow the user resizing the application
             * window by dragging its edges/corners. Safe to call both
             * before the window is created (the initial state) and while
             * it's already running (a genuine live toggle, in either
             * direction).
             *
             * @param bResizeable true to allow resizing, false to disallow it;
             */
            virtual void SetWindowResizeable( bool bResizeable ) = 0;

            /**
             * @brief Query whether the window is currently resizeable (see
             * @link SetWindowResizeable).
             */
            virtual bool GetWindowResizeable( void ) = 0;

            /**
             * @brief Set the renderer's own target frame rate - the cap
             * the game loop paces itself against, not the current
             * measured FPS. Safe to call both before the window is
             * created (the initial value, normally given to
             * TileMapRenderer's own constructor instead - see it's own
             * comment) and while it's already running (a genuine live
             * change, taking effect immediately).
             *
             * @param nTargetFps The new target frame rate, in frames per second;
             */
            virtual void SetTargetFPS( int nTargetFps ) = 0;

            /**
             * @brief Query the currently configured target FPS (see
             * @link SetTargetFPS) - reads back a locally cached value,
             * since the underlying engine has no API to query a
             * previously-set target FPS (only the live, measured
             * GetFPS(), which this deliberately does not wrap here).
             */
            virtual int GetTargetFPS( void ) = 0;

            /**
             * @brief Choose how the fixed-internal-resolution render target
             * is blitted onto the real window/screen when their sizes
             * differ (fullscreen, or a live-resized window). false
             * (default) preserves aspect ratio via a single uniform scale
             * factor, letterboxing (black bars) whichever axis doesn't
             * fill exactly. true stretches to fill the real window/screen
             * completely instead, using independent X/Y scale factors -
             * no black bars, but the image visibly warps whenever the
             * window's own aspect ratio doesn't match the render target's.
             *
             * @param bStretchToFill true to stretch-fill (no letterboxing,
             * may distort), false to letterbox (preserves aspect ratio);
             */
            virtual void SetStretchToFill( bool bStretchToFill ) = 0;

            /**
             * @brief Query whether the render target is currently being
             * stretched to fill (see @link SetStretchToFill).
             */
            virtual bool GetStretchToFill( void ) = 0;

            /**
             * @brief Load (or replace) the font used by @link DrawText,
             * from any backend-supported font file (at minimum TrueType/
             * OpenType; the raylib backend also auto-detects an AngelCode
             * BMFont ".fnt" atlas by extension - see IEngine::SetFont).
             * Callers just pass a file path; which font formats are
             * actually supported is entirely a backend concern, not
             * something callers need to know about. Until this is called
             * for the first time, @link DrawText falls back to the
             * backend's own built-in default font, so text can be drawn
             * with zero setup.
             *
             * @param szFilePath Path to the font file to load;
             * @return true if the font loaded successfully and is now the
             * active font, false if it failed to load (the previously
             * active font, if any, is left untouched).
             */
            virtual bool SetFont( const char *szFilePath ) = 0;

            /**
             * @brief Draw a line of text on screen, in screen space (no
             * viewport/camera transform, same as the FPS counter), using
             * whichever font is currently active (see @link SetFont).
             *
             * @param szText The text to draw;
             * @param nPosX X coordinate to draw at;
             * @param nPosY Y coordinate to draw at;
             * @param nFontSize Font size, in pixels;
             * @param nRed Text color red channel (0-255);
             * @param nGreen Text color green channel (0-255);
             * @param nBlue Text color blue channel (0-255);
             * @param nAlpha Text color alpha channel (0-255);
             */
            virtual void DrawText( const char *szText,
                                   int nPosX,
                                   int nPosY,
                                   int nFontSize,
                                   unsigned char nRed,
                                   unsigned char nGreen,
                                   unsigned char nBlue,
                                   unsigned char nAlpha ) = 0;

            /**
             * @brief Draw a filled, solid-color rectangle in screen space
             * (no viewport/camera transform, same coordinate space as
             * @link DrawText and the FPS counter) - meant for simple HUD
             * elements (e.g. a progress/health bar) that don't warrant a
             * whole sprite/texture asset. A thin pass-through to
             * IEngine::DrawFilledRectangle, which already exists and is
             * already used internally for the screen-fade overlay - this
             * just exposes that same capability as it's own, arbitrary-
             * position/size/color primitive rather than the fixed, full-
             * window, single-alpha-value overlay @link SetScreenFade
             * already covers.
             *
             * @param nPosX X coordinate of the rectangle's top-left corner;
             * @param nPosY Y coordinate of the rectangle's top-left corner;
             * @param nWidth Rectangle width, in pixels;
             * @param nHeight Rectangle height, in pixels;
             * @param nRed Fill color red channel (0-255);
             * @param nGreen Fill color green channel (0-255);
             * @param nBlue Fill color blue channel (0-255);
             * @param nAlpha Fill color alpha channel (0-255);
             */
            virtual void DrawFilledRectangle( int nPosX,
                                              int nPosY,
                                              int nWidth,
                                              int nHeight,
                                              unsigned char nRed,
                                              unsigned char nGreen,
                                              unsigned char nBlue,
                                              unsigned char nAlpha ) = 0;

            /**
             * @brief Measure how wide a line of text would render, in
             * pixels, at a given font size, using whichever font is
             * currently active (see @link SetFont) - the same font @link
             * DrawText itself would use. Meant for screen-space HUD
             * layout (e.g. right-aligning a score/lives readout), not for
             * anything camera/viewport-aware.
             *
             * @param szText The text to measure;
             * @param nFontSize Font size, in pixels;
             * @return The text's rendered width, in pixels;
             */
            virtual int MeasureText( const char *szText, int nFontSize ) = 0;

            /**
             * @brief Return the engine's own fixed design/render
             * resolution width, in pixels - the coordinate space @link
             * DrawText and every other screen-space draw call (the FPS
             * counter, the screen-fade overlay) operate in. This is
             * constant for the lifetime of the app (set once, at
             * construction) and deliberately NOT the actual, live OS
             * window's pixel size, which can change independently (see
             * IEngine::GetScreenWidth) - the engine always letterbox-
             * scales this fixed resolution to fit whatever the real
             * window size ends up being, so screen-space drawing (and HUD
             * layout built on top of it) should always measure against
             * this value, not the live window size.
             *
             * @return The render resolution width, in pixels;
             */
            virtual int GetWindowWidth( void ) = 0;

            /**
             * @brief Return the engine's own fixed design/render
             * resolution height, in pixels (see @link GetWindowWidth).
             *
             * @return The render resolution height, in pixels;
             */
            virtual int GetWindowHeight( void ) = 0;
        };
    }
}

#endif
