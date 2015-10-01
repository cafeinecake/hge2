/*
** A 2D game engine based on the API of Haaf's Game Engine 1.8
** HGE2 copyright: Dmytro Lytovchenko http://kvakvs.github.io/hge/
**
** Original engine copyright (C) 2003-2007, Relish Games
** http://hge.relishgames.com; Forum: http://relishgames.com/forum
**
*/
#pragma once

#include <windows.h>
#include <cmath>

#define HGE_VERSION 0x200

// CMake adds PROJECTNAME_EXPORTS when compiles DLL
#ifdef hge_EXPORTS
#define HGEDLL
#endif
//------

#ifdef HGEDLL
#define HGE_EXPORT  __declspec(dllexport)
#else
#define HGE_EXPORT
#endif

#define HGE_CALL  __stdcall

//
// Common data types
//
#include <stdint.h>

//
// Common math constants
//
// TODO: remove this
#ifndef M_PI
#define M_PI    3.14159265358979323846f
#define M_PI_2  1.57079632679489661923f
#define M_PI_4  0.785398163397448309616f
#define M_1_PI  0.318309886183790671538f
#define M_2_PI  0.636619772367581343076f
#endif


namespace hge {

//
// HGE Handle types
//
template <int>
class HGEHANDLE {
public:
  void *_handle;
  HGEHANDLE(void *h) : _handle(h) {}
  HGEHANDLE() : _handle(nullptr) {}
  operator void *() const
  {
    return _handle;
  }
  template <typename T> const T as() const
  {
    return (const T)_handle;
  }
  template <typename T> T as()
  {
    return (T)_handle;
  }
};

// FIXME: Won't compile in 64-bit mode due to handles (4 bytes) holding a pointer (8 bytes)
using HTEXTURE = HGEHANDLE<0>;
using HTARGET = HGEHANDLE<1>;
//using HEFFECT = HGEHANDLE<10>;
//using HMUSIC = HGEHANDLE<11>;
//using HSTREAM = HGEHANDLE<12>;
//using HCHANNEL = HGEHANDLE<13>;

#if HGE_DIRECTX_VER >= 9
using HSHADER = HGEHANDLE<2>;
#endif

template <typename T>
class Point {
public:
  T x, y;
  Point() : x(T()), y(T()) {}
  Point(T _x, T _y) : x(_x), y(_y) {}
  void set(T _x, T _y)
  {
    x = _x;
    y = _y;
  }
};
using Pointi32 = Point<int32_t>;
using Pointf = Point<float>;

template <typename T>
class Size {
public:
  T width, height;
};
using Sizei32 = Size<int32_t>;
using Sizef = Size<float>;

template <typename T>
class Rect {
public:
  Point<T> tl;
  Size<T>  size;
};
using Recti32 = Rect<int32_t>;
using Rectf = Rect<float>;

class hgeRect {
public:
  Pointf tl, br;

  hgeRect() = default;
  hgeRect(float _x1, float _y1, float _x2, float _y2)
    : tl(_x1, _y1), br(_x2, _y2) {}

  void Clear() { 
    m_clean = true;
  }
  bool IsClean() const {
    return m_clean;
  }

  void  Set(float _x1, float _y1, float _x2, float _y2)
  {
    tl.set(_x1, _y1);
    br.set(_x2, _y2);
    m_clean = false;
  }

  void  SetRadius(float x, float y, float r)
  {
    tl.set(x - r, y - r);
    br.set(x + r, y + r);
    m_clean = false;
  }

  void Encapsulate(float x, float y)
  {
    if (m_clean) {
      tl.set(x, y);
      br.set(x, y);
      m_clean = false;
    } else {
      if (x < tl.x) {
        tl.x = x;
      }

      if (x > br.x) {
        br.x = x;
      }

      if (y < tl.y) {
        tl.y = y;
      }

      if (y > br.y) {
        br.y = y;
      }
    }
  }

  bool TestPoint(float x, float y) const
  {
    if (x >= tl.x && x < br.x && y >= tl.y && y < br.y) {
      return true;
    }

    return false;
  }

  bool Intersect(const hgeRect &rect) const
  {
    if (std::abs(tl.x + br.x - rect.tl.x - rect.br.x) < (br.x - tl.x + rect.br.x - rect.tl.x)) {
      if (std::abs(tl.y + br.y - rect.tl.y - rect.br.y) < (br.y - tl.y + rect.br.y - rect.tl.y)) {
        return true;
      }
    }

    return false;
  }
  
  float width() const {
    return br.x - tl.x;
  }
  float height() const {
    return br.y - tl.y;
  }

  // Move preserving size
  void move_to(float x, float y) {
    float w = width();
    float h = height();
    tl.set(x, y);
    br.set(x + w, y + h);
  }

  void move_by(float dx, float dy) {
    tl.x += dx;
    br.x += dx;
    tl.y += dy;
    br.y += dy;
  }

private:
  bool m_clean = false;
};

/*
** Hardware color macros
*/
class Color {
public:
  union {
    struct {
      uint8_t a, r, g, b;
    };
    uint32_t argb;
  };
  Color() : argb(0) {}
  Color(uint32_t _argb) : argb(_argb) {}
  Color(uint8_t _a, uint8_t _r, uint8_t _g, uint8_t _b) : a(_a), r(_r), g(_g), b(_b) {}
  Color set_a(uint8_t newa) const
  {
    return Color(newa, r, g, b);
  }
};
/*
#define ARGB(a,r,g,b)   ((uint32_t(a)<<24) + (uint32_t(r)<<16) + (uint32_t(g)<<8) + uint32_t(b))
#define GETA(col)       ((col)>>24)
#define GETR(col)       (((col)>>16) & 0xFF)
#define GETG(col)       (((col)>>8) & 0xFF)
#define GETB(col)       ((col) & 0xFF)
#define SETA(col,a)     (((col) & 0x00FFFFFF) + (uint32_t(a)<<24))
#define SETR(col,r)     (((col) & 0xFF00FFFF) + (uint32_t(r)<<16))
#define SETG(col,g)     (((col) & 0xFFFF00FF) + (uint32_t(g)<<8))
#define SETB(col,b)     (((col) & 0xFFFFFF00) + uint32_t(b))
*/

struct Vertex {
  Pointf      pos; // screen position
  float       z;   // Z-buffer depth 0..1
  Color       col; // color
  Pointf      tex; // texture coordinates
};

struct hgeTriple {
  Vertex      v[3];
  HTEXTURE    tex;
  int         blend;
};


struct Quad {
  Vertex      v[4];
  HTEXTURE    tex;
  int         blend;
  Quad() {}
};



/*
** HGE Blending constants
*/
const uint32_t BLEND_COLORADD   = 1;
const uint32_t BLEND_COLORMUL   = 0;
const uint32_t BLEND_ALPHABLEND = 2;
const uint32_t BLEND_ALPHAADD   = 0;
const uint32_t BLEND_ZWRITE     = 4;
const uint32_t BLEND_NOZWRITE   = 0;

// Darken does real color multiplication, white source pixels don't change destination, while
// black source pixels make destination completely black
// Use example: http://relishgames.com/forum/index.php?p=/discussion/5799/darken-screen-plus-uneffected-hole/p1
const uint32_t BLEND_DARKEN = 8;
const uint32_t BLEND_BLACKEN = BLEND_DARKEN;

#define BLEND_DEFAULT       (BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE)
#define BLEND_DEFAULT_Z     (BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_ZWRITE)


/*
** HGE System state constants
*/
enum hgeBoolState {
  HGE_WINDOWED        = 1,    // bool     run in window?      (default: false)
  HGE_ZBUFFER         = 2,    // bool     use z-buffer?       (default: false)
  HGE_TEXTUREFILTER   = 3,    // bool     texture filtering?  (default: true)

  HGE_USESOUND        = 4,    // bool     use BASS for sound? (default: true)

  HGE_DONTSUSPEND     = 5,    // bool     focus lost:suspend? (default: false)
  HGE_HIDEMOUSE       = 6,    // bool     hide system cursor? (default: true)

  HGE_SHOWSPLASH      = 7,    // bool     hide system cursor? (default: true)

  HGEBOOLSTATE_FORCE_DWORD = 0x7FFFFFFF
};

enum hgeFuncState {
  HGE_FRAMEFUNC       = 8,    // bool*()  frame function      (default: NULL) (you MUST set this)
  HGE_RENDERFUNC      = 9,    // bool*()  render function     (default: NULL)
  HGE_FOCUSLOSTFUNC   = 10,   // bool*()  focus lost function (default: NULL)
  HGE_FOCUSGAINFUNC   = 11,   // bool*()  focus gain function (default: NULL)
  HGE_GFXRESTOREFUNC  = 12,   // bool*()  exit function       (default: NULL)
  HGE_EXITFUNC        = 13,   // bool*()  exit function       (default: NULL)

  HGEFUNCSTATE_FORCE_DWORD = 0x7FFFFFFF
};

enum hgeHwndState {
  HGE_HWND            = 15,   // int      window handle: read only
  HGE_HWNDPARENT      = 16,   // int      parent win handle   (default: 0)

  HGEHWNDSTATE_FORCE_DWORD = 0x7FFFFFFF
};

enum hgeIntState {
  HGE_SCREENWIDTH     = 17,   // int      screen width        (default: 800)
  HGE_SCREENHEIGHT    = 18,   // int      screen height       (default: 600)
  HGE_SCREENBPP       = 19,   // int      screen bitdepth     (default: 32) (desktop bpp in windowed mode)

  HGE_SAMPLERATE      = 20,   // int      sample rate         (default: 44100)
  HGE_FXVOLUME        = 21,   // int      global fx volume    (default: 100)
  HGE_MUSVOLUME       = 22,   // int      global music volume (default: 100)
  HGE_STREAMVOLUME    = 23,   // int      global music volume (default: 100)

  HGE_FPS             = 24,   // int      fixed fps           (default: HGEFPS_UNLIMITED)

  HGE_POWERSTATUS     = 25,   // int      battery life percent + status

  HGEINTSTATE_FORCE_DWORD = 0x7FFFFFF
};

enum hgeStringState {
  HGE_ICON            = 26,   // char*    icon resource       (default: NULL)
  HGE_TITLE           = 27,   // char*    window title        (default: "HGE")

  HGE_INIFILE         = 28,   // char*    ini file            (default: NULL) (meaning no file)
  HGE_LOGFILE         = 29,   // char*    log file            (default: NULL) (meaning no file)

  HGESTRINGSTATE_FORCE_DWORD = 0x7FFFFFFF
};

/*
** Callback protoype used by HGE
*/
typedef bool (*hgeCallback)();


/*
** HGE_FPS system state special constants
*/
#define HGEFPS_UNLIMITED    0
#define HGEFPS_VSYNC        -1


/*
** HGE_POWERSTATUS system state special constants
*/
#define HGEPWR_AC           -1
#define HGEPWR_UNSUPPORTED  -2


/*
** HGE Primitive type constants
*/
#define HGEPRIM_LINES       2
#define HGEPRIM_TRIPLES     3
#define HGEPRIM_QUADS       4


/*
** HGE Input Event type constants
*/
#define INPUT_KEYDOWN       1
#define INPUT_KEYUP         2
#define INPUT_MBUTTONDOWN   3
#define INPUT_MBUTTONUP     4
#define INPUT_MOUSEMOVE     5
#define INPUT_MOUSEWHEEL    6


/*
** HGE Input Event flags
*/
#define HGEINP_SHIFT        1
#define HGEINP_CTRL         2
#define HGEINP_ALT          4
#define HGEINP_CAPSLOCK     8
#define HGEINP_SCROLLLOCK   16
#define HGEINP_NUMLOCK      32
#define HGEINP_REPEAT       64

/*
** HGE Virtual-key codes
*/
#undef DELETE
enum class Key : uint32_t {
  NO_KEY     = 0x00,
  LBUTTON   = 0x01,  RBUTTON   = 0x02,  MBUTTON   = 0x04,
  ESCAPE    = 0x1B,  BACKSPACE = 0x08,  TAB       = 0x09,
  ENTER     = 0x0D,  SPACE     = 0x20,  SHIFT     = 0x10,
  CTRL      = 0x11,  ALT       = 0x12,  LWIN      = 0x5B,
  RWIN      = 0x5C,  APPS      = 0x5D,  PAUSE     = 0x13,
  CAPSLOCK  = 0x14,  NUMLOCK   = 0x90,  SCROLLLOCK = 0x91,
  PGUP      = 0x21,  PGDN      = 0x22,  HOME      = 0x24,
  END       = 0x23,  INSERT    = 0x2D,  DELETE    = 0x2E,
  LEFT      = 0x25,  UP        = 0x26,  RIGHT     = 0x27,
  DOWN      = 0x28,  NUM0      = 0x30,  NUM1      = 0x31,
  NUM2      = 0x32,  NUM3      = 0x33,  NUM4      = 0x34,
  NUM5      = 0x35,  NUM6      = 0x36,  NUM7      = 0x37,
  NUM8      = 0x38,  NUM9      = 0x39,  A         = 0x41,
  B         = 0x42,  C         = 0x43,  D         = 0x44,
  E         = 0x45,  F         = 0x46,  G         = 0x47,
  H         = 0x48,  I         = 0x49,  J         = 0x4A,
  K         = 0x4B,  L         = 0x4C,  M         = 0x4D,
  N         = 0x4E,  O         = 0x4F,  P         = 0x50,
  Q         = 0x51,  R         = 0x52,  S         = 0x53,
  T         = 0x54,  U         = 0x55,  V         = 0x56,
  W         = 0x57,  X         = 0x58,  Y         = 0x59,
  Z         = 0x5A,  GRAVE     = 0xC0,  MINUS     = 0xBD,
  EQUALS    = 0xBB,  BACKSLASH = 0xDC,  LBRACKET  = 0xDB,
  RBRACKET  = 0xDD,  SEMICOLON = 0xBA,  APOSTROPHE = 0xDE,
  COMMA     = 0xBC,  PERIOD    = 0xBE,  SLASH     = 0xBF,
  NUMPAD0   = 0x60,  NUMPAD1   = 0x61,  NUMPAD2   = 0x62,
  NUMPAD3   = 0x63,  NUMPAD4   = 0x64,  NUMPAD5   = 0x65,
  NUMPAD6   = 0x66,  NUMPAD7   = 0x67,  NUMPAD8   = 0x68,
  NUMPAD9   = 0x69,  MULTIPLY  = 0x6A,  DIVIDE    = 0x6F,
  ADD       = 0x6B,  SUBTRACT  = 0x6D,  DECIMAL   = 0x6E,
  F1        = 0x70,  F2        = 0x71,  F3        = 0x72,
  F4        = 0x73,  F5        = 0x74,  F6        = 0x75,
  F7        = 0x76,  F8        = 0x77,  F9        = 0x78,
  F10       = 0x79,  F11       = 0x7A,  F12       = 0x7B
};

struct InputEvent {
  int     type;           // event type
  Key     key;            // key code
  int     flags;          // event flags
  int     chr;            // character code
  Key     wheel;          // wheel shift
  float   x;              // mouse cursor x-coordinate
  float   y;              // mouse cursor y-coordinate
};


/*
** HGE Interface class
*/
class HGE {
public:
  virtual void        HGE_CALL    Release() = 0;

  virtual bool        HGE_CALL    System_Initiate() = 0;
  virtual void        HGE_CALL    System_Shutdown() = 0;
  virtual bool        HGE_CALL    System_Start() = 0;
  virtual char       *HGE_CALL    System_GetErrorMessage() = 0;
  virtual void        HGE_CALL    System_Log(const char *format, ...) = 0;
  virtual bool        HGE_CALL    System_Launch(const char *url) = 0;
  virtual void        HGE_CALL    System_Snapshot(const char *filename = 0) = 0;

private:
  virtual void        HGE_CALL    System_SetStateBool(hgeBoolState   state, bool        value) = 0;
  virtual void        HGE_CALL    System_SetStateFunc(hgeFuncState   state, hgeCallback value) = 0;
  virtual void        HGE_CALL    System_SetStateHwnd(hgeHwndState   state, HWND        value) = 0;
  virtual void        HGE_CALL    System_SetStateInt(hgeIntState    state, int         value) = 0;
  virtual void        HGE_CALL    System_SetStateString(hgeStringState state, const char *value) = 0;
  virtual bool        HGE_CALL    System_GetStateBool(hgeBoolState   state) = 0;
  virtual hgeCallback HGE_CALL    System_GetStateFunc(hgeFuncState   state) = 0;
  virtual HWND        HGE_CALL    System_GetStateHwnd(hgeHwndState   state) = 0;
  virtual int         HGE_CALL    System_GetStateInt(hgeIntState    state) = 0;
  virtual const char *HGE_CALL    System_GetStateString(hgeStringState state) = 0;

public:
  inline void                 System_SetState(hgeBoolState   state, bool        value)
  {
    System_SetStateBool(state, value);
  }
  inline void                 System_SetState(hgeFuncState   state, hgeCallback value)
  {
    System_SetStateFunc(state, value);
  }
  inline void                 System_SetState(hgeHwndState   state, HWND        value)
  {
    System_SetStateHwnd(state, value);
  }
  inline void                 System_SetState(hgeIntState    state, int         value)
  {
    System_SetStateInt(state, value);
  }
  inline void                 System_SetState(hgeStringState state, const char *value)
  {
    System_SetStateString(state, value);
  }
  inline bool                 System_GetState(hgeBoolState   state)
  {
    return System_GetStateBool(state);
  }
  inline hgeCallback          System_GetState(hgeFuncState   state)
  {
    return System_GetStateFunc(state);
  }
  inline HWND                 System_GetState(hgeHwndState   state)
  {
    return System_GetStateHwnd(state);
  }
  inline int                  System_GetState(hgeIntState    state)
  {
    return System_GetStateInt(state);
  }
  inline const char          *System_GetState(hgeStringState state)
  {
    return System_GetStateString(state);
  }

  virtual void       *HGE_CALL    Resource_Load(const char *filename, uint32_t *size = 0) = 0;
  virtual void        HGE_CALL    Resource_Free(void *res) = 0;
  virtual bool        HGE_CALL    Resource_AttachPack(const char *filename,
      const char *password = 0) = 0;
  virtual void        HGE_CALL    Resource_RemovePack(const char *filename) = 0;
  virtual void        HGE_CALL    Resource_RemoveAllPacks() = 0;
  virtual char       *HGE_CALL    Resource_MakePath(const char *filename = 0) = 0;
  virtual char       *HGE_CALL    Resource_EnumFiles(const char *wildcard = 0) = 0;
  virtual char       *HGE_CALL    Resource_EnumFolders(const char *wildcard = 0) = 0;

  virtual void        HGE_CALL    Ini_SetInt(const char *section, const char *name, int value) = 0;
  virtual int         HGE_CALL    Ini_GetInt(const char *section, const char *name, int def_val) = 0;
  virtual void        HGE_CALL    Ini_SetFloat(const char *section, const char *name,
      float value) = 0;
  virtual float       HGE_CALL    Ini_GetFloat(const char *section, const char *name,
      float def_val) = 0;
  virtual void        HGE_CALL    Ini_SetString(const char *section, const char *name,
      const char *value) = 0;
  virtual char       *HGE_CALL    Ini_GetString(const char *section, const char *name,
      const char *def_val) = 0;

  virtual void        HGE_CALL    Random_Seed(int seed = 0) = 0;
  virtual int         HGE_CALL    Random_Int(int min, int max) = 0;
  virtual float       HGE_CALL    Random_Float(float min, float max) = 0;

  virtual float       HGE_CALL    Timer_GetTime() = 0;
  virtual float       HGE_CALL    Timer_GetDelta() = 0;
  virtual int         HGE_CALL    Timer_GetFPS() = 0;

  //virtual HEFFECT     HGE_CALL    Effect_Load(const char *filename, uint32_t size=0) = 0;
  //virtual void        HGE_CALL    Effect_Free(HEFFECT eff) = 0;
  //virtual HCHANNEL    HGE_CALL    Effect_Play(HEFFECT eff) = 0;
  //virtual HCHANNEL    HGE_CALL    Effect_PlayEx(HEFFECT eff, int volume=100, int pan=0, float pitch=1.0f, bool loop=false) = 0;

  //virtual HMUSIC      HGE_CALL    Music_Load(const char *filename, uint32_t size=0) = 0;
  //virtual void        HGE_CALL    Music_Free(HMUSIC mus) = 0;
  //virtual HCHANNEL    HGE_CALL    Music_Play(HMUSIC mus, bool loop, int volume = 100, int order = -1, int row = -1) = 0;
  //virtual void        HGE_CALL    Music_SetAmplification(HMUSIC music, int ampl) = 0;
  //virtual int         HGE_CALL    Music_GetAmplification(HMUSIC music) = 0;
  //virtual int         HGE_CALL    Music_GetLength(HMUSIC music) = 0;
  //virtual void        HGE_CALL    Music_SetPos(HMUSIC music, int order, int row) = 0;
  //virtual bool        HGE_CALL    Music_GetPos(HMUSIC music, int *order, int *row) = 0;
  //virtual void        HGE_CALL    Music_SetInstrVolume(HMUSIC music, int instr, int volume) = 0;
  //virtual int         HGE_CALL    Music_GetInstrVolume(HMUSIC music, int instr) = 0;
  //virtual void        HGE_CALL    Music_SetChannelVolume(HMUSIC music, int channel, int volume) = 0;
  //virtual int         HGE_CALL    Music_GetChannelVolume(HMUSIC music, int channel) = 0;

  //virtual HSTREAM     HGE_CALL    Stream_Load(const char *filename, uint32_t size=0) = 0;
  //virtual void        HGE_CALL    Stream_Free(HSTREAM stream) = 0;
  //virtual HCHANNEL    HGE_CALL    Stream_Play(HSTREAM stream, bool loop, int volume = 100) = 0;

  //virtual void        HGE_CALL    Channel_SetPanning(HCHANNEL chn, int pan) = 0;
  //virtual void        HGE_CALL    Channel_SetVolume(HCHANNEL chn, int volume) = 0;
  //virtual void        HGE_CALL    Channel_SetPitch(HCHANNEL chn, float pitch) = 0;
  //virtual void        HGE_CALL    Channel_Pause(HCHANNEL chn) = 0;
  //virtual void        HGE_CALL    Channel_Resume(HCHANNEL chn) = 0;
  //virtual void        HGE_CALL    Channel_Stop(HCHANNEL chn) = 0;
  //virtual void        HGE_CALL    Channel_PauseAll() = 0;
  //virtual void        HGE_CALL    Channel_ResumeAll() = 0;
  //virtual void        HGE_CALL    Channel_StopAll() = 0;
  //virtual bool        HGE_CALL    Channel_IsPlaying(HCHANNEL chn) = 0;
  //virtual float       HGE_CALL    Channel_GetLength(HCHANNEL chn) = 0;
  //virtual float       HGE_CALL    Channel_GetPos(HCHANNEL chn) = 0;
  //virtual void        HGE_CALL    Channel_SetPos(HCHANNEL chn, float fSeconds) = 0;
  //virtual void        HGE_CALL    Channel_SlideTo(HCHANNEL channel, float time, int volume, int pan = -101, float pitch = -1) = 0;
  //virtual bool        HGE_CALL    Channel_IsSliding(HCHANNEL channel) = 0;

  virtual Pointi32    HGE_CALL    Input_GetMousePos() = 0;
  virtual void      HGE_CALL    Input_SetMousePos(const Pointi32 &pos) = 0;
  virtual int         HGE_CALL    Input_GetMouseWheel() = 0;
  virtual bool        HGE_CALL    Input_IsMouseOver() = 0;
  virtual bool        HGE_CALL    Input_KeyDown(Key key) = 0;
  virtual bool        HGE_CALL    Input_KeyUp(Key key) = 0;
  virtual bool        HGE_CALL    Input_GetKeyState(Key key) = 0;
  virtual char       *HGE_CALL    Input_GetKeyName(Key key) = 0;
  virtual Key         HGE_CALL    Input_GetKey() = 0;
  virtual int         HGE_CALL    Input_GetChar() = 0;
  virtual bool        HGE_CALL    Input_GetEvent(InputEvent *event) = 0;

  virtual bool        HGE_CALL    Gfx_BeginScene(HTARGET target = 0) = 0;
  virtual void        HGE_CALL    Gfx_EndScene() = 0;
  virtual void        HGE_CALL    Gfx_Clear(uint32_t color) = 0;
  virtual void        HGE_CALL    Gfx_RenderLine(float x1, float y1, float x2, float y2,
      uint32_t color = 0xFFFFFFFF, float z = 0.5f) = 0;
  virtual void        HGE_CALL    Gfx_RenderTriple(const hgeTriple *triple) = 0;
  virtual void        HGE_CALL    Gfx_RenderQuad(const Quad *quad) = 0;
  virtual Vertex  *HGE_CALL    Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend,
      int *max_prim) = 0;
  virtual void        HGE_CALL    Gfx_FinishBatch(int nprim) = 0;
  virtual void        HGE_CALL    Gfx_SetClipping(int x = 0, int y = 0, int w = 0, int h = 0) = 0;
  virtual void        HGE_CALL    Gfx_SetTransform(float x = 0, float y = 0, float dx = 0,
      float dy = 0, float rot = 0, float hscale = 0, float vscale = 0) = 0;

#if HGE_DIRECTX_VER >= 9
  virtual HSHADER   HGE_CALL  Shader_Create(const char *filename) = 0;
  virtual void    HGE_CALL  Shader_Free(HSHADER shader) = 0;
  virtual void    HGE_CALL  Gfx_SetShader(HSHADER shader) = 0;
#endif

  virtual HTARGET     HGE_CALL    Target_Create(int width, int height, bool zbuffer) = 0;
  virtual void        HGE_CALL    Target_Free(HTARGET target) = 0;
  virtual HTEXTURE    HGE_CALL    Target_GetTexture(HTARGET target) = 0;

  virtual HTEXTURE    HGE_CALL    Texture_Create(int width, int height) = 0;
  virtual HTEXTURE    HGE_CALL    Texture_Load(const char *filename, uint32_t size = 0,
      bool bMipmap = false) = 0;
  virtual void        HGE_CALL    Texture_Free(HTEXTURE tex) = 0;
  virtual int     HGE_CALL    Texture_GetWidth(HTEXTURE tex, bool bOriginal = false) = 0;
  virtual int     HGE_CALL    Texture_GetHeight(HTEXTURE tex, bool bOriginal = false) = 0;
  virtual uint32_t   *HGE_CALL    Texture_Lock(HTEXTURE tex, bool bReadOnly = true, int left = 0,
      int top = 0, int width = 0, int height = 0) = 0;
  virtual void    HGE_CALL    Texture_Unlock(HTEXTURE tex) = 0;
};

} // ns hge

using HGE = hge::HGE;
extern "C" {
  HGE_EXPORT HGE *HGE_CALL hgeCreate(int ver);
}



