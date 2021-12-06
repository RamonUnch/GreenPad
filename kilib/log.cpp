#include "stdafx.h"
#include "log.h"
#include "app.h"
#include "string.h"
using namespace ki;



//=========================================================================

void Logger::WriteLine( const String& str )
{
	WriteLine( str.c_str(), str.len()*sizeof(TCHAR) );
}

void Logger::WriteLine( const TCHAR* str )
{
	WriteLine( str, my_lstrlen(str)*sizeof(TCHAR) );
}

void Logger::WriteLine( const TCHAR* str, int siz )
{
#ifdef DO_LOGGING
	// Fileクラス自体のデバッグに使うかもしれないので、
	// Fileクラスを使用することは出来ない。API直叩き
	static bool st_firsttime = true;
	DWORD dummy;

	// ファイル名
	TCHAR fname[MAX_PATH];
	::GetModuleFileName( ::GetModuleHandle(NULL), fname, countof(fname) );
	::lstrcat( fname, TEXT("_log") );

	// ファイルを書き込み専用で開く
	HANDLE h = ::CreateFile( fname,
		GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( h == INVALID_HANDLE_VALUE )
		return;

	// 初回限定処理
	if( st_firsttime )
	{
		::SetEndOfFile( h );
		#ifdef _UNICODE
			::WriteFile( h, "\xff\xfe", 2, &dummy, NULL );
		#endif
		st_firsttime = false;
	}
	else
	{
		::SetFilePointer( h, 0, NULL, FILE_END );
	}

	// 書く
	::WriteFile( h, str, siz, &dummy, NULL );
	::WriteFile( h, TEXT("\r\n"), sizeof(TEXT("\r")), &dummy, NULL );

	// 閉じる
	::CloseHandle( h );

#endif
}
