#ifndef _KILIB_STDAFX_H_
#define _KILIB_STDAFX_H_

#undef   WINVER
#define  WINVER      0x0400
#undef  _WIN32_IE
#define _WIN32_IE    0x0200
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0400

#define  OEMRESOURCE
#define  NOMINMAX
#ifdef SUPERTINY
  #define memset memset_default
#endif

#ifdef __GNUC__
// Define some cool gcc attributes
#define A_HOT __attribute__((hot))
#define A_COLD __attribute__((cold))
#define A_PURE __attribute__((pure))
#define A_XPURE __attribute__((const))
#define A_FLATTEN __attribute__((flatten))
#else //__GNUC__
#define A_HOT
#define A_COLD
#define A_PURE
#define A_XPURE
#define A_FLATTEN
#endif

#if defined (__cplusplus) && __cplusplus >= 201103L
	// Define the cool new 'final' class attribute
	// Introduced in C++11
	#define A_FINAL final
#else
	#define A_FINAL
	#define override
#endif

#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>
#include <commctrl.h>
#include <imm.h>
// dimm.hが無くてエラーになる場合、プロジェクトの設定でUSEGLOBALIMEの定義を
// 削除するか、最新の Platform SDK を導入すればビルドが通るようになります。
#ifdef USEGLOBALIME
#include <dimm.h>
#endif

#ifdef _UNICODE
  #define UNICODEBOOL true
#else
  #define UNICODEBOOL false
#endif

#ifndef NO_MLANG
#include <mlang.h>
#endif

#ifdef SUPERTINY
  #undef memset
#endif

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL 0x00400000
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#ifndef OFN_ENABLESIZING
#define OFN_ENABLESIZING 0x00800000
#endif

#ifndef IMR_RECONVERTSTRING
#define IMR_RECONVERTSTRING             0x0004
typedef struct tagRECONVERTSTRING {
    DWORD dwSize;
    DWORD dwVersion;
    DWORD dwStrLen;
    DWORD dwStrOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwTargetStrLen;
    DWORD dwTargetStrOffset;
} RECONVERTSTRING, *PRECONVERTSTRING, NEAR *NPRECONVERTSTRING, FAR *LPRECONVERTSTRING;
#endif

#ifndef IMR_CONFIRMRECONVERTSTRING
#define IMR_CONFIRMRECONVERTSTRING      0x0005
#endif

#ifndef WM_IME_REQUEST
#define WM_IME_REQUEST                  0x0288
#endif

#ifdef _MSC_VER
#pragma warning( disable: 4355 )
#endif

#if defined(__DMC__) || (defined(_MSC_VER) && _MSC_VER < 1300)
	#define SetWindowLongPtr SetWindowLong
	#define GetWindowLongPtr GetWindowLong
	#define UINT_PTR         UINT
	#define LONG_PTR         LONG
	#define GWLP_WNDPROC     GWL_WNDPROC
	#define GWLP_USERDATA    GWL_USERDATA
#endif

#if defined(TARGET_VER) && TARGET_VER <= 303
	// On Windows NT 3.10.340 MessageBox does not exists!
  #undef MessageBox

  #ifdef _UNICODE
	#define MessageBox(a, b, c, d) MessageBoxExW(a, b, c, d, 0)

    UINT myDragQueryFileW(HDROP hd, UINT i, LPWSTR wpath, UINT l);
    #define myDragQueryFile myDragQueryFileW
  #else
	#define MessageBox(a, b, c, d) MessageBoxExA(a, b, c, d, 0)
	#define myDragQueryFile DragQueryFileA
  #endif

#else // TARGET_VER >= 303
	// Default implementation
	#define myDragQueryFileW DragQueryFileW
	#define myDragQueryFileA DragQueryFileA
	#ifdef _UNICODE
		#define myDragQueryFile myDragQueryFileW
	#else
		#define myDragQueryFile myDragQueryFileA
	#endif
#endif // TARGET_VER

#endif // _KILIB_STDAFX_H_
