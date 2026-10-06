/*
 * Shape pixels (study tool): draws a rectangle, a polygon and an ellipse that cross the viewport's
 * edges, and records the pixels that would reach the screen. Each build (software boundary test, or
 * the engine clip of the thick-primitives study) writes its own list when SUNLIGHT_PIXEL_DUMP names a
 * file; the two files are then compared. Without the variable the test does nothing.
 *
 * The recording engine applies the clip the way the scissor does: [x, x + w) x [y, y + h), and a clip
 * begun inside another is the intersection of the two.
 */

#include <doctest/doctest.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "engines/enginefactory.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    struct Pixel  {
        int             x;
        int             y;
        unsigned char   r;
        unsigned char   g;
        unsigned char   b;
        unsigned char   a;
    };

    bool operator<( const Pixel &lhs, const Pixel &rhs )  {
        if( lhs.y != rhs.y ) return lhs.y < rhs.y;
        if( lhs.x != rhs.x ) return lhs.x < rhs.x;
        if( lhs.r != rhs.r ) return lhs.r < rhs.r;
        if( lhs.g != rhs.g ) return lhs.g < rhs.g;
        if( lhs.b != rhs.b ) return lhs.b < rhs.b;
        return lhs.a < rhs.a;
    }

    class PixelEngine : public MockEngine  {

        public:

        std :: vector<SunLight :: Base :: stRectangle>  clips;
        std :: vector<Pixel>                             pixels;

        void BeginClip( SunLight :: Base :: stRectangle rect ) override  {
            if( !clips.empty() )  {
                const SunLight :: Base :: stRectangle  &outer = clips.back();
                float  fLeft   = std :: max( rect.x, outer.x );
                float  fTop    = std :: max( rect.y, outer.y );
                float  fRight  = std :: min( rect.x + rect.width, outer.x + outer.width );
                float  fBottom = std :: min( rect.y + rect.height, outer.y + outer.height );

                rect = SunLight :: Base :: stRectangle { fLeft, fTop, std :: max( 0.0f, fRight - fLeft ),
                                                          std :: max( 0.0f, fBottom - fTop ) };
            }

            clips.push_back( rect );
        }

        void EndClip( void ) override  {
            if( !clips.empty() )
                clips.pop_back();
        }

        void SetPixel( int nPosX, int nPosY, SunLight :: Base :: stColor color ) override  {
            if( !clips.empty() )  {
                const SunLight :: Base :: stRectangle  &clip = clips.back();

                if( ( nPosX < clip.x ) || ( nPosX >= clip.x + clip.width ) ||
                    ( nPosY < clip.y ) || ( nPosY >= clip.y + clip.height ) )
                    return;
            }

            pixels.push_back( Pixel { nPosX, nPosY, color.nRed, color.nGreen, color.nBlue, color.nAlpha } );
        }
    };

    // A map of shapes crossing the viewport's top-left and bottom-right edges, plus one inside it.
    Bytes MakeShapesTmx( void )  {

        std :: ostringstream  tmx;

        tmx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"4\" height=\"4\""
            << " tilewidth=\"16\" tileheight=\"16\" nextlayerid=\"3\" nextobjectid=\"10\">"
            << "<objectgroup id=\"1\" name=\"shapes\" color=\"#ffff8040\">"
            << "<object id=\"1\" x=\"-20\" y=\"-20\" width=\"120\" height=\"80\"/>"
            << "<object id=\"2\" x=\"60\" y=\"40\" width=\"0\" height=\"0\"><polygon points=\"0,0 80,10 40,90\"/></object>"
            << "<object id=\"3\" x=\"150\" y=\"100\" width=\"120\" height=\"120\"><ellipse/></object>"
            << "</objectgroup></map>";

        std :: string  str = tmx.str();

        return Bytes( str.begin(), str.end() );
    }

    class PixelRig  {

        public:

        MockWindowFixture         windowFixture;
        MemoryFileSystemFixture   fsFixture;
        PixelEngine               engine;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        PixelRig( void )  {
            SunLight :: Engines :: EngineFactory :: SetEngine( &engine );
            fsFixture.fs.files["maps/shapes.tmx"] = MakeShapesTmx();

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport.emplace();
            config.viewport -> pos.x        = 10;
            config.viewport -> pos.y        = 10;
            config.viewport -> size.nWidth  = 200;
            config.viewport -> size.nHeight = 150;

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
            REQUIRE( pRenderer -> LoadMap( "maps/shapes.tmx", ITM :: MAP_ALIGNMENT_TOP_LEFT ) == true );

            pRenderer -> SetCameraPosition( 0, 0 );
        }

        ~PixelRig( void )  {
            if( pRenderer )
                pRenderer -> Stop();

            SunLight :: Engines :: EngineFactory :: SetEngine( nullptr );
        }
    };
}

TEST_SUITE( "study: shape pixels" )  {

    TEST_CASE( "Shape pixels are written to SUNLIGHT_PIXEL_DUMP for comparison between builds" )  {

        const char  *szDump = std :: getenv( "SUNLIGHT_PIXEL_DUMP" );

        if( szDump == nullptr )
            return;

        PixelRig  rig;

        rig.windowFixture.window.nFramesUntilShouldClose = 1;
        rig.pRenderer -> Run();

        std :: vector<Pixel>  pixels = rig.engine.pixels;
        std :: sort( pixels.begin(), pixels.end() );

        std :: ofstream  out( szDump );

        for( const Pixel &px : pixels )
            out << px.x << ' ' << px.y << ' ' << ( int ) px.r << ' ' << ( int ) px.g << ' ' << ( int ) px.b << ' ' << ( int ) px.a << '\n';

        CHECK( !pixels.empty() );
    }
}
