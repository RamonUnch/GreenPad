#include "stdafx.h"
#include "app.h"
#include "log.h"
#include "memory.h"
#include "thread.h"
#include "window.h"
#include "string.h"
using namespace ki;

#ifndef NO_OLE32
typedef DWORD (WINAPI * Initialize_funk)(LPVOID r);
static DWORD MyOleInitialize(LPVOID r)
{
	static Initialize_funk func = (Initialize_funk)(-1);
	if (func == (Initialize_funk)(-1)) // First time!
		func = (Initialize_funk)GetProcAddress(GetModuleHandleA("OLE32.DLL"), "OleInitialize");

	if (func) { // We got the function!
		return func(r);
	}
	return 666; // Fail with 666 error.
}

typedef void (WINAPI * UnInitialize_funk)( );
static void MyOleUninitialize( )
{
	static UnInitialize_funk func = (UnInitialize_funk)(-1);
	if (func == (UnInitialize_funk)(-1)) // First time!
		func = (UnInitialize_funk)GetProcAddress(GetModuleHandleA("OLE32.DLL"), "OleUninitialize");

	if (func) { // We got the function!
		func();
	}
}
typedef HRESULT (WINAPI * CoCreateInstance_funk)(REFCLSID , LPUNKNOWN , DWORD , REFIID , LPVOID *);
HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	static CoCreateInstance_funk func = (CoCreateInstance_funk)(-1);
	if (func == (CoCreateInstance_funk)(-1)) // First time!
		func = (CoCreateInstance_funk)GetProcAddress(GetModuleHandleA("OLE32.DLL"), "CoCreateInstance");

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
#endif // NO_OLE32

#if defined(TARGET_VER) && TARGET_VER <= 350
typedef BOOL (WINAPI *GetVersionEx_funk)(LPOSVERSIONINFOA s_osVer);
static BOOL MyGetVersionEx(LPOSVERSIONINFOA s_osVer)
{
	// Try first to get the real GetVersionEx function
	// We use the ANSI version because it does not matter.
	GetVersionEx_funk func = (GetVersionEx_funk)
		GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "GetVersionExA");
	if (func && func( s_osVer ))
	{
		if (s_osVer->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{   // fixup broken build number in early 9x builds (roytam1)
			s_osVer->dwBuildNumber &= 0xffff;
		}
		return TRUE;
	}

	// Fallback in case the above failed (WinNT 3.1 / Win32s)
	DWORD dwVersion = ::GetVersion();
//	TCHAR buf[12];
//	::wsprintf(buf, "%x", dwVersion);
//	MessageBox(NULL, buf, TEXT("Windows Version"), 0);

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
			s_osVer->dwPlatformId = VER_PLATFORM_WIN32s;
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
	: hOle32_      ((HINSTANCE)(-1))
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
//	if( loadedModule_ & COM )
//		::MyCoUninitialize();
	if( loadedModule_ & OLE )
		::MyOleUninitialize();
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
		hOle32_ = ::LoadLibrary(TEXT("OLE32.DLL"));
#endif
	// 初期化済みでなければ初期化する
	bool ret = true;
	if( !(loadedModule_ & what) )
		switch( what )
		{
		case CTL: {
			// ::InitCommonControls();
			if( !hInstComCtl_ )
				hInstComCtl_ = ::LoadLibrary(TEXT("COMCTL32.DLL"));
			if( hInstComCtl_ )
			{
				void (WINAPI *dyn_InitCommonControls)(void) = ( void (WINAPI *)(void) )
					GetProcAddress( hInstComCtl_, "InitCommonControls" );
				if (dyn_InitCommonControls)
					dyn_InitCommonControls();
			}
			} break;
		case COM:
			// Actually we only ever use OLE, that calls COM, so it can
			// be Ignored safely...
			//ret = S_OK == ::MyCoInitialize( NULL );
			//MessageBoxA(NULL, "CoInitialize", ret?"Sucess": "Failed", MB_OK);
			//break;
		case OLE:
			#ifndef NO_OLE32
			ret = S_OK == ::MyOleInitialize( NULL );
			#endif
			// MessageBoxA(NULL, "OleInitialize", ret?"Sucess": "Failed", MB_OK);
			break;
		default: break;
		}

	// 今回初期化したモノを記憶
	if (ret) loadedModule_ |= what;
}

void App::Exit( int code )
{
	// 終了コードを設定して
	SetExitCode( code );

	// only free library when program quits
	if( hInstComCtl_ ) ::FreeLibrary( hInstComCtl_ );

	// 自殺
	this->~App();
}



//-------------------------------------------------------------------------

const OSVERSIONINFOA& App::osver()
{
	static OSVERSIONINFOA s_osVer;
	if( s_osVer.dwOSVersionInfoSize == 0 )
	{
		// 初回だけは情報取得
		s_osVer.dwOSVersionInfoSize = sizeof( s_osVer );
		MyGetVersionEx( &s_osVer );
	}
	return s_osVer;
}

DWORD App::getOSVer()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwMajorVersion*100+v.dwMinorVersion;
}

DWORD App::getOSBuild()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwBuildNumber;
}

bool App::isOSVerEqual(DWORD ver, DWORD build)
{
	return getOSVer() == ver && getOSBuild() == build;
}

bool App::isNTOSVerEqual(DWORD ver, DWORD build)
{
	return isNT() && getOSVer() == ver && getOSBuild() == build;
}
bool App::is9xOSVerEqual(DWORD ver, DWORD build)
{
	return !isNT() && getOSVer() == ver && getOSBuild() == build;
}

bool App::isOSVerLarger(DWORD ver, DWORD build)
{
	return ( getOSVer() > ver || ( getOSVer() == ver && getOSBuild() >= build ) );
}

bool App::isNTOSVerLarger(DWORD ver, DWORD build)
{
	return isNT() && ( getOSVer() > ver || ( getOSVer() == ver && getOSBuild() >= build ) );
}

bool App::is9xOSVerLarger(DWORD ver, DWORD build)
{
	return !isNT() && ( getOSVer() > ver || ( getOSVer() == ver && getOSBuild() >= build ) );
}

bool App::isNewTypeWindows()
{
	static const OSVERSIONINFOA& v = osver();
	return (
		( v.dwPlatformId==VER_PLATFORM_WIN32_NT && v.dwMajorVersion>=5 )
	 || ( v.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS &&
	          v.dwMajorVersion*100+v.dwMinorVersion>=410 )
	);
}

bool App::isWin95()
{
	static const OSVERSIONINFOA& v = osver();
	return (
		v.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS &&
		v.dwMajorVersion==4 &&
		v.dwMinorVersion==0
	);
}

bool App::isNT()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwPlatformId==VER_PLATFORM_WIN32_NT;
}

bool App::isWin32s()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwPlatformId==VER_PLATFORM_WIN32s;
}

bool App::isNewShell()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwMajorVersion>3;
}

bool App::is351p()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwMajorVersion>3
		|| (v.dwMajorVersion==3 && v.dwMinorVersion >= 51);
}

bool App::isNT31()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwMajorVersion==3 && v.dwMinorVersion < 50;
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
