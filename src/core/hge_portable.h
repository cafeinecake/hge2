#pragma once

#if (defined(__APPLE__) && defined(__MACH__))
#   define HGE_MACOSX   1
#   define HGE_POSIX    1
#endif

#if ( defined(unix) || defined(__linux__) || PLATFORM_MACOSX )
#   define HGE_UNIX   1
#   define HGE_POSIX  1
#endif

// Useful to sprinkle around the codebase without a bunch of #ifdefs...
#ifdef _WINDOWS
#   define BYTESWAP(x)
#   define STUBBED(x)

#   define HGE_WINDOWS  1


#   include <SDL.h>
#   include <Windows.h>
#   include <wchar.h>

#   define HGE_NORETURN /*no attribute on Windows platform*/
#   define HGE_MAX_PATH MAX_PATH
#   define snprintf hgeos::c99_snprintf
namespace hgeos {
  int c99_snprintf(char* str, size_t size, const char* format, ...);
  int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap);
} // ns hgeos
#   define strcasecmp _stricmp
#   define strncasecmp _strnicmp 

#   define hgeAssert _ASSERTE


#endif

// don't want rest of this header on Windows, etc.
#if (PLATFORM_UNIX)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <hgeAssert.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "SDL.h"

#define HGE_MAX_PATH PATH_MAX

typedef void *HANDLE;
typedef HANDLE HWND;

static inline uint32_t timeGetTime(void)
{
  return SDL_GetTicks();
} // timeGetTime


// macro so I know what is still on the TODO list...
#if 1
#define STUBBED(x)
#else
void CalledSTUBBED(void);  // you can set a breakpoint on this.
#define STUBBED(x) \
do { \
    static bool seen_this = false; \
    if (!seen_this) { \
        seen_this = true; \
        fprintf(stderr, "STUBBED: %s at %s (%s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
        fflush(stderr); \
        CalledSTUBBED(); \
    } \
} while (false)
#endif

static inline char *itoa(const int i, char *s, const int radix)
{
  hgeAssert(radix == 10);
  sprintf(s, "%d", i);
  return s;
}

static inline char *_i64toa(const int64_t i, char *s, const int radix)
{
  hgeAssert(radix == 10);
  hgeAssert(sizeof(long long) == sizeof(int64_t));
  sprintf(s, "%lld", static_cast<long long>(i));
  return s;
}

static inline int64_t _atoi64(const char *str)
{
  return static_cast<int64_t>(strtoll(str, NULL, 10));
}

static inline void Sleep(const int ms)
{
  usleep(static_cast<__useconds_t>(ms * 1000));
}

static inline char *_gcvt(const double value, const int digits, char *buffer)
{
  char fmt[32];
  snprintf(fmt, sizeof(fmt), "%%.%dg", digits);
  sprintf(buffer, fmt, value);
  return buffer;
}

#define ZeroMemory(a,b) memset(a, '\0', b)

#ifdef __cplusplus
#ifdef max
#undef max
#endif
template <class T> inline const T &max(const T &a, const T &b)
{
  return (a > b) ? a : b;
}
#ifdef min
#undef min
#endif
template <class T> inline const T &min(const T &a, const T &b)
{
  return (a < b) ? a : b;
}
#endif

// Byteswap magic...

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define PLATFORM_BIGENDIAN 1
#define PLATFORM_LITTLEENDIAN 0
#else
#define PLATFORM_BIGENDIAN 0
#define PLATFORM_LITTLEENDIAN 1
#endif

#if PLATFORM_BIGENDIAN
#define SWAPPER64(t) \
        inline void BYTESWAP(t &x) { \
            union { t orig; Uint64 ui; } swapper; \
            swapper.orig = x; \
            swapper.ui = SDL_SwapLE64(swapper.ui); \
            x = swapper.orig; \
        }
#define SWAPPER32(t) \
        inline void BYTESWAP(t &x) { \
            union { t orig; Uint32 ui; } swapper; \
            swapper.orig = x; \
            swapper.ui = SDL_SwapLE32(swapper.ui); \
            x = swapper.orig; \
        }
#define SWAPPER16(t) \
        inline void BYTESWAP(t &x) { \
            union { t orig; Uint16 ui; } swapper; \
            swapper.orig = x; \
            swapper.ui = SDL_SwapLE16(swapper.ui); \
            x = swapper.orig; \
        }
#define SWAPPER8(t) inline void BYTESWAP(t &_x) {}
SWAPPER64(double)
SWAPPER32(size_t)   // !!! FIXME: this will fail on gnuc/amd64.
SWAPPER32(int)
SWAPPER32(float)
SWAPPER32(DWORD)
SWAPPER16(WORD)
SWAPPER16(short)
SWAPPER8(BYTE)
#undef SWAPPER32
#undef SWAPPER16
#undef SWAPPER8
#else
#define BYTESWAP(x)
#endif

#define HGE_NORETURN __attribute__((__noreturn__))

#endif  // PLATFORM_UNIX
