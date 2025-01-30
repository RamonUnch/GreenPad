#include "stdafx.h"
#include "app.h"
#include "ctrl.h"
using namespace ki;



//=========================================================================

StatusBar::StatusBar()
	: width_   (0)
	, visible_ (false)
	, parent_  (NULL)
{
	//app().InitModule( App::CTL );
}

bool StatusBar::Create( )
{
	HWND h = NULL;
	WNDCLASS wc;
	app().InitModule( App::CTL );
	// Avoid using CreateStatusWindow that is not present on NT3.1.
	h = ::CreateWindowEx(
		0, // ExStyle
		GetClassInfo(NULL, STATUSCLASSNAME, &wc)?
		STATUSCLASSNAME: // TEXT("msctls_statusbar32")...
		TEXT("msctls_statusbar"), // TEXT("msctls_statusbar") for NT3.1 and 3.5 build <711

		NULL, // pointer to window name
		WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP , // window style
		0, 0, 0, 0, //x, y, w, h
		parent_, // handle to parent or owner window
		reinterpret_cast<HMENU>(1787), // handle to menu or child-window identifier
		app().hinst(), // handle to application instance
		NULL // pointer to window-creation data
	);

	if( h == NULL )
	{
		LOGGERF(TEXT("StatusBar::Create() failed"));
		return false;
	}

	SetHwnd( h );
	visible_ = true;
	AutoResize( false );
	return true;
}

void StatusBar::SetStatusBarVisible(bool b)
{
	if (b && !hwnd() && parent_ )
		Create(); // Create if needed
	if( hwnd() )
		::ShowWindow( hwnd(), b? SW_SHOW: SW_HIDE );
	visible_= b && hwnd();
}

int StatusBar::AutoResize( bool maximized )
{
	// サイズ自動変更
	SendMsg( WM_SIZE );

	// 変更後のサイズを取得
	RECT rc;
	getPos( &rc );
	width_ = rc.right - rc.left;
	return (isStatusBarVisible() ? rc.bottom - rc.top : 0);
}

bool StatusBar::PreTranslateMessage( MSG* )
{
	// 何もしない
	return false;
}

void StatusBar::SetText( const TCHAR* str, int part )
{
#if defined(UNICODE) && defined(TARGET_VER) && TARGET_VER <= 350
	if ( app().isNTOSVerLarger(MKVER(3,50,711)) )
	{	// Unicode in UNICOWS mode to be used on NT only from 3.5 build 711
		SendMsg( SB_SETTEXTW, part, reinterpret_cast<LPARAM>(str) );
	}
	else
	{	// Use ANSI version NT3.1 and Win9x (convert string).
		char buf[256];
		long len = ::WideCharToMultiByte(CP_ACP, 0, str, -1 , buf, countof(buf), NULL, NULL);
		buf[len] = '\0';
		SendMsg( SB_SETTEXTA, part, reinterpret_cast<LPARAM>(buf) );
	}
#else
	SendMsg( SB_SETTEXT, part, reinterpret_cast<LPARAM>(str) );
#endif
}

int StatusBar::GetTextLen( int part )
{
#if defined(UNICODE) && defined(TARGET_VER) && TARGET_VER <= 350
	if ( app().isNTOSVerLarger(MKVER(3,50,711)) )
		return SendMsg( SB_GETTEXTLENGTHW, part, 0 );
	else
		return SendMsg( SB_GETTEXTLENGTHA, part, 0 );
#else
	return SendMsg( SB_GETTEXTLENGTH, part, 0 );
#endif
}
int StatusBar::GetText( TCHAR* str, int part )
{
	int len = GetTextLen(part);
	if( len >= 255 )
	{
		str[0] = TEXT('\0');
		return 0;
	}
#if defined(UNICODE) && defined(TARGET_VER) && TARGET_VER <= 350
	if ( app().isNTOSVerLarger(MKVER(3,50,711)) )
	{	// Unicode in UNICOWS mode to be used on NT only from 3.5 build 711
		SendMsg( SB_GETTEXTW, part, reinterpret_cast<LPARAM>(str) );
	}
	else
	{	// Use ANSI version NT3.1 and Win9x (convert string).
		char buf[256];
		SendMsg( SB_GETTEXTA, part, reinterpret_cast<LPARAM>(buf) );
		len = ::MultiByteToWideChar( CP_ACP, 0, buf, len+1, str, 255 );
		str[len] = L'\0';
	}
#else
	SendMsg( SB_GETTEXT, part, reinterpret_cast<LPARAM>(str) );
#endif
	return len;
}



//=========================================================================

void ComboBox::Select( const TCHAR* str )
{
	// SELECTSTRING は先頭が合ってる物に全てにマッチするので使えない。
	// おそらくインクリメンタルサーチとかに使うべきものなのだろう。
	LRESULT i = // Use CB_FINDSTRING on Windows NT below 3.10.404
	#if defined(TARGET_VER) && TARGET_VER <= 303
		SendMsg( app().isNT() && app().getOOSVer() < MKVER(3,10,404)? CB_FINDSTRING :CB_FINDSTRINGEXACT, ~0, reinterpret_cast<LPARAM>(str) );
	#else
		SendMsg( CB_FINDSTRINGEXACT, ~0, reinterpret_cast<LPARAM>(str) );
	#endif
	if( i != CB_ERR )
		SendMsg( CB_SETCURSEL, i );
//	else
//		SendMsg( WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str) );
}

bool ComboBox::PreTranslateMessage( MSG* )
{
	return false;
}
