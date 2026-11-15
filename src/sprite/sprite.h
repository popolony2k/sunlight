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

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include <map>
#include <memory>
#include "base/blendmode.h"
#include "base/color.h"
#include "collision/collider.h"
#include "sprite/texturemap.h"


namespace SunLight  {
    namespace Sprite  {
        /**
         * @brief Sunlight sprite management class implementation.
         * 
         */
        class Sprite : public SunLight :: Canvas :: Canvas  {

            /**
             * @brief Texture sequence list definition.
             * 
             */
            typedef std :: map<int, std :: unique_ptr<TextureMap>>  TextureSequenceList;

            TextureSequenceList             m_Sequences;
            TextureSequenceList :: iterator m_itActiveSequence;
            bool                            m_bIsValidActiveSequence;

            // Set by Advance(): this frame the texture map did NOT step to a
            // new frame, so the current one is held (see Advance/Draw).
            bool                            m_bFrameHeld;

            // L3 of the master plan: draw order among sprites registered on the SAME TMX layer
            // (TileMapRenderer::AddSprite) - default 0. Layers already order sprites between each
            // other; this only reorders siblings within one layer, stably, on a change.
            int                             m_nZOrder;

            // I2 of the master plan: the mode this sprite draws by, if it has one (SetBlendMode).
            bool                            m_bHasBlendMode;
            SunLight :: Base :: BlendMode   m_BlendMode;

            // I10 of the master plan: the sprite's opacity (0..1) and tint, applied by Draw().
            float                           m_fOpacity;
            SunLight :: Base :: stColor     m_Tint;

            public:

            Sprite( void );
            virtual ~Sprite( void );

            // The canvases are NOT owned by the sprite (raw pointers): the caller keeps them alive for as
            // long as the sprite uses them, and must not use one after the sprite is destroyed (its parent
            // and dimension pointers refer to the sprite). Destroying the sprite does not unload them.
            void AddTextureSequence( int nSequence,
                                     SunLight :: Canvas :: TextureCanvas* pTexture,
                                     int64_t nDelayMilli = -1 );
            // Sequences can be changed after they were built - a texture is only ever APPENDED by
            // AddTextureSequence, so without these a sequence cannot get a new pace or be rebuilt.
            //
            // The number of entries (textures) of a sequence, or -1 if there is no such sequence. A canvas
            // given to AddTextureSequence twice is two entries.
            int GetTextureSequenceSize( int nSequence );

            // Set the delay of EVERY entry of a sequence, in milliseconds. The entry being shown is rescheduled
            // from now, so the new pace applies from the next step. It never touches which entry is shown or the
            // canvases' animation state, and it changes NOTHING when the sequence already has that delay - safe to
            // call on every reconfigure. -1 means "no timing": a HELD frame by design (a single-entry sequence with
            // delay -1 never steps, so it never animates; give it a real delay to animate). false if there is no
            // such sequence.
            bool SetTextureSequenceDelay( int nSequence, int64_t nDelayMilli );

            // Remove every entry of a sequence. The canvases are released, not destroyed or unloaded: they stay
            // loaded and can be added again (AddTextureSequence), and - as with Unload() - the ones that
            // followed this sprite or its parent are unparented, since nothing can reach them through the sprite
            // any more. If the sequence was the ACTIVE one the sprite has no active sequence afterwards
            // (GetActiveTextureSequence() is -1, nothing is drawn or advanced) until SetActiveTextureSequence is
            // called; clearing another sequence leaves the active one alone. false if there is no such sequence.
            bool ClearTextureSequence( int nSequence );

            bool SetActiveTextureSequence( int nSequence );
            int GetActiveTextureSequence( void );
            SunLight :: Canvas :: TextureCanvas* GetActiveTexture( void );

            // Also tells the previous parent (ChildRemoved) and moves the canvases that were following it.
            void SetParent( SunLight :: Canvas :: BaseCanvas* pParent );

            void SetVisible( bool bVisible );

            // World space (see BaseCanvas::SetWorldSpace): the sprite's position is a MAP position, drawn
            // where the map is drawn at that position by every view. Applies to every canvas added
            // to the sprite, before or after (AddTextureSequence re-applies the sprite's mode to the canvas
            // on every call, so a canvas reused from an earlier life cannot keep an old one). Off by default.
            // Unload() does not touch it: the mode is the sprite's, not the textures'.
            void SetWorldSpace( bool bWorldSpace );

            // L1 of the master plan: same propagation shape as SetWorldSpace, for
            // BaseCanvas::SetParallaxLayerId - set by TileMapRenderer::AddSprite, not meant to
            // be called directly by application code.
            void SetParallaxLayerId( int nLayerId );

            // L3 of the master plan: this sprite's draw order among the other sprites on the same
            // TMX layer. Default 0. TileMapRenderer stable-sorts a layer's sprites by this value
            // before drawing them, so equal values keep whichever order the sprites were added in
            // (AddSprite) - every sprite at 0 therefore draws in that same, unchanged order.
            void SetZOrder( int nZOrder );
            int GetZOrder( void );

            // I2 of the master plan: the mode this sprite draws by (see SunLight::Base::BlendMode) - a
            // light in BLEND_MODE_ADD, a shadow in BLEND_MODE_MULTIPLY - on every frame and in every view,
            // until ClearBlendMode. Draw() switches to it for this sprite alone and back to the mode that
            // was in effect, so it also holds inside a game's IDrawSurface::BeginBlendMode. A sprite with
            // no mode (the default) draws by whatever mode is in effect and switches nothing. Unload()
            // does not touch it: the mode is the sprite's, not the textures'.
            void SetBlendMode( SunLight :: Base :: BlendMode mode );
            void ClearBlendMode( void );
            bool HasBlendMode( void );
            // The mode set by SetBlendMode; BLEND_MODE_NORMAL when the sprite has none.
            SunLight :: Base :: BlendMode GetBlendMode( void );

            // I10 of the master plan: the sprite's opacity, 0 (transparent) to 1 (opaque, the default),
            // as a layer's (stLayer::fOpacity) - clamped; a NaN leaves it as it was. And its tint, opaque
            // white (none) by default: its RGB multiplies into the frame's, as a layer's tint does into
            // its tiles; its alpha is kept but not used - the opacity is the alpha control. Draw()
            // applies both to the frame it draws, for that draw only: the frame canvases' own colours
            // are never changed, and multiply into the result (a canvas alpha of 200 at opacity 0.5
            // draws 100). Every frame, sequence and view takes them, until changed; Unload() does not
            // touch them. A sprite does not take its layer's opacity or tint (I8).
            void SetOpacity( float fOpacity );
            float GetOpacity( void );
            void SetTint( SunLight :: Base :: stColor tint );
            SunLight :: Base :: stColor GetTint( void );

            // Whether the ACTIVE texture passes the viewport test that gates Advance() and Draw() in the
            // view being drawn (the sprite is visible, has an active sequence, and the texture's rectangle is
            // not entirely past the viewport's far edges - one entirely before its origin still passes, as it
            // always has for Advance(), with a zero-size clip). Outside a draw pass the view being drawn is
            // the DEFAULT view (the passes leave it active), so this answers for the default view's viewport
            // and camera.
            bool IsOnScreen( void );

            void Move( SunLight :: Base :: stCoordinate2D& step );

            void Advance( void );
            void Draw( void );
            void Update( void );
            void Unload( void );
        };
    }
}

#endif /* __SPRITE_H__ */
