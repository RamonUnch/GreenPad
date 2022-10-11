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
typedef unsigned long ulong;

// 配列の要素数
#define countof(_array) (sizeof(_array)/sizeof(_array[0]))

// 大きい方、小さい方
template<typename T> inline T Min(T x,T y) { return (x<y ? x : y); }
template<typename T> inline T Max(T x,T y) { return (y<x ? x : y); }
template<typename T> inline T Clamp(T l, T x,T h) { return (x < l)? l: ((x > h)? h: x); }
template<typename T> inline T NZero(T x)   { return (x==0? 1 : x); }

// 古いC++処理系でも、forで使う変数のスコープを強制的に制限
#if defined(_MSC_VER) || defined(__DMC__)
#define for if(0);else for
#endif

// コピー禁止オブジェクト
#define NOCOPY(T) T( const T& ); T& operator=( const T& )

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

#endif // _KILIB_TYPES_H_
