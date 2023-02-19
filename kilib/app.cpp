#include "stdafx.h"
#include "app.h"
#include "log.h"
#include "memory.h"
#include "thread.h"
#include "window.h"
#include "string.h"
using namespace ki;

#ifndef NO_OLE32
typedef HRESULT (WINAPI * Initialize_funk)(LPVOID r);
static HRESULT MyOleInitialize(LPVOID r)
{
	Initialize_funk func = (Initialize_funk)GetProcAddress(app().hOle32(), "OleInitialize");

	if (func) { // We got the function!
		return func(r);
	}
	return 666; // Fail with 666 error.
}
static HRESULT MyCoInitialize(LPVOID r)
{
	Initialize_funk func = (Initialize_funk)GetProcAddress(app().hOle32(), "CoInitialize");

	if (func) { // We got the function!
		return func(r);
	}
	return 666; // Fail with 666 error.
}

typedef void (WINAPI * UnInitialize_funk)( );
static void MyOleUninitialize( )
{
	UnInitialize_funk func = (UnInitialize_funk)GetProcAddress(app().hOle32(), "OleUninitialize");

	if (func) { // We got the function!
		func();
	}
}
typedef HRESULT (WINAPI * CoCreateInstance_funk)(REFCLSID , LPUNKNOWN , DWORD , REFIID , LPVOID *);
HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	static CoCreateInstance_funk func = (CoCreateInstance_funk)(-1);
	if (func == (CoCreateInstance_funk)(-1)) // First time!
		func = (CoCreateInstance_funk)GetProcAddress(app().hOle32(), "CoCreateInstance");

	if (func)
	{ // We got the function!
		HRESULT ret = func(rclsid, pUnkOuter, dwClsContext, riid, ppv);
		#ifdef WIN32S
		// On Win32s HRESULTS can return S_OK instead of E_NOTIMPL
		// and only LastError is set to E_NOTIMPL
		if (ret == S_OK  && GetLastError() == E_NOTIMPL )
			ret = E_NOTIMPL;
		#endif
		return ret;
	}
	return 666; // Fail with 666 error
}
HRESULT MyCoLockObjectExternal(IUnknown * pUnk, BOOL fLock, BOOL fLastUnlockReleases)
{
	#define FUNK_TYPE ( HRESULT (WINAPI *)(IUnknown *, BOOL, BOOL) )

	static HRESULT (WINAPI *dyn_CoLockObjectExternal)(IUnknown *, BOOL, BOOL) = FUNK_TYPE(-1);
	if( dyn_CoLockObjectExternal == FUNK_TYPE(-1))
		dyn_CoLockObjectExternal = FUNK_TYPE GetProcAddress(app().hOle32(), "CoLockObjectExternal");

	if( dyn_CoLockObjectExternal )
		return dyn_CoLockObjectExternal(pUnk, fLock, fLastUnlockReleases);
	#undef FUNK_TYPE

	return E_NOTIMPL;
}
#endif // NO_OLE32

#if defined(TARGET_VER) && TARGET_VER <= 350
static BOOL MyGetVersionEx(LPOSVERSIONINFOA s_osVer)
{
	// Try first to get the real GetVersionEx function
	// We use the ANSI version because it does not matter.
	typedef BOOL (WINAPI *GetVersionEx_funk)(LPOSVERSIONINFOA s_osVer);
	GetVersionEx_funk func = (GetVersionEx_funk)
		GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "GetVersionExA");
	if (func && func( s_osVer ))
	{
		if (s_osVer->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{   // fixup broken build number in early 9x builds (roytam1)
			s_osVer->dwBuildNumber &= 0xffff;

			if( s_osVer->dwMajorVersion == 4
			&&  s_osVer->dwMinorVersion == 0
			&&  s_osVer->dwBuildNumber  == 0 )
			{	// RAMON: Windows 4.00.0 with GetVersionEx
				// This means we have at least build 99 of chicago
				s_osVer->dwBuildNumber = 99;
			}
		}
		// Only return if we got a Major version (in case)
		if (s_osVer->dwMajorVersion)
			return TRUE;
	}

	// Fallback in case the above failed (WinNT 3.1 / Win32s)
	DWORD dwVersion = ::GetVersion();
	s_osVer->dwOSVersionInfoSize = 0; // Indicate the fallback.

	s_osVer->dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	s_osVer->dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

	if (dwVersion < 0x80000000)
	{ // WINNT => build number = HIWORD(dwversion)
		s_osVer->dwBuildNumber = (DWORD)(HIWORD(dwVersion));
		s_osVer->dwPlatformId = VER_PLATFORM_WIN32_NT;
	}

	if (s_osVer->dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		s_osVer->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
		s_osVer->dwBuildNumber = 0; // No available Build number on 9x...

		#ifdef WIN32S
		if (dwVersion == 0x80000a3f)
		{ // Win32s beta build 61 (Makes no sense!)
		  // It identifies as Windows 63.10...
			s_osVer->dwMajorVersion = 3;
			s_osVer->dwPlatformId = VER_PLATFORM_WIN32s;
			s_osVer->dwBuildNumber = 61;
			return TRUE;
		}
		if (s_osVer->dwMajorVersion == 3)
		{
			// Windows 3 => we are using Win32s...
			s_osVer->dwPlatformId = VER_PLATFORM_WIN32s;
			// Get real build bumber removing the most significant bit.
			s_osVer->dwBuildNumber = HIWORD(dwVersion)&(~0x8000);
		}
		#endif // WIN32S
	}

	return TRUE;
}
#else // TARGET_VER > 3.50
	#define MyGetVersionEx GetVersionExA
#endif

//=========================================================================

App* App::pUniqueInstance_;

inline App::App()
	: osver_       (init_osver())
	, hOle32_      ((HINSTANCE)(-1))
	, exitcode_    (-1)
	, loadedModule_(0)
	, hInst_       (::GetModuleHandle(NULL))
	, hInstComCtl_ (NULL)
{
	// 唯一のインスタンスは私です。
	pUniqueInstance_ = this;
}

#pragma warning( disable : 4722 ) // 警告：デストラクタに値が戻りません
App::~App()
{
	// ロード済みモジュールがあれば閉じておく
#ifndef NO_OLE32
	if( hOle32_ && hOle32_ != (HINSTANCE)(-1) )
	{
	//	if( loadedModule_ & COM )
	//		::MyCoUninitialize();
		if( loadedModule_ & OLE )
			::MyOleUninitialize();

		::FreeLibrary( hOle32_ );
	}

	if( hInstComCtl_ )
		::FreeLibrary( hInstComCtl_ );
#endif

	// 終～了～
	::ExitProcess( exitcode_ );
}

inline void App::SetExitCode( int code )
{
	// 終了コードを設定
	exitcode_ = code;
}

void App::InitModule( imflag what )
{
#ifndef NO_OLE32
	if (hOle32_ == (HINSTANCE)(-1) && what&(OLE|COM|OLEDLL))
		hOle32_ = hasSysDLL(TEXT("OLE32.DLL"))? ::LoadLibrary(TEXT("OLE32.DLL")): NULL;
#endif
	// 初期化済みでなければ初期化する
	bool ret = true;
	if( !(loadedModule_ & what) )
		switch( what )
		{
		case CTL: {
			// ::InitCommonControls();
			if( !hInstComCtl_ )
				hInstComCtl_ = hasSysDLL(TEXT("COMCTL32.DLL"))? ::LoadLibrary(TEXT("COMCTL32.DLL")): NULL;
			if( hInstComCtl_ )
			{
				void (WINAPI *dyn_InitCommonControls)(void) = ( void (WINAPI *)(void) )
					GetProcAddress( hInstComCtl_, "InitCommonControls" );
				if (dyn_InitCommonControls)
					dyn_InitCommonControls();
			}
			} break;
		//case COM:
			// Actually we only ever use OLE, that calls COM, so it can
			// be Ignored safely...
			//ret = hOle32_ && S_OK == ::MyCoInitialize( NULL );
			//MessageBoxA(NULL, "CoInitialize", ret?"Sucess": "Failed", MB_OK);
			//break;
		case OLE:
			#ifndef NO_OLE32
			ret = hOle32_ && S_OK == ::MyOleInitialize( NULL );
			#endif
			// MessageBoxA(NULL, "OleInitialize", ret?"Sucess": "Failed", MB_OK);
			break;
		default: break;
		}

	// 今回初期化したモノを記憶
	if (ret) loadedModule_ |= what;
}
bool App::hasSysDLL(const TCHAR *dllname) const
{
#ifdef WIN32S
	if( isWin32s() )
	{	// Only used for Win32s because LoadLibrary()
		// Shows an error dialog box otherwise.
		TCHAR fp[MAX_PATH];
		UINT len = GetSystemDirectory( fp, countof(fp) );
		my_lstrcpy( fp+len, TEXT("\\WIN32S\\") );
		my_lstrcpy( fp+len+8, dllname );

		return 0xffffffff != GetFileAttributes(fp);
	}
#endif
	return true;
}
void App::Exit( int code )
{
	// 終了コードを設定して
	SetExitCode( code );

	// 自殺
	this->~App();
}



//-------------------------------------------------------------------------

MYVERINFO App::init_osver()
{
	// 初回だけは情報取得
	OSVERSIONINFOA v;
	mem00( &v, sizeof(v) );
	v.dwOSVersionInfoSize = sizeof( OSVERSIONINFOA );
	MyGetVersionEx( &v );

	#ifdef DO_LOGGING
	TCHAR buf[256];
	::wsprintf(buf,
		TEXT("%s %u.%u build %u (%hs) - %s")
		, v.dwPlatformId==VER_PLATFORM_WIN32_NT? TEXT("Windows NT")
		: v.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS? TEXT("Windows")
		: v.dwPlatformId==VER_PLATFORM_WIN32s? TEXT("Win32s"): TEXT("UNKNOWN")
		, v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber
		, v.szCSDVersion
		, v.dwOSVersionInfoSize? TEXT("GetVersionEx()"): TEXT("GetVersion()")
	);
	//MessageBox(NULL, buf, TEXT("Windows Version"), 0);
	LOGGERS( buf );
	#endif // DO_LOGGING

	MYVERINFO mv;
	mv.wFromWhichAPI = (WORD)v.dwOSVersionInfoSize != 0;
	mv.wPlatform =     (WORD)v.dwPlatformId;
	mv.v.dwVer = MKVER(v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);

	return mv;
}

DWORD App::getOOSVer() const
{
	return osver_.v.dwVer;
}

WORD App::getOSVer() const
{
	return osver_.v.vb.ver.wVer;
}

WORD App::getOSBuild() const
{
	return osver_.v.vb.wBuild;
}

bool App::isOSVerEqual(DWORD ver) const
{
	return osver_.v.dwVer == ver;
}

bool App::isNTOSVerEqual(DWORD ver) const
{
	return isNT() && osver_.v.dwVer == ver;
}
bool App::is9xOSVerEqual(DWORD ver) const
{
#if defined(WIN64)
	return false;
#else
	return !isNT() && osver_.v.dwVer == ver;
#endif
}

bool App::isOSVerLarger(DWORD ver) const
{
	return ver <= osver_.v.dwVer;
}

bool App::isNTOSVerLarger(DWORD ver) const
{
	return isNT() && ver <= osver_.v.dwVer;
}

bool App::is9xOSVerLarger(DWORD ver) const
{
#if defined(WIN64)
	return false;
#else
	return !isNT() &&  ver <= osver_.v.dwVer;
#endif
}

bool App::isNewTypeWindows() const
{
#if defined(WIN64)
	return true;
#else
	return (
		( osver_.wPlatform==VER_PLATFORM_WIN32_NT      && osver_.v.vb.ver.wVer >= 0x0500 ) // 5.0
	 || ( osver_.wPlatform==VER_PLATFORM_WIN32_WINDOWS && osver_.v.vb.ver.wVer >= 0x040A ) // 4.10
	);
#endif
}

bool App::isWin95() const
{
#if defined(WIN64)
	return false;
#else
#if defined(_M_IX86) || defined(_M_AMD64)
	// Not sure for which CPU this stupid optimization is safe...
	struct midosver{ WORD a; WORD dwPlatVer; WORD b; WORD c; };
	// Ugly cast to take the middle part of the version info (PLAT|wVER)
	DWORD platver = *(DWORD*)&((const struct midosver*)&osver_)->dwPlatVer;
	return platver == 0x00010400;
#else
	return (
		osver_.wPlatform==VER_PLATFORM_WIN32_WINDOWS &&
		osver_.v.vb.ver.wVer == 0x0400
	);
#endif

#endif // WIN64
}

bool App::isNT() const
{
#if defined(WIN64) || defined(UNICODE) && !defined(UNICOWS)
	return true;
#else
	return osver_.wPlatform==(WORD)VER_PLATFORM_WIN32_NT;
#endif
}

bool App::isWin32s() const
{
#ifndef WIN32S
	return false;
#else
	return osver_.wPlatform==(WORD)VER_PLATFORM_WIN32s;
#endif
}

bool App::isNewShell() const
{
#if defined(WIN64)
	return true;
#else
	return osver_.v.vb.ver.u.cMajor > (BYTE)3;
#endif
}

// Windows 95 4.0.180 and NT4.0
bool App::isNewOpenSaveDlg() const
{
#if defined(WIN64)
	return true;
#else
	return app().is9xOSVerLarger( MKVER(4,0,180) )
	    || app().isNTOSVerLarger( MKVER(4,0,0) ) ;
#endif
}
//=========================================================================

extern int kmain();

namespace ki
{
	void APIENTRY Startup()
	{
		// Startup :
		// プログラム開始すると、真っ先にここに来ます。

		// C++のローカルオブジェクトの破棄順序の仕様に
		// 自信がないので(^^;、スコープを利用して順番を強制
		// たぶん宣言の逆順だとは思うんだけど…

		LOGGER( "StartUp" );
		App myApp;
		{
			LOGGER( "StartUp app ok" );
			#ifdef USE_THREADS
			ThreadManager myThr;
			{
				LOGGER( "StartUp thr ok" );
			#endif
				MemoryManager myMem;
				{
					LOGGER( "StartUp mem ok" );
					IMEManager myIME;
					{
						LOGGER( "StartUp ime ok" );
						String::LibInit();
						{
							LOGGER( "StartUp strings ok" );
							const int r = kmain();
							myApp.SetExitCode( r );
						}
					}
				}
			#ifdef USE_THREADS
			} // myThr, ~ThreadManager
			#endif
		}
	}
}

#ifdef __GNUC__
  #ifdef SUPERTINY
	extern "C" void WINAPI entryp() { ki::Startup(); }
  #endif
	extern "C" void __cdecl __cxa_pure_virtual(void) {};
	extern "C" void __deregister_frame_info() {};
	extern "C" void __register_frame_info() {};
	extern int __stack_chk_guard = 696115047 ;

	extern "C" int __stack_chk_fail(){ MessageBoxA(NULL, "__stack_chk_fail", NULL, MB_OK|MB_TOPMOST) ; ExitProcess(1); };
#endif

#ifdef SUPERTINY

	extern "C" int __cdecl _purecall(){return 0;}
	#ifdef _DEBUG
		int main(){return 0;}
	#endif
	#pragma comment(linker, "/entry:\"Startup\"")

#else

	// VS2005でビルドしてもWin95で動くようにするため
	#if _MSC_VER >= 1400
		extern "C" BOOL WINAPI _imp__IsDebuggerPresent() { return FALSE; }
	#endif

	int APIENTRY WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
	{
		ki::Startup();
		return 0;
	}

#endif
