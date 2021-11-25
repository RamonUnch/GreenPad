#include "stdafx.h"
#include "app.h"
#include "log.h"
#include "memory.h"
#include "thread.h"
#include "window.h"
#include "string.h"
using namespace ki;



//=========================================================================

App* App::pUniqueInstance_;

inline App::App()
	: exitcode_    (-1)
	, loadedModule_(0)
	, hInst_       (::GetModuleHandle(NULL))
{
	// �B��̃C���X�^���X�͎��ł��B
	pUniqueInstance_ = this;
}

#pragma warning( disable : 4722 ) // �x���F�f�X�g���N�^�ɒl���߂�܂���
App::~App()
{
	// ���[�h�ς݃��W���[��������Ε��Ă���
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>310)
	if( loadedModule_ & COM )
		::CoUninitialize();
	if( loadedModule_ & OLE )
		::OleUninitialize();
#endif

	// �I�`���`
	::ExitProcess( exitcode_ );
}

inline void App::SetExitCode( int code )
{
	// �I���R�[�h��ݒ�
	exitcode_ = code;
}

void App::InitModule( imflag what )
{
	// �������ς݂łȂ���Ώ���������
	if( !(loadedModule_ & what) )
		switch( what )
		{
		case CTL: ::InitCommonControls(); break;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>310)
		case COM: ::CoInitialize( NULL ); break;
		case OLE: ::OleInitialize( NULL );break;
#endif
		}

	// ���񏉊����������m���L��
	loadedModule_ |= what;
}

void App::Exit( int code )
{
	// �I���R�[�h��ݒ肵��
	SetExitCode( code );

	// ���E
	this->~App();
}



//-------------------------------------------------------------------------

const OSVERSIONINFO& App::osver()
{
	static OSVERSIONINFO s_osVer;
	if( s_osVer.dwOSVersionInfoSize == 0 )
	{
		// ���񂾂��͏��擾
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
		// �v���O�����J�n����ƁA�^����ɂ����ɗ��܂��B

		// C++�̃��[�J���I�u�W�F�N�g�̔j�������̎d�l��
		// ���M���Ȃ��̂�(^^;�A�X�R�[�v�𗘗p���ď��Ԃ�����
		// ���Ԃ�錾�̋t�����Ƃ͎v���񂾂��ǁc

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

	// VS2005�Ńr���h���Ă�Win95�œ����悤�ɂ��邽��
	#if _MSC_VER >= 1400
		extern "C" BOOL WINAPI _imp__IsDebuggerPresent() { return FALSE; }
	#endif

	int APIENTRY WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
	{
		ki::Startup();
		return 0;
	}

#endif
