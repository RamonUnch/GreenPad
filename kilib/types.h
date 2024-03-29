#ifndef _KILIB_TYPES_H_
#define _KILIB_TYPES_H_

//=========================================================================
//@{ @pkg ki.Types //@}
//=========================================================================

// 変数のサイズを明示的に指示するときに使う名前
typedef unsigned char  byte;
typedef unsigned short dbyte;
typedef unsigned long  qbyte;
typedef wchar_t unicode;

// unsigned って毎回打つの面倒
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

#define CHAR_DIGITS 4
#define UCHAR_DIGITS 3
#define SHORT_DIGITS 6
#define UHORT_DIGITS 5
#define INT_DIGITS 11
#define UINT_DIGITS 10
#define LONG_DIGITS 11

#ifdef WIN64
typedef unsigned long long ulong;
#define ULONG_DIGITS 20
#define SIZET_DIGITS 20
#else
typedef unsigned long ulong;
#define ULONG_DIGITS 10
#define SIZET_DIGITS 10
#endif

// 配列の要素数
#define countof(_array) (sizeof(_array)/sizeof(_array[0]))

// 大きい方、小さい方
template<typename T> inline T Min(T x,T y) { return (x<y ? x : y); }
template<typename T> inline T Max(T x,T y) { return (y<x ? x : y); }
template<typename T> inline T Abs(T x) { return (x<0 ? -x : x); }
template<typename T> inline T Clamp(T l, T x,T h) { return (x < l)? l: ((x > h)? h: x); }
template<typename T> inline T NZero(T x)   { return (x==0? 1 : x); }

inline bool isUDigit(TCHAR x) { return TEXT('0') <= x && x <= TEXT('9'); }
inline bool isSDigit(TCHAR x) { return isUDigit(x) || x == TEXT('-'); }

// 古いC++処理系でも、forで使う変数のスコープを強制的に制限
#if defined(_MSC_VER) || defined(__DMC__)
#define for if(0);else for
#endif

// コピー禁止オブジェクト
#if __cplusplus >= 201103L
// In C++ 2011 we can use the = delete initializers.
#define NOCOPY(T) T( const T& )=delete; T& operator=( const T& )=delete
#else // C++ 98/05
#define NOCOPY(T) T( const T& ); T& operator=( const T& )
#endif

#ifndef GS_8BIT_INDICES
#define GS_8BIT_INDICES 0x00000001
typedef struct tagWCRANGE {
  WCHAR wcLow;
  USHORT cGlyphs;
} WCRANGE,*PWCRANGE,*LPWCRANGE;

typedef struct tagGLYPHSET {
  DWORD   cbThis;
  DWORD   flAccel;
  DWORD   cGlyphsSupported;
  DWORD   cRanges;
  WCRANGE ranges[1];
} GLYPHSET, *PGLYPHSET, *LPGLYPHSET;
#endif

#ifndef SPI_GETWHEELSCROLLCHARS
#define SPI_GETWHEELSCROLLCHARS 0x006C
#endif

#endif // _KILIB_TYPES_H_
