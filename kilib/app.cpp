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

#if defined(TARGET_VER)
typedef BOOL (WINAPI *GetVersionEx_funk)(LPOSVERSIONINFOA s_osVer);
static BOOL MyGetVersionEx(LPOSVERSIONINFOA s_osVer)
{
	// Try first to get the real GetVersionEx function
	// We use the ANSI version because it does not matter.
#if TARGET_VER > 300
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
#endif
	// Fallback in case the above failed (WinNT 3.1 / Win32s)
	DWORD dwVersion = ::GetVersion();

	s_osVer->dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	s_osVer->dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

	if (dwVersion < 0x80000000) 
	{ // WINNT => build number = HIWORD(dwversion)
		s_osVer->dwBuildNumber = (DWORD)(HIWORD(dwVersion));
		s_osVer->dwPlatformId = VER_PLATFORM_WIN32_NT;
	}

	if (s_osVer->dwPlatformId != VER_PLATFORM_WIN32_NT) 
	{
		if (s_osVer->dwMajorVersion == 3) s_osVer->dwPlatformId = VER_PLATFORM_WIN32s;
		else s_osVer->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
	}

	return TRUE;
}
#else
	#define MyGetVersionEx GetVersionExA
#endif

//=========================================================================

App* App::pUniqueInstance_;

inline App::App()
	: exitcode_    (-1)
	, loadedModule_(0)
	, hInst_       (::GetModuleHandle(NULL))
	, hOle32_      ((HINSTANCE)(-1))
{
	// ?B?????C???X?^???X?????????B
	pUniqueInstance_ = this;
}

#pragma warning( disable : 4722 ) // ?x???F?f?X?g???N?^???l????????????
App::~App()
{
	// ???[?h???????W???[????????????????????
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
//	if( loadedModule_ & COM )
//		::MyCoUninitialize();
	if( loadedModule_ & OLE )
		::MyOleUninitialize();
#endif

	// ?I?`???`
	::ExitProcess( exitcode_ );
}

inline void App::SetExitCode( int code )
{
	// ?I???R?[?h??????
	exitcode_ = code;
}

void App::InitModule( imflag what )
{
	if (hOle32_ == (HINSTANCE)(-1) && what&(OLE|COM|OLEDLL)) 
		hOle32_ = ::LoadLibraryA("OLE32.DLL");

	// ??????????????????????????????
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
		default: break;
		}

	// ?????????????????m???L??
	if (ret) loadedModule_ |= what;
}

void App::Exit( int code )
{
	// ?I???R?[?h??????????
	SetExitCode( code );

	// ???E
	this->~App();
}



//-------------------------------------------------------------------------

const OSVERSIONINFOA& App::osver()
{
	static OSVERSIONINFOA s_osVer;
	if( s_osVer.dwOSVersionInfoSize == 0 )
	{
		// ??????????????????
		s_osVer.dwOSVersionInfoSize = sizeof( s_osVer );
		MyGetVersionEx( &s_osVer );
	}
	return s_osVer;
}

int App::getOSVer()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwMajorVersion*100+v.dwMinorVersion;
}

int App::getOSBuild()
{
	static const OSVERSIONINFOA& v = osver();
	return v.dwBuildNumber;
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
		// ?v???O?????J?n???????A?^???????????????????B

		// C++?????[?J???I?u?W?F?N?g???j?????????d?l??
		// ???M??????????(^^;?A?X?R?[?v?????p??????????????
		// ?????????????t?????????v???????????c

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

	// VS2005???r???h??????Win95????????????????????
	#if _MSC_VER >= 1400
		extern "C" BOOL WINAPI _imp__IsDebuggerPresent() { return FALSE; }
	#endif

	int APIENTRY WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
	{
		ki::Startup();
		return 0;
	}

#endif
