/*
 * Text objects (B1 of the master plan): a Tiled text object is drawn with the font
 * RegisterFont registered for its fontfamily and bold/italic flags, laid out inside
 * its box (wrap, halign, valign), and drawn as "??????" in its place when that font
 * is not registered. Drawn through the renderer against MockEngine, so the event log
 * says what was drawn, where and with which font.
 */

#include <doctest/doctest.h>
#include <cmath>
#include <memory>
#include <sstream>
#include "renderer/tilemaprenderer.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;
typedef MockEngine :: Event              Event;

namespace  {

    // A 4 x 4 map of 16 px tiles with one object group holding the given objects XML.
    Bytes MakeMapWithObjects( const std :: string &strObjects )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"2\">"
            << "<objectgroup id=\"1\" name=\"labels\">" << strObjects << "</objectgroup>"
            << "</map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    // One text object. Attributes and text are given as XML by the caller.
    std :: string TextObject( int nId, int nX, int nY, int nW, int nH, const std :: string &strAttrs, const std :: string &strText )  {

        std :: ostringstream  xml;

        xml << "<object id=\"" << nId << "\" x=\"" << nX << "\" y=\"" << nY << "\" width=\"" << nW << "\" height=\"" << nH << "\">"
            << "<text " << strAttrs << ">" << strText << "</text></object>";

        return xml.str();
    }

    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        explicit Scene( const std :: string &strObjects )  {

            fsFixture.fs.files["maps/labels.tmx"] = MakeMapWithObjects( strObjects );

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport.emplace();
            config.viewport -> pos.x        = 10;
            config.viewport -> pos.y        = 10;
            config.viewport -> size.nWidth  = 890;
            config.viewport -> size.nHeight = 790;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/labels.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

            pRenderer -> SetCameraPosition( 0, 0 );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        double Zoom( void )  { return pRenderer -> GetView( 0 ) -> GetViewport().GetZoomProperties().fZoomFactor; }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // The TEXT events of the last frame, in drawing order.
        std :: vector<Event> Texts( void )  {
            std :: vector<Event>  texts;

            for( const Event &evt : engine().events )
                if( evt.kind == Event :: TEXT )
                    texts.push_back( evt );

            return texts;
        }
    };
}

TEST_SUITE( "renderer/text objects" )  {

    TEST_CASE( "RegisterFont needs a started renderer" )  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        RendererConfig                      config;
        std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

        REQUIRE( pRenderer != nullptr );

        CHECK( pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == false );
        CHECK( engineFixture.engine.nLoadFontCalls == 0 );
    }

    TEST_CASE( "A registered font is loaded through the engine, and the active font is not touched" )  {

        Scene  scene( TextObject( 1, 0, 0, 0, 0, "", "x" ) );

        CHECK( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );
        CHECK( scene.engine().nLoadFontCalls == 1 );
        CHECK( scene.engine().strLastLoadFontPath == "fonts/sans.ttf" );
        CHECK( scene.engine().nSetFontCalls == 0 );
    }

    TEST_CASE( "A font that fails to load is not registered, and an earlier one for the family is kept" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Sans\" pixelsize=\"12\"", "Hi" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        scene.engine().bLoadFontResult = false;
        CHECK( scene.pRenderer -> RegisterFont( "Sans", "fonts/missing.ttf", false, false ) == false );
        CHECK( scene.engine().nFontsDestroyed == 0 );

        scene.RunFrames( 1 );
        REQUIRE( scene.Texts().size() == 1 );
        CHECK( scene.Texts()[0].text == "Hi" );
        CHECK( scene.Texts()[0].font == scene.engine().hLastLoadedFont );
    }

    TEST_CASE( "Registering the same family and style again replaces the font and releases the old one" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Sans\" pixelsize=\"12\"", "Hi" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        const void  *hFirst = scene.engine().hLastLoadedFont;

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans2.ttf", false, false ) == true );

        // The old font object is destroyed (which releases it), and the new one is the one drawn with.
        CHECK( scene.engine().nFontsDestroyed == 1 );
        CHECK( scene.engine().hLastLoadedFont != hFirst );

        scene.RunFrames( 1 );
        REQUIRE( scene.Texts().size() == 1 );
        CHECK( scene.Texts()[0].font == scene.engine().hLastLoadedFont );
    }

    TEST_CASE( "A text object draws its text with the registered font, at its box position" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Sans\" pixelsize=\"12\"", "Hello" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].text == "Hello" );
        CHECK( texts[0].font == scene.engine().hLastLoadedFont );

        // Screen position = (object position x zoom) + viewport origin (10, 10), camera at 0.
        double  fZoom = scene.Zoom();
        CHECK( texts[0].x == ( float ) std :: lround( 5 * fZoom + 10 ) );
        CHECK( texts[0].y == ( float ) std :: lround( 7 * fZoom + 10 ) );
        CHECK( texts[0].nFontSize == ( int ) std :: lround( 12 * fZoom ) );
    }

    TEST_CASE( "A missing family draws ?????? in place of the text, with the active font, and never the text itself" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Nope\" pixelsize=\"12\"", "Secret" ) );

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].text == "??????" );
        CHECK( texts[0].font == nullptr );
    }

    TEST_CASE( "The placeholder is drawn on every frame, not only the first" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Nope\" pixelsize=\"12\"", "Secret" ) );

        scene.RunFrames( 1 );
        CHECK( scene.Texts().size() == 1 );

        scene.RunFrames( 1 );
        CHECK( scene.Texts().size() == 1 );
        CHECK( scene.Texts()[0].text == "??????" );
    }

    TEST_CASE( "A bold or italic object is not drawn with the regular face of its family" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Sans\" pixelsize=\"12\" bold=\"1\"", "Loud" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].text == "??????" );
        CHECK( texts[0].font == nullptr );
    }

    TEST_CASE( "A bold face registered for the family is used by a bold object" )  {

        Scene  scene( TextObject( 1, 5, 7, 0, 0, "fontfamily=\"Sans\" pixelsize=\"12\" bold=\"1\"", "Loud" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sansbold.ttf", true, false ) == true );

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 1 );
        CHECK( texts[0].text == "Loud" );
        CHECK( texts[0].font == scene.engine().hLastLoadedFont );
    }

    TEST_CASE( "Word wrap breaks lines at the box width, between words" )  {

        // MeasureText is the string's length x size / 2: at size 10 a line of 8 characters
        // is 40 px, the box width. "aaa bbb" fits (7 characters); "aaa bbb ccc" does not.
        Scene  scene( TextObject( 1, 0, 0, 40, 0, "fontfamily=\"Sans\" pixelsize=\"10\" wrap=\"1\"", "aaa bbb ccc" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );
        scene.engine().bMeasureByLength = true;

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 2 );
        CHECK( texts[0].text == "aaa bbb" );
        CHECK( texts[1].text == "ccc" );
        CHECK( texts[1].y > texts[0].y );
    }

    TEST_CASE( "A line break in the text starts a new line even without wrap" )  {

        Scene  scene( TextObject( 1, 0, 0, 0, 0, "fontfamily=\"Sans\" pixelsize=\"10\"", "one\ntwo" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 2 );
        CHECK( texts[0].text == "one" );
        CHECK( texts[1].text == "two" );
    }

    TEST_CASE( "Center alignment centres each line in the box" )  {

        // "abcd" at size 10 is 20 px by MeasureText; in a 100 px box it starts 40 px in.
        Scene  scene( TextObject( 1, 0, 0, 100, 0, "fontfamily=\"Sans\" pixelsize=\"10\" halign=\"center\"", "abcd" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );
        scene.engine().bMeasureByLength = true;

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 1 );

        double  fZoom = scene.Zoom();
        CHECK( texts[0].x == ( float ) std :: lround( 10 + ( 100 - 20 ) / 2 * fZoom ) );
    }

    TEST_CASE( "Right alignment and bottom valign place the block at the box's far edges" )  {

        // Box 100 x 50, size 10, one line "abcd" (20 px): right puts it at x = 80,
        // bottom puts one 10 px line at y = 40 inside the box.
        Scene  scene( TextObject( 1, 0, 0, 100, 50,
                                  "fontfamily=\"Sans\" pixelsize=\"10\" halign=\"right\" valign=\"bottom\"", "abcd" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );
        scene.engine().bMeasureByLength = true;

        scene.RunFrames( 1 );

        std :: vector<Event>  texts = scene.Texts();

        REQUIRE( texts.size() == 1 );

        double  fZoom = scene.Zoom();
        CHECK( texts[0].x == ( float ) std :: lround( 10 + 80 * fZoom ) );
        CHECK( texts[0].y == ( float ) std :: lround( 10 + 40 * fZoom ) );
    }

    TEST_CASE( "A text object entirely outside the viewport is not drawn" )  {

        Scene  scene( TextObject( 1, 5000, 5000, 0, 0, "fontfamily=\"Sans\" pixelsize=\"12\"", "far" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        scene.RunFrames( 1 );

        CHECK( scene.Texts().empty() );
    }

    TEST_CASE( "A text object's lines are drawn inside a clip of the viewport rectangle" )  {

        Scene  scene( TextObject( 1, 0, 0, 0, 0, "fontfamily=\"Sans\" pixelsize=\"10\"", "cut" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        scene.RunFrames( 1 );

        // The clip is the viewport (10, 10, 890, 790), and the text lies between its begin and end.
        const std :: vector<Event> &events = scene.engine().events;
        int                         nBegin = -1, nText = -1, nEnd = -1;

        for( int nIdx = 0; nIdx < ( int ) events.size(); nIdx++ )  {
            if( events[nIdx].kind == Event :: CLIP_BEGIN )  nBegin = nIdx;
            if( events[nIdx].kind == Event :: TEXT )        nText  = nIdx;
            if( events[nIdx].kind == Event :: CLIP_END )    nEnd   = nIdx;
        }

        REQUIRE( nBegin >= 0 );
        REQUIRE( nText > nBegin );
        REQUIRE( nEnd > nText );

        CHECK( events[nBegin].x == 10.0f );
        CHECK( events[nBegin].y == 10.0f );
        CHECK( events[nBegin].w == 890.0f );
        CHECK( events[nBegin].h == 790.0f );
        CHECK( scene.engine().nBeginClipCalls == 1 );
        CHECK( scene.engine().nEndClipCalls == 1 );
    }

    TEST_CASE( "Text objects draw under the frame's clip and get none of their own" )  {

        Scene  scene( TextObject( 1, 0, 0, 0, 0, "fontfamily=\"Sans\" pixelsize=\"10\"", "one" ) +
                      TextObject( 2, 0, 40, 0, 0, "fontfamily=\"Sans\" pixelsize=\"10\"", "two" ) );

        REQUIRE( scene.pRenderer -> RegisterFont( "Sans", "fonts/sans.ttf", false, false ) == true );

        // One single-view frame = one clip (the viewport). Two text objects add none: the clip is
        // the engine's job for the whole frame, not one per text object.
        scene.RunFrames( 1 );
        CHECK( scene.engine().nBeginClipCalls == 1 );
        CHECK( scene.engine().nEndClipCalls == 1 );
    }

    TEST_CASE( "A map with only shapes gets the frame's clip plus one clip per shape" )  {

        Scene  scene( "<object id=\"1\" x=\"0\" y=\"0\" width=\"20\" height=\"20\"/>" );

        // The frame's clip, then the shape's own clip from PrimitiveClip.
        scene.RunFrames( 1 );

        CHECK( scene.engine().nBeginClipCalls == 2 );
        CHECK( scene.engine().nEndClipCalls == 2 );
    }
}
