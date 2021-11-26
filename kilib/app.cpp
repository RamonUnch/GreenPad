#include "stdafx.h"
#include "app.h"
#include "log.h"
#include "memory.h"
#include "thread.h"
#include "window.h"
#include "string.h"
using namespace ki;


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
typedef DWORD (WINAPI * CoCreateInstance_funk)(REFCLSID , LPUNKNOWN , DWORD , REFIID , LPVOID *);
DWORD MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	static CoCreateInstance_funk func = (CoCreateInstance_funk)(-1);
	if (func == (CoCreateInstance_funk)(-1)) // First time!
		func = (CoCreateInstance_funk)GetProcAddress(GetModuleHandleA("OLE32.DLL"), "CoCreateInstance");

	if (func) { // We got the function!
		return func(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}
	return 666; // Fail with 666 error
}

//=========================================================================

App* App::pUniqueInstance_;

inline App::App()
	: exitcode_    (-1)
	, loadedModule_(0)
	, hInst_       (::GetModuleHandle(NULL))
	, hOle32_      ((HINSTANCE)(-1))
{
	// 唯一のインスタンスは私です。
	pUniqueInstance_ = this;
}

#pragma warning( disable : 4722 ) // 警告：デストラクタに値が戻りません
App::~App()
{
	// ロード済みモジュールがあれば閉じておく
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
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
	if (hOle32_ == (HINSTANCE)(-1) && what&(OLE|COM)) 
	{
		hOle32_ = ::LoadLibraryA("OLE32.DLL");
		// MessageBoxA(NULL, "Loading OLE32.DLL", hOle32_?"Sucess": "Failed", MB_OK);
	}

	// 初期化済みでなければ初期化する
	bool ret = true;
	if( !(loadedModule_ & what) )
		switch( what )
		{
		case CTL:
			::InitCommonControls(); break;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
		case COM: 
			// Actually we only ever use OLE, that calls COM, so it can
			// be Ignored safely...
			//ret = S_OK == ::MyCoInitialize( NULL );
			//MessageBoxA(NULL, "CoInitialize", ret?"Sucess": "Failed", MB_OK);
			//break;
		case OLE: 
			ret = S_OK == ::MyOleInitialize( NULL );
			// MessageBoxA(NULL, "OleInitialize", ret?"Sucess": "Failed", MB_OK);
			break;
#endif
		}

	// 今回初期化したモノを記憶
	if (ret) loadedModule_ |= what;
}

void App::Exit( int code )
{
	// 終了コードを設定して
	SetExitCode( code );

	// 自殺
	this->~App();
}



//-------------------------------------------------------------------------

const OSVERSIONINFO& App::osver()
{
	static OSVERSIONINFO s_osVer;
	if( s_osVer.dwOSVersionInfoSize == 0 )
	{
		// 初回だけは情報取得
		s_osVer.dwOSVersionInfoSize = sizeof( s_osVer );
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>310)
		::GetVersionEx( &s_osVer );
#else
		DWORD dwVersion = ::GetVersion();

		s_osVer.dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
		s_osVer.dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
		if (dwVersion < 0x80000000)              
				s_osVer.dwBuildNumber = (DWORD)(HIWORD(dwVersion));

		if(s_osVer.dwMajorVersion == 3) s_osVer.dwPlatformId=VER_PLATFORM_WIN32_NT;
		else if(s_osVer.dwMajorVersion == 4)
		{
			if(s_osVer.dwMinorVersion == 0)
			{
				if(s_osVer.dwBuildNumber <= 950 || s_osVer.dwBuildNumber == 1111 || s_osVer.dwBuildNumber == 1214)
					s_osVer.dwPlatformId=VER_PLATFORM_WIN32_WINDOWS;
				else s_osVer.dwPlatformId=VER_PLATFORM_WIN32_NT;
			}
			else
			{
				s_osVer.dwPlatformId=VER_PLATFORM_WIN32_WINDOWS;
			}
		}
		else s_osVer.dwPlatformId=VER_PLATFORM_WIN32_NT;
#endif
	}
	return s_osVer;
}

bool App::isNewTypeWindows()
{
	static const OSVERSIONINFO& v = osver();
	return (
		( v.dwPlatformId==VER_PLATFORM_WIN32_NT && v.dwMajorVersion>=5 )
	 || ( v.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS &&
	          v.dwMajorVersion*100+v.dwMinorVersion>=410 )
	);
}

bool App::isWin95()
{
	static const OSVERSIONINFO& v = osver();
	return (
		v.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS &&
		v.dwMajorVersion==4 &&
		v.dwMinorVersion==0
	);
}

bool App::isNT()
{
	static const OSVERSIONINFO& v = osver();
	return v.dwPlatformId==VER_PLATFORM_WIN32_NT;
}

bool App::isNewShell()
{
	static const OSVERSIONINFO& v = osver();
	return v.dwMajorVersion>3;
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
			ThreadManager myThr;
			{
				LOGGER( "StartUp thr ok" );
				MemoryManager myMem;
				{
					LOGGER( "StartUp mem ok" );
					IMEManager myIME;
					{
						LOGGER( "StartUp ime ok" );
						String::LibInit();
						{
							const int r = kmain();
							myApp.SetExitCode( r );
						}
					}
				}
			}
		}
	}
}

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
