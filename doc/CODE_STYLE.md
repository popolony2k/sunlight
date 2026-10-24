# sunlight code style

This is the style of sunlight's C++ code, written down so that other code built with it - Scarab
first, then Caravellius - can follow the same one. It is taken from the code itself: the owner
wrote sunlight, and the first commit (`e479bcd`, 2024-07-09, code dating from 2021) is the
reference. Where the code varied, the owner chose one form (2026-10-14); those choices are marked
**(chosen)**. Every example below is sunlight code, shown in the chosen forms (the code is
aligned to them by item H2 of the master plan).

The project rules in [`CLAUDE.md`](../CLAUDE.md) (tests, AddressSanitizer, verified comments,
the backend abstraction) apply on top of this guide.

## Files

- Every file starts with the zlib license header, as in any file under `src/`.
- One module per folder, one namespace per module: `src/collision/` is `SunLight::Collision`.
- File names are lower case and match the class they hold: `CollisionManager` lives in
  `collisionmanager.h` and `collisionmanager.cpp`. An interface's file starts with `i`
  (`isound.h`).
- Include guards are the file name in upper case between double underscores, and the closing
  `#endif` repeats it after one space **(chosen)**:

```cpp
#ifndef __COLLISIONMANAGER_H__
#define __COLLISIONMANAGER_H__

...

#endif /* __COLLISIONMANAGER_H__ */
```

- Includes use paths from `src/` (`"collision/collider.h"`). A `.cpp` file includes its own
  header first, by its path from `src/` too **(chosen)**:
  `#include "collision/collisionmanager.h"`.

## Namespaces and indentation

- Indentation is 4 spaces, never tabs.
- Namespaces are nested and indented, one level each:

```cpp
namespace SunLight  {
    namespace Collision  {

        class CollisionManager : public ICollisionManager  {
            ...
        };
    }
}
```

- A forward declaration follows the same rule: nested and indented, one level each, never the
  one-line C++17 form (`namespace A :: B  { class X; }`). A class of the header's own namespace
  is declared inside that namespace's block; one from another namespace gets its own nested
  block:

```cpp
namespace SunLight  {
    namespace Backends  {
        namespace Null  {
            class NullBackend;
        }
    }
}
```

- Forward-declare only the project's own classes, and only where the header needs nothing but
  the name (a pointer, a reference, a smart pointer, a `friend`): to break an include cycle
  (`View` and `TileMapRenderer`), or to keep a backend out of a public header (`NullBackend`).
  Everything else, and every class of an external library, is included.

- There is no `using namespace` in sunlight's own code; names are written in full
  (`SunLight :: TileMap :: stTile`). Tests may use `using` for the class under test.

## Spacing

- `::` and `->` have a space on each side: `SunLight :: Collision :: Collider`, `std :: vector`,
  `pLayer -> offsetx`.
- A function with no parameters says so: `( void )`, in declarations and definitions. A call
  has none: `Clear();`.
- Inside parentheses there is a space after `(` and before `)`; a keyword takes no space before
  its parenthesis:

```cpp
if( nColliderLayerId < m_ColliderLayerList.size() )  {
for( size_t nListener = 0; nListener < nListeners; nListener++ )  {
switch( pLayer -> type )  {
dst.pos.x = ( int ) ( ( src.pos.x * fZoomFactor ) + vp.pos.x );
```

- A cast has spaces inside its parentheses and one after: `( float ) nWidth`.

## Braces

- An opening brace goes at the end of the line, after **two spaces** **(chosen)**, on every line
  that ends with one: functions, control statements, lambdas, `namespace`, `class`, `struct`
  and `enum`.
- A function body starts with a blank line.
- `else` goes on its own line, never `} else {`:

```cpp
void CollisionManager :: FireOnCollision( SunLight :: Collision :: Collider* pFirst,
                                          SunLight :: TileMap :: stTile* pSecond )  {

    size_t  nListeners = m_Listeners.size();

    for( size_t nListener = 0; nListener < nListeners; nListener++ )  {
        m_Listeners[nListener] -> OnCollision( pFirst, pSecond );
    }
}

if( d1 < 0 )  {
    x++;
}
else  {
    x++;
    y--;
}
```

- `switch` cases are indented one level inside the `switch` and their bodies one more; a case
  label has a space before its colon (`case L_GROUP :`), and each case ends with `break;`:

```cpp
switch( pLayer -> type )  {
    case L_GROUP :
        DrawAllLayers( pLayer -> content.group_head );
        break;

    case L_OBJGR :
        ...
}
```

- A short body may go without braces on the next line (`if( !Tmx() )` / `    return;`).

## Pointers and references

The `*` and the `&` stay with the type **(chosen)**:

```cpp
SunLight :: Canvas :: TextureCanvas* pTexture
SunLight :: TileMap :: stDimension2D& GetDimension2D( void );
void GetCameraPosition( int& nX, int& nY );
```

A pointer is declared alone on its line, so `Type* pA, pB` (one pointer, one value) cannot
mislead.

## Names

Hungarian prefixes tell what a name holds:

| Prefix | For | Example |
|---|---|---|
| `m_` | a class member (before the type prefix) | `m_pParent`, `m_nZOrder`, `m_Sequences` |
| `p` | a pointer | `pLayer`, `m_pMap` |
| `n` | an integer | `nLayerId`, `nWidth` |
| `f` | a `float` or `double` | `fZoomFactor` |
| `b` | a `bool` | `bVisible`, `m_bIsStarted` |
| `sz` | a C string | `szFileName` |
| `str` | a `std::string` | `strTitle` |
| `it` | an iterator | `itItem`, `m_itActiveSequence` |
| `h` | a handle | `hTexture`, `hController` |

- Classes and methods are `PascalCase` (`SpriteRegistry`, `AddSprite`); objects of a class type
  and structs take no prefix (`viewport`, `dimension`, `m_Fonts`).
- Interfaces start with `I` (`ISound`, `ITileMap`); structs with `st` (`stDimension2D`).
- A container type gets a `typedef` named for what it holds:
  `typedef std :: map<int, TextureMap*>  TextureSequenceList;`.
- Enumerators are upper case and start with their type's name: `MAP_ALIGNMENT_TOP_LEFT`,
  `RENDERER_BACKEND_NULL`.

## Constants

- No magic numbers in new code: every value gets a name.
- A constant local to one `.cpp` file is a `#define` with a double underscore, in upper case,
  aligned in columns, with a comment when the value is not obvious:

```cpp
#define __DEFAULT_FPS                   30
#define __MAX_OPACITY_LEVEL            0xFF
#define __CULL_PADDING_TILES           1       // extra tiles either side of the visible range
```

- A constant that users of the library see, in a header, has no underscores in front:
  `MAX_COLLIDER_LAYERS`, `MAX_GAMEPAD_COUNT`, `WHITE_COLOR`.

## Declarations

Types and names are aligned in columns, for members and for local variables alike:

```cpp
TextureSequenceList             m_Sequences;
TextureSequenceList :: iterator m_itActiveSequence;
bool                            m_bIsValidActiveSequence;

float  fZoom  = context.fZoom;
int    nPadX  = __CULL_PADDING_TILES * ( int ) pTmxMap -> tile_width;
```

## Classes

- Data members first, private by default; then `public:`, indented at member level and with a
  blank line on each side:

```cpp
class Sprite : public SunLight :: Canvas :: Canvas  {

    TextureSequenceList             m_Sequences;
    bool                            m_bIsValidActiveSequence;

    public:

    Sprite( void );
    virtual ~Sprite( void );
    ...
};
```

- An interface is pure virtual, with a virtual destructor: `virtual ~ISound( void )  {}`,
  `virtual bool Load( std :: string strFileName ) = 0;`. An implementation marks its methods
  `override`.
- A destructor that deliberately leaves something alone says what and why in its comment (see
  `Sprite :: ~Sprite`, which does not unload the canvases it was given).

## Comments

- Functions are documented with a Doxygen comment, in the header or in the `.cpp`; each
  `@param` ends with `;`:

```cpp
/**
 * Add a collider to manager;
 * @param nColliderLayerId collider layer id to add the collider;
 * @param pCollider Pointer to collider to add;
 */
```

- Comments explain why, not only what. A `/* ... */` block explains a decision where it is made.
- Every claim a comment makes - about a library's behaviour, the standard, a measurement - is
  checked before it is written (read the vendored source, measure it). A wrong "why" is worse
  than none (see `CLAUDE.md`).

## Design

The rules sunlight's code relies on, beyond formatting (each one in `CLAUDE.md`):

- A member that owns heap data is a `std::unique_ptr`; a raw pointer never owns.
- Every backend sits behind an interface and a factory: no raylib, SDL or PhysFS call outside
  `src/backends/<name>/`.
- A class does not know who uses it: the renderer's parts (`MapData`, `LayerRenderer`,
  `SpriteRegistry`, `Camera`, ...) take what they need as arguments.
- Everything that can be tested without a window is tested with doctest, through the mocks and
  the null backend; a bug fix comes with a test shown to fail on the old code.
