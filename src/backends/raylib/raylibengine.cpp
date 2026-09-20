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
#include "filesystem/filesystemfactory.h"
#include "window/windowfactory.h"

#include <cstring>
#include <vector>

// Extra spacing (in pixels) DrawTextEx adds between characters, on top of
// whatever a font's own glyph metrics already provide - 0 means "use the
// font as authored, no extra letter-spacing", the correct default for a
// generic engine primitive with no per-call spacing parameter of it's own.
#define __DEFAULT_TEXT_SPACING   0.0f

namespace SunLight  {
    namespace Engines  {
        namespace Raylib  {

            /**
             * @brief raylib's own SetLoadFileDataCallback trampoline -
             * redirects every raylib-internal file load that goes through
             * the PUBLIC, hookable LoadFileData() (utils.c) - confirmed
             * by reading raylib's own source that texture loading
             * (rtextures.c) does - through
             * SunLight::FileSystem::FileSystemFactory instead of raylib's
             * own default fopen()-based reader. Whatever's mounted there
             * (a real loose directory today, potentially a real archive
             * later) is what texture loads actually read from; nothing
             * here decides that policy.
             *
             * NOT every raylib loader honors this hook - raudio.c (sound)
             * has it's OWN private, static copy of LoadFileData that
             * always reads straight from the OS filesystem, confirmed by
             * reading it's own source directly - sound loading needs a
             * different fix entirely (reading via IFileSystem directly,
             * then LoadWaveFromMemory), not this callback (see
             * RayLibSound::Load).
             *
             * Allocates the returned buffer via raylib's own MemAlloc()
             * (not new/malloc directly) so that UnloadFileData()'s
             * matching RL_FREE() - called by every one of raylib's own
             * loaders once they're done with the buffer - frees it
             * correctly regardless of whether raylib is configured with
             * it's own custom allocator.
             *
             * Routes szFileName through IFileSystem::ToVirtualPath()
             * before reading it - not just a courtesy for a caller-
             * supplied real path, but a genuine necessity here: raylib's
             * own LoadBMFont (rtext.c) constructs a multi-file AngelCode
             * BMFont's atlas image path internally via
             * GetDirectoryPath(fileName), which always prepends "./" to
             * relative paths (confirmed directly in raylib's own source).
             * That "./"-prefixed request reaches this callback verbatim -
             * ToVirtualPath() strips it, same as it already strips a
             * Windows drive letter, since PhysFS's own path sanitizer
             * rejects a leading "./" outright (a "." path segment is
             * explicitly illegal there, not just a whole-string special
             * case) - without this, the atlas silently fails to load and
             * raylib falls back to it's own default font, with no trace
             * at all.
             * @param szFileName The path being requested - whatever the
             * original LoadTexture/LoadImage/etc. call was given verbatim;
             * @param pDataSize Set to the number of bytes returned;
             * @return A MemAlloc'd copy of the file's contents, or nullptr
             * if it doesn't exist/couldn't be read (same failure shape as
             * raylib's own default LoadFileData);
             */
            static unsigned char* FileSystemLoadFileDataCallback( const char *szFileName, int *pDataSize )  {

                std :: vector<unsigned char>  data;
                std :: string                 strVirtualPath = SunLight :: FileSystem :: IFileSystem :: ToVirtualPath( szFileName );

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strVirtualPath, data ) )  {
                    *pDataSize = 0;

                    return nullptr;
                }

                unsigned char  *pBuffer = ( unsigned char * ) ::MemAlloc( ( unsigned int ) data.size() );

                memcpy( pBuffer, data.data(), data.size() );

                *pDataSize = ( int ) data.size();

                return pBuffer;
            }

            /**
             * @brief raylib's own SetLoadFileTextCallback trampoline - the
             * sibling of @see FileSystemLoadFileDataCallback for text
             * reads, hooking LoadFileText() (utils.c) rather than
             * LoadFileData(). A genuine, distinct gap otherwise: raylib's
             * own LoadBMFont (rtext.c) reads a multi-file AngelCode
             * BMFont's own .fnt file - it's text content, not it's binary
             * atlas image - via LoadFileText(), which (confirmed directly
             * in utils.c) falls back to a raw fopen(fileName, "rt") when
             * no callback is registered here, same as LoadFileData()
             * does for the binary path. That fallback works fine for a
             * loose directory (the file's really on disk either way) but
             * silently fails once the game's content lives inside a
             * mounted archive instead - the .fnt file is never found,
             * LoadBMFont returns an empty Font immediately, no trace at
             * all. @see FileSystemLoadFileDataCallback's own
             * ToVirtualPath() routing fixed the atlas image half of this
             * same multi-file BMFont load (v0.17.3); this fixes the other
             * half, the .fnt file itself.
             *
             * Allocates via raylib's own MemAlloc() (not new/malloc
             * directly), +1 byte for the null terminator LoadFileText's
             * own contract requires, so that UnloadFileText()'s matching
             * RL_FREE() frees it correctly.
             * @param szFileName The path being requested, whatever the
             * original LoadFileText/LoadBMFont/etc. call was given
             * verbatim;
             * @return A MemAlloc'd, null-terminated copy of the file's
             * text content, or nullptr if it doesn't exist/couldn't be
             * read (same failure shape as raylib's own default
             * LoadFileText);
             */
            static char* FileSystemLoadFileTextCallback( const char *szFileName )  {

                std :: vector<unsigned char>  data;
                std :: string                 strVirtualPath = SunLight :: FileSystem :: IFileSystem :: ToVirtualPath( szFileName );

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strVirtualPath, data ) )
                    return nullptr;

                char  *pText = ( char * ) ::MemAlloc( ( unsigned int ) data.size() + 1 );

                memcpy( pText, data.data(), data.size() );

                pText[ data.size() ] = '\0';

                return pText;
            }

            /**
             * @brief Registers @see FileSystemLoadFileDataCallback and
             * @see FileSystemLoadFileTextCallback so every texture/text
             * load routes through SunLight::FileSystem from construction
             * onward - safe/idempotent regardless of whether anything has
             * actually been mounted yet (an unmounted read simply fails,
             * same as any other missing-file case).
             */
            RaylibEngine :: RaylibEngine( void )  {

                ::SetLoadFileDataCallback( FileSystemLoadFileDataCallback );
                ::SetLoadFileTextCallback( FileSystemLoadFileTextCallback );

                // Subscribe to this backend's OWN window (GetDefaultWindow,
                // not the test-overridable GetWindow - see its doc comment)
                // so the custom font is released right before the window's
                // GL context is destroyed. Constructing the engine is what
                // first constructs that window when nothing else has yet, so
                // the window is always destroyed after this engine at exit.
                m_nCloseHandlerId = SunLight :: Window :: WindowFactory :: GetDefaultWindow().AddCloseHandler(
                                        [this]( void ) { ReleaseWindowState(); } );
            }

            /**
             * @brief Unsubscribe from the window's close event.
             */
            RaylibEngine :: ~RaylibEngine( void )  {

                SunLight :: Window :: WindowFactory :: GetDefaultWindow().RemoveCloseHandler( m_nCloseHandlerId );
            }

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
             * before the window/context goes away (fired by the window's
             * close handlers, see the constructor) - just the custom font
             * tracking, at the moment. Deliberately does NOT call ::UnloadFont here
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
            void RaylibEngine :: ReleaseWindowState( void )  {

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
             * @brief Fill whatever is currently being drawn into (the
             * window's own frame, or a render target between
             * BeginRenderTarget/EndRenderTarget) with a solid color - a
             * direct pass-through to raylib's own ::ClearBackground, which
             * acts on the currently bound framebuffer either way.
             * @param color The color to fill with;
             */
            void RaylibEngine :: ClearBackground( SunLight :: Base :: stColor color )  {

                ::ClearBackground( Color{ color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
            }

            /**
             * @brief Draw raylib's own built-in FPS counter at the given
             * position (::DrawFPS).
             * @param nPosX X coordinate to draw the counter at;
             * @param nPosY Y coordinate to draw the counter at;
             */
            void RaylibEngine :: DrawFPS( int nPosX, int nPosY )  {

                ::DrawFPS( nPosX, nPosY );
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
