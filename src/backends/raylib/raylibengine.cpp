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

#include "backends/raylib/raylibengine.h"

// Extra spacing (in pixels) DrawTextEx adds between characters, on top of
// whatever a font's own glyph metrics already provide - 0 means "use the
// font as authored, no extra letter-spacing", the correct default for a
// generic engine primitive with no per-call spacing parameter of it's own.
#define __DEFAULT_TEXT_SPACING   0.0f

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief Draw part of a texture (defined by a rectangle) with rotation and scale tiled into dest
             * This routines is planned to be removed after raylib 4.2.0 and was moved to a samples project directory
             * found at https://github.com/raysan5/raylib/blob/master/examples/textures/textures_draw_tiled.c
             *
             * @param hTexture
             * @param sourceIn
             * @param destIn
             * @param originIn
             * @param rotation
             * @param scale
             * @param tintIn
             */
            void RaylibEngine :: DrawTextureTiled( SunLight :: Base :: TextureHandle hTexture,
                                                   SunLight :: Base :: stRectangle sourceIn,
                                                   SunLight :: Base :: stRectangle destIn,
                                                   SunLight :: Base :: stVector2D originIn,
                                                   float rotation,
                                                   float scale,
                                                   SunLight :: Base :: stColor tintIn )  {

                Texture2D  texture = *reinterpret_cast<Texture2D*>( hTexture );
                Rectangle  source  { sourceIn.x, sourceIn.y, sourceIn.width, sourceIn.height };
                Rectangle  dest    { destIn.x, destIn.y, destIn.width, destIn.height };
                Vector2    origin  { originIn.x, originIn.y };
                Color      tint    { tintIn.nRed, tintIn.nGreen, tintIn.nBlue, tintIn.nAlpha };

                if ((texture.id <= 0) || (scale <= 0.0f)) return;  // Wanna see a infinite loop?!...just delete this line!
                if ((source.width == 0) || (source.height == 0)) return;

                int tileWidth = (int)(source.width*scale), tileHeight = (int)(source.height*scale);
                if ((dest.width < tileWidth) && (dest.height < tileHeight))
                {
                    // Can fit only one tile
                    ::DrawTexturePro( texture, 
                                      Rectangle{ source.x, source.y, 
                                                 ((float)dest.width/tileWidth)*source.width, 
                                                 ((float)dest.height/tileHeight)*source.height },
                                      Rectangle { dest.x, 
                                                  dest.y, 
                                                  dest.width,
                                                  dest.height }, 
                                      origin, 
                                      rotation, 
                                      tint );
                }
                else if (dest.width <= tileWidth)
                {
                    // Tiled vertically (one column)
                    int dy = 0;
                    for (;dy+tileHeight < dest.height; dy += tileHeight)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     ((float)dest.width/tileWidth)*source.width, 
                                                     source.height }, 
                                          Rectangle{ dest.x, 
                                                     dest.y + dy, 
                                                     dest.width, 
                                                     (float)tileHeight }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }

                    // Fit last tile
                    if (dy < dest.height)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     ((float)dest.width/tileWidth)*source.width, 
                                                     ((float)(dest.height - dy)/tileHeight)*source.height },
                                          Rectangle{ dest.x, 
                                                     dest.y + dy, 
                                                     dest.width, 
                                                     dest.height - dy }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }
                }
                else if (dest.height <= tileHeight)
                {
                    // Tiled horizontally (one row)
                    int dx = 0;
                    for (;dx+tileWidth < dest.width; dx += tileWidth)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     source.width, 
                                                     ((float)dest.height/tileHeight)*source.height }, 
                                          Rectangle{ dest.x + dx, 
                                                     dest.y, 
                                                     (float)tileWidth, 
                                                     dest.height }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }

                    // Fit last tile
                    if (dx < dest.width)
                    {
                        ::DrawTexturePro( texture, 
                                          Rectangle{ source.x, 
                                                     source.y, 
                                                     ((float)(dest.width - dx)/tileWidth)*source.width, 
                                                     ((float)dest.height/tileHeight)*source.height },
                                          Rectangle{ dest.x + dx, 
                                                     dest.y, 
                                                     dest.width - dx,
                                                     dest.height }, 
                                          origin, 
                                          rotation, 
                                          tint );
                    }
                }
                else
                {
                    // Tiled both horizontally and vertically (rows and columns)
                    int dx = 0;
                    for (;dx+tileWidth < dest.width; dx += tileWidth)
                    {
                        int dy = 0;
                        for (;dy+tileHeight < dest.height; dy += tileHeight)
                        {
                            ::DrawTexturePro( texture, 
                                              source, 
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         (float)tileWidth, 
                                                         (float)tileHeight }, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }

                        if (dy < dest.height)
                        {
                            ::DrawTexturePro( texture, 
                                              Rectangle{ source.x, 
                                                         source.y, 
                                                         source.width, 
                                                         ((float)(dest.height - dy)/tileHeight)*source.height },
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         (float)tileWidth, 
                                                         dest.height - dy }, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }
                    }

                    // Fit last column of tiles
                    if (dx < dest.width)
                    {
                        int dy = 0;
                        for (;dy+tileHeight < dest.height; dy += tileHeight)
                        {
                            ::DrawTexturePro( texture, 
                                              Rectangle{ source.x, 
                                                         source.y, 
                                                         ((float)(dest.width - dx)/tileWidth)*source.width, 
                                                         source.height },
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         dest.width - dx, 
                                                         (float)tileHeight}, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }

                        // Draw final tile in the bottom right corner
                        if (dy < dest.height)
                        {
                            ::DrawTexturePro( texture, 
                                              Rectangle{ source.x, 
                                                         source.y, 
                                                         ((float)(dest.width - dx)/tileWidth)*source.width, 
                                                         ((float)(dest.height - dy)/tileHeight)*source.height },
                                              Rectangle{ dest.x + dx, 
                                                         dest.y + dy, 
                                                         dest.width - dx, 
                                                         dest.height - dy }, 
                                              origin, 
                                              rotation, 
                                              tint );
                        }
                    }
                }
            }

            /**
             * @brief Fill a rectangle with a solid/alpha-blended color, at
             * exactly the coordinates given - no viewport/camera logic of
             * its own, same as every other draw method here.
             * @param nPosX X coordinate of the rectangle's top-left corner;
             * @param nPosY Y coordinate of the rectangle's top-left corner;
             * @param nWidth Rectangle width;
             * @param nHeight Rectangle height;
             * @param color Fill color (including alpha);
             */
            void RaylibEngine :: DrawFilledRectangle( int nPosX,
                                                      int nPosY,
                                                      int nWidth,
                                                      int nHeight,
                                                      SunLight :: Base :: stColor color )  {

                ::DrawRectangle( nPosX, nPosY, nWidth, nHeight,
                                Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }

            /**
             * @brief Load (or replace) the font used by DrawText (see
             * @see IEngine::SetFont). Forwards straight to raylib's own
             * ::LoadFont, which auto-detects the font file's format from
             * it's extension (TrueType/OpenType, or an AngelCode BMFont
             * ".fnt" atlas) - this method has no format-specific logic of
             * it's own. The previous custom font, if any, is unloaded only
             * after the new one is confirmed valid, so a failed load never
             * leaves DrawText without a usable font.
             */
            bool RaylibEngine :: SetFont( const char *szFilePath )  {

                Font  newFont = ::LoadFont( szFilePath );

                // On failure raylib's own ::LoadFont returns an all-zero,
                // never-allocated Font (confirmed in raylib's own source -
                // it only reaches a GPU upload on the success path), so
                // there's nothing of newFont's to release here.
                if( !::IsFontValid( newFont ) )
                    return false;

                if( m_bCustomFontLoaded )
                    ::UnloadFont( m_CurrentFont );

                m_CurrentFont       = newFont;
                m_bCustomFontLoaded = true;

                return true;
            }

            /**
             * @brief The font DrawText/MeasureText should use right now -
             * whichever one SetFont last loaded, or raylib's own built-in
             * default font if SetFont has never been called (or every
             * call to it so far has failed).
             */
            Font  RaylibEngine :: GetActiveFont( void )  {

                return m_bCustomFontLoaded ? m_CurrentFont : ::GetFontDefault();
            }

            /**
             * @brief Draw a line of text in screen space, using whichever
             * font is currently active - the backend's own built-in
             * default font until @see SetFont is called for the first
             * time (see @see IEngine::DrawText).
             */
            void RaylibEngine :: DrawText( const char *szText,
                                           int nPosX,
                                           int nPosY,
                                           int nFontSize,
                                           SunLight :: Base :: stColor color )  {

                ::DrawTextEx( GetActiveFont(), szText, Vector2{ ( float ) nPosX, ( float ) nPosY },
                             ( float ) nFontSize, __DEFAULT_TEXT_SPACING,
                             Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }

            /**
             * @brief Measure a line of text's rendered width, using
             * whichever font is currently active - same font resolution
             * as @see DrawText (see @see IEngine::MeasureText).
             */
            int RaylibEngine :: MeasureText( const char *szText, int nFontSize )  {

                Vector2  size = ::MeasureTextEx( GetActiveFont(), szText,
                                                 ( float ) nFontSize, __DEFAULT_TEXT_SPACING );

                return ( int ) size.x;
            }

            /**
             * @brief Release this class's own GPU-context-tied state
             * before the window/context goes away (see @see
             * IEngine::OnWindowClosing) - just the custom font tracking,
             * at the moment. Deliberately does NOT call ::UnloadFont here
             * first - not because the context is already gone (it isn't:
             * this runs before CloseWindow() is even called, so the GL
             * context is still fully valid at this point, an explicit
             * unload would be perfectly safe here too), but because it
             * would be redundant work for no benefit - CloseWindow()'s own
             * teardown (rlglClose(), then ClosePlatform()'s
             * glfwDestroyWindow()) discards the whole GL context
             * immediately after this runs anyway, taking every GPU handle
             * in it with it, including this one. This only clears this
             * class's own bookkeeping, so a *future* window (if Start() is
             * ever called again) doesn't inherit a stale handle pointing
             * at a texture that no longer exists.
             */
            void RaylibEngine :: OnWindowClosing( void )  {

                m_CurrentFont       = Font {};
                m_bCustomFontLoaded = false;
            }

            /**
             * Draw  pixel according the specified position.
             * @param nPosX The X coordinate to plot pixel;
             * @param nPosY The Y coordinate to plot pixel;
             * @param color Color of pixel;
             */
            void RaylibEngine :: SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color )  {
                ::DrawPixel( nPosX, nPosY, Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }

            /**
             * @brief Load a texture from disk.
             * @param szFileName Texture file name to load;
             * @param nWidth Output parameter receiving the loaded texture width;
             * @param nHeight Output parameter receiving the loaded texture height;
             * @return An opaque handle to the loaded texture, or nullptr on failure;
             */
            SunLight :: Base :: TextureHandle RaylibEngine :: LoadTexture( const char *szFileName,
                                                                            int& nWidth,
                                                                            int& nHeight )  {

                Texture2D *pTexture = new Texture2D;

                *pTexture = ::LoadTexture( szFileName );

                if( pTexture -> id <= 0 )  {
                    delete pTexture;
                    nWidth  = 0;
                    nHeight = 0;

                    return nullptr;
                }

                nWidth  = pTexture -> width;
                nHeight = pTexture -> height;

                return pTexture;
            }

            /**
             * @brief Unload a texture previously loaded by @see LoadTexture.
             * @param hTexture The texture handle to unload;
             */
            void RaylibEngine :: UnloadTexture( SunLight :: Base :: TextureHandle hTexture )  {

                Texture2D *pTexture = reinterpret_cast<Texture2D*>( hTexture );

                ::UnloadTexture( *pTexture );
                delete pTexture;
            }

            /**
             * @brief Draw a texture at the specified position.
             * @param hTexture The texture handle to draw;
             * @param nPosX X coordinate to draw texture;
             * @param nPosY Y coordinate to draw texture;
             * @param tint Color tint applied to texture;
             */
            void RaylibEngine :: DrawTexture( SunLight :: Base :: TextureHandle hTexture,
                                              int nPosX,
                                              int nPosY,
                                              SunLight :: Base :: stColor tint )  {

                Texture2D *pTexture = reinterpret_cast<Texture2D*>( hTexture );

                ::DrawTexture( *pTexture, nPosX, nPosY, Color{ tint.nRed, tint.nGreen, tint.nBlue, tint.nAlpha } );
            }

            /**
             * @brief Return the directory the running executable lives in.
             * Copies raylib's own internal static buffer into an owned
             * std::string immediately, so the result stays valid regardless
             * of subsequent raylib calls.
             * @return The application's own directory, or an empty string
             * if it could not be determined;
             */
            std :: string RaylibEngine :: GetApplicationDirectory( void )  {

                const char *szDirectory = ::GetApplicationDirectory();

                return ( szDirectory ? std :: string( szDirectory ) : std :: string() );
            }

            /**
             * @brief Enter or leave fullscreen. Defaults to raylib's real,
             * exclusive ToggleFullscreen() (which changes the monitor's
             * own video mode) rather than the borderless-windowed mode
             * this used to use unconditionally - switched 2026-08-26 after
             * a real, live-confirmed problem with borderless-windowed on
             * macOS: since it's an ordinary window merely resized to
             * cover the full screen (not a genuine OS-level fullscreen
             * space), the macOS Dock - when set to always show rather
             * than auto-hide - still draws on top of it, visibly covering
             * the bottom of the window. True ToggleFullscreen() enters a
             * real fullscreen space, which macOS itself hides the
             * Dock/menu bar behind automatically, matching what a player
             * actually expects from a fullscreen game. Confirmed via a
             * live A/B test that this genuinely fixes the Dock overlap
             * with no visible mode-switch flicker or resolution/scaling
             * artifacts on the platform this was tested on.
             *
             * FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED is kept available as
             * a fallback: unlike real ToggleFullscreen() (which switches
             * the monitor's actual video mode to the window's current
             * size, per GLFW's own glfwSetWindowMonitor() documentation),
             * borderless-windowed explicitly resizes to the monitor's
             * native resolution instead - safer on any platform/window
             * manager where a true video-mode switch misbehaves,
             * especially if the window's own size doesn't already match
             * the player's monitor.
             *
             * Switching strategy while already fullscreen in the other
             * one is unsupported (see IEngine::SetFullscreen) - both
             * raylib toggles assume theirs is the only active fullscreen
             * strategy, so mixing them without returning to windowed mode
             * first can leave stale window flags set.
             * @param bFullscreen true to enter fullscreen, false for windowed;
             * @param strategy Which fullscreen strategy to use when
             * entering fullscreen (ignored when bFullscreen is false);
             */
            void RaylibEngine :: SetFullscreen( bool bFullscreen, FullscreenStrategy strategy )  {

                if( strategy == FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED )  {
                    if( bFullscreen != ::IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE ) )
                        ::ToggleBorderlessWindowed();
                }
                else  {
                    if( bFullscreen != ::IsWindowState( FLAG_FULLSCREEN_MODE ) )
                        ::ToggleFullscreen();
                }
            }

            /**
             * @brief Query whether the window is currently fullscreen,
             * regardless of which strategy is active (see @see
             * SetFullscreen).
             */
            bool RaylibEngine :: GetFullscreen( void )  {

                return ::IsWindowState( FLAG_FULLSCREEN_MODE ) || ::IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE );
            }

            /**
             * @brief Allow or disallow live window resizing on an
             * already-created window (see @see IEngine::SetWindowResizeable).
             * SetWindowState/ClearWindowState (unlike SetConfigFlags, which
             * only takes effect if set before InitWindow) act directly on
             * the live window handle, so this works in either direction at
             * any point after the window already exists.
             */
            void RaylibEngine :: SetWindowResizeable( bool bResizeable )  {

                if( bResizeable )
                    ::SetWindowState( FLAG_WINDOW_RESIZABLE );
                else
                    ::ClearWindowState( FLAG_WINDOW_RESIZABLE );
            }

            /**
             * @brief Set the application window's title (see @see
             * IEngine::SetWindowTitle). ::SetWindowTitle acts on the live
             * window handle (glfwSetWindowTitle underneath, on the GLFW
             * desktop backend), so this is only meaningful once the
             * window already exists.
             */
            void RaylibEngine :: SetWindowTitle( const char *szTitle )  {

                ::SetWindowTitle( szTitle );
            }

            /**
             * @brief Current window/screen width, in pixels.
             */
            int RaylibEngine :: GetScreenWidth( void )  {

                return ::GetScreenWidth();
            }

            /**
             * @brief Current window/screen height, in pixels.
             */
            int RaylibEngine :: GetScreenHeight( void )  {

                return ::GetScreenHeight();
            }

            /**
             * @brief Allocate an offscreen render target. Point filtering
             * is applied so pixel-art content stays crisp when later drawn
             * scaled up rather than blurring.
             * @param nWidth Render target width, in pixels;
             * @param nHeight Render target height, in pixels;
             * @return Opaque handle to the render target;
             */
            SunLight :: Base :: TextureHandle RaylibEngine :: LoadRenderTarget( int nWidth, int nHeight )  {

                RenderTexture2D *pRenderTarget = new RenderTexture2D;

                *pRenderTarget = ::LoadRenderTexture( nWidth, nHeight );

                ::SetTextureFilter( pRenderTarget -> texture, TEXTURE_FILTER_POINT );

                return pRenderTarget;
            }

            /**
             * @brief Release a render target previously allocated by
             * @see LoadRenderTarget.
             * @param hRenderTarget The render target handle to release;
             */
            void RaylibEngine :: UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget )  {

                RenderTexture2D *pRenderTarget = reinterpret_cast<RenderTexture2D*>( hRenderTarget );

                ::UnloadRenderTexture( *pRenderTarget );
                delete pRenderTarget;
            }

            /**
             * @brief Redirect subsequent drawing into the given render
             * target, until the matching @see EndRenderTarget call.
             * @param hRenderTarget The render target to draw into;
             */
            void RaylibEngine :: BeginRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget )  {

                ::BeginTextureMode( *reinterpret_cast<RenderTexture2D*>( hRenderTarget ) );
            }

            /**
             * @brief Stop redirecting drawing into whichever render target
             * the matching @see BeginRenderTarget call started.
             */
            void RaylibEngine :: EndRenderTarget( void )  {

                ::EndTextureMode();
            }

            /**
             * @brief Return a texture handle for the given render target's
             * own contents - points directly at the texture field embedded
             * in the render target itself, which stays valid for as long
             * as the render target does.
             * @param hRenderTarget The render target handle whose texture
             * is being requested;
             */
            SunLight :: Base :: TextureHandle RaylibEngine :: GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget )  {

                return &( reinterpret_cast<RenderTexture2D*>( hRenderTarget ) -> texture );
            }

            /**
             * @brief Draw a texture stretched from a source rectangle into
             * a destination rectangle, with no tiling. A negative source
             * width/height flips the drawn result along that axis - used
             * for render targets, whose contents are stored bottom-up.
             * @param hTexture The texture handle to draw;
             * @param source Source rectangle defining the texture area to
             * be drawn;
             * @param dest Destination rectangle the source is stretched
             * into;
             * @param tint Color tint applied to texture;
             */
            void RaylibEngine :: DrawTextureScaled( SunLight :: Base :: TextureHandle hTexture,
                                                    SunLight :: Base :: stRectangle source,
                                                    SunLight :: Base :: stRectangle dest,
                                                    SunLight :: Base :: stColor tint )  {

                Texture2D  texture = *reinterpret_cast<Texture2D*>( hTexture );
                Rectangle  sourceRect { source.x, source.y, source.width, source.height };
                Rectangle  destRect   { dest.x, dest.y, dest.width, dest.height };

                ::DrawTexturePro( texture, sourceRect, destRect, Vector2 { 0.0f, 0.0f }, 0.0f,
                                 Color{ tint.nRed, tint.nGreen, tint.nBlue, tint.nAlpha } );
            }
        }
    }
}
