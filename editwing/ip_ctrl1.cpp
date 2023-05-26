#include "../kilib/stdafx.h"
#include "ewCtrl1.h"
using namespace ki;
using namespace editwing;



//-------------------------------------------------------------------------
// EwEditコントロール作成/破棄
//-------------------------------------------------------------------------

EwEdit::ClsName EwEdit::className_ = TEXT("EditWing Control-01");

EwEdit::EwEdit()
	: WndImpl( className_, WS_CHILD|WS_VISIBLE, WS_EX_CLIENTEDGE )
{
	static bool ClassRegistered = false;
	if( !ClassRegistered )
	{
		ClassRegistered = true;

		// 初回構築時のみ、クラス登録を行う
		WNDCLASS wc      = {0};
		wc.lpszClassName = className_;
		WndImpl::Register( &wc );
	}
}

EwEdit::~EwEdit()
{
	Destroy();
}

void EwEdit::on_create( CREATESTRUCT* cs )
{
	doc_  = new doc::Document;
	view_ = new view::View( *doc_, hwnd() );
}

void EwEdit::on_destroy()
{
	view_ = NULL;
	doc_  = NULL;
}



//-------------------------------------------------------------------------
// 簡単なメッセージ制御
//-------------------------------------------------------------------------

LRESULT EwEdit::on_message( UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{
	case WM_SETFOCUS:
		{
			view_->SetFocus();
			break;
		}
	case WM_SIZE:
		{
			RECT rc;
			getClientRect( &rc );
			view_->MoveTo( 0, 0, rc.right, rc.bottom );
			break;
		}
	case EM_CANUNDO:     return getDoc().isUndoAble();
	case EM_SETREADONLY: getCursor().SetROMode(wp!=FALSE); return TRUE;
	case WM_COPY:  getCursor().Copy();  return 0;
	case WM_CUT:   getCursor().Cut();   return 0;
	case WM_PASTE: getCursor().Paste(); return 0;
	case EM_UNDO:
	case WM_UNDO:  getDoc().Undo();     return TRUE;
	default:
		return WndImpl::on_message( msg, wp, lp );
	}
	return 0;
/*
EM_CHARFROMPOS 
EM_EMPTYUNDOBUFFER:
EM_FMTLINES 
EM_GETCUEBANNER 
EM_GETFIRSTVISIBLELINE 
EM_GETHANDLE 
EM_GETIMESTATUS 
EM_GETLIMITTEXT 
EM_GETLINE 
EM_GETLINECOUNT 
EM_GETMARGINS 
EM_GETMODIFY 
EM_GETPASSWORDCHAR 
EM_GETRECT 
EM_GETSEL 
EM_GETTHUMB 
EM_GETWORDBREAKPROC 
EM_HIDEBALLOONTIP 
EM_LIMITTEXT 
EM_LINEFROMCHAR 
EM_LINEINDEX 
EM_LINELENGTH 
EM_LINESCROLL 
EM_POSFROMCHAR 
EM_REPLACESEL 
EM_SCROLL 
EM_SCROLLCARET 
EM_SETCUEBANNER 
EM_SETHANDLE 
EM_SETIMESTATUS 
EM_SETLIMITTEXT 
EM_SETMARGINS 
EM_SETMODIFY 
EM_SETPASSWORDCHAR 
EM_SETRECT 
EM_SETRECTNP 
EM_SETSEL 
EM_SETTABSTOPS 
EM_SETWORDBREAKPROC 
EM_SHOWBALLOONTIP 
WM_COMMAND 
WM_CTLCOLOREDIT 
WM_CTLCOLORSTATIC 
*/
}
