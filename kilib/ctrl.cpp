#include "stdafx.h"
#include "app.h"
#include "ctrl.h"
using namespace ki;



//=========================================================================

StatusBar::StatusBar()
{
	app().InitModule( App::CTL );
}

bool StatusBar::Create( HWND parent )
{
	HWND h = NULL;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	// Avoid using CreateStatusWindow that is not present on NT3.1.
	h = CreateWindowEx(
		0, // ExStyle
		app().isNT31()? TEXT("msctls_statusbar"): STATUSCLASSNAME,  // TEXT("msctls_statusbar") for NT3.1
		NULL, // pointer to window name
		WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP , // window style
		0, 0, 0, 0, //x, y, w, h
		parent, // handle to parent or owner window
		(struct HMENU__ *)1787, // handle to menu or child-window identifier
		app().hinst(), // handle to application instance
		NULL // pointer to window-creation data
	);
#else 
	h = NULL;
#endif
	if( h == NULL )
		return false;

	SetStatusBarVisible();
	SetHwnd( h );
	AutoResize( false );
	return true;
}

int StatusBar::AutoResize( bool maximized )
{
	// �T�C�Y�����ύX
	SendMsg( WM_SIZE );

	// �ύX��̃T�C�Y���擾
	RECT rc;
	getPos( &rc );
	width_ = rc.right - rc.left;
	if( !maximized )
		width_ -= 15;
	return (isStatusBarVisible() ? rc.bottom - rc.top : 0);
}

bool StatusBar::PreTranslateMessage( MSG* )
{
	// �������Ȃ�
	return false;
}

void StatusBar::SetText( const TCHAR* str, int part )
{
  # ifdef UNICOWS
	if (app().isNT() && !app().isNT31())
  # endif
	{	// Unicode in UNICOWS mode to be used on NT only from 3.5
		SendMsg( SB_SETTEXT, part, reinterpret_cast<LPARAM>(str) ); 
	}
  # ifdef UNICOWS
	else 
	{	// Use ANSI version NT3.1 and Win9x (convert string).
		char buf[256];
		long len = ::WideCharToMultiByte(CP_ACP, 0, str, -1 , buf, 255, NULL, NULL);
		buf[len] = '\0';
		SendMsg( SB_SETTEXTA, part, reinterpret_cast<LPARAM>(buf) ); 
	}
  # endif
}


//=========================================================================

void ComboBox::Select( const TCHAR* str )
{
	// SELECTSTRING �͐擪�������Ă镨�ɑS�ĂɃ}�b�`����̂Ŏg���Ȃ��B
	// �����炭�C���N�������^���T�[�`�Ƃ��Ɏg���ׂ����̂Ȃ̂��낤�B
	size_t i =
		SendMsg( CB_FINDSTRINGEXACT, ~0, reinterpret_cast<LPARAM>(str) );
	if( i != CB_ERR )
		SendMsg( CB_SETCURSEL, i );
}

bool ComboBox::PreTranslateMessage( MSG* )
{
	return false;
}
