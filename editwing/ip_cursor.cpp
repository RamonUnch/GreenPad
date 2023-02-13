#include "stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;
using doc::Insert;
using doc::Delete;
using doc::Replace;

static ulong countLines( const unicode *str, size_t len )
{
	ulong nl = 0;
	while( len-- )
	{
		if( *str == L'\r' )
		{
			nl++; // CR or CRLF
			str += str[1] == '\n'; // CRLF
		}
		else if( *str == L'\n' )
		{
			nl++; // LF
		}
		str++;
	}
	return nl;
}
static ulong lastLineLength( const unicode *str, size_t len )
{
	ulong lll = 0;
	while( len-- )
	{
		if( str[len] == L'\r' || str[len] == L'\n' )
			break;
		lll++;
	}
	return lll;
}

#if defined(_MBCS)
static BOOL WINAPI myIsDBCSLeadByteEx_1st(UINT cp, BYTE ch);
static BOOL WINAPI myIsDBCSLeadByteEx_fb(UINT cp, BYTE ch);
static BOOL (WINAPI *myIsDBCSLeadByteEx)(UINT cp, BYTE ch) = myIsDBCSLeadByteEx_1st;

static BOOL WINAPI myIsDBCSLeadByteEx_1st(UINT cp, BYTE ch)
{
	myIsDBCSLeadByteEx = myIsDBCSLeadByteEx_fb;

	if( app().isNT() || app().is9xOSVerLarger( MKVER(4,00,950)) )
	{
		// On Chicago build 116 We get a crash when using IsDBCSLeadByteEx
		// TODO: find exactly which build is required...
		FARPROC funk = GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "IsDBCSLeadByteEx");
		if( funk )
			myIsDBCSLeadByteEx = ( BOOL (WINAPI *)(UINT cp, BYTE ch) )funk;
	}

	return myIsDBCSLeadByteEx(cp, ch);
}
static BOOL WINAPI myIsDBCSLeadByteEx_fb(UINT cp, BYTE ch)
{
	return IsDBCSLeadByte(ch);
}
#endif // defined(_MBCS)

#if !defined(UNICOWS)
static int myGetLocaleInfo(LCID Locale, LCTYPE LCType, LPTSTR lpLCData, int cchData)
{
	#define FUNK_TYPE ( int (WINAPI *)(LCID Locale, LCTYPE LCType, LPTSTR lpLCData, int cchData) )
	static int (WINAPI *funk)(LCID Locale, LCTYPE LCType, LPTSTR lpLCData, int cchData) = FUNK_TYPE 1;

	if (funk == FUNK_TYPE 1)
	{
	#ifdef UNICODE
		funk = FUNK_TYPE GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetLocaleInfoW");
	#else
		funk = FUNK_TYPE GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetLocaleInfoA");
	#endif
	}

	if (funk)
		return funk( Locale, LCType, lpLCData, cchData );

	/* Fallback */
	return 0;
	#undef FUNK_TYPE

}
#undef GetLocaleInfo
#define GetLocaleInfo myGetLocaleInfo
#endif // !defined(UNICOWS)
HKL MyGetKeyboardLayout(DWORD dwLayout);
static UINT GetInputCP()
{
	UINT kb_cp = CP_ACP;
#if defined(UNICOWS) || !defined(UNICODE) || defined(_MBCS)
	LCID lcid = LOWORD(MyGetKeyboardLayout( 0 ));
	TCHAR cpstr[16]; cpstr[0] = TEXT('\0');
	if( lcid && ::GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, cpstr, countof(cpstr)))
	{	// This should be the codepage of the local associated with
		// the current keyboard layout
		UINT tcp = String::GetInt( cpstr );
		if( tcp && ::IsValidCodePage( tcp ) )
			kb_cp = tcp;
	}
#endif
	return kb_cp;
}


//=========================================================================
//---- ip_cursor.cpp カーソルコントロール
//
//		カレットを表示したりIMEに適当に対応したり色々。
//		ところで疑問なのだが Caret って「カレット」と
//		読むのか「キャレット」と読むのか？
//
//---- ip_text.cpp   文字列操作・他, string manipulation, etc.
//---- ip_parse.cpp  キーワード解析, keyword parsing
//---- ip_wrap.cpp   折り返し
//---- ip_scroll.cpp スクロール
//---- ip_draw.cpp   描画・他
//=========================================================================


//-------------------------------------------------------------------------
// Caret制御用ラッパー
//-------------------------------------------------------------------------

class editwing::view::Caret : public Object
{
public:

	Caret( HWND wnd )
		: hwnd_( wnd ), created_( false ) {}

	~Caret()
		{ Destroy(); }

	void Show()
		{ if( created_ ) ::ShowCaret( hwnd_ ); }

	void Hide()
		{ if( created_ ) ::HideCaret( hwnd_ ); }

	void Destroy()
		{ if( created_ ) ::DestroyCaret(), created_=false; }

	void SetPos( int x, int y )
		{ if( created_ ) ::SetCaretPos(x,y), ime().SetPos(hwnd_,x,y); }

	void Create( int H, int W, const LOGFONT& lf )
		{
			if( created_ )
				::DestroyCaret();
			created_ = true;
			::CreateCaret( hwnd_, NULL, W, H );
			ime().SetFont( hwnd_, lf );
			Show();
		}

	bool isAlive()
		{ return created_; }

	HWND hwnd()
		{ return hwnd_; }

private:

	const HWND hwnd_;
	bool    created_;
};



//-------------------------------------------------------------------------
// カーソル初期化, Cursor initialization
//-------------------------------------------------------------------------

Cursor::Cursor( HWND wnd, ViewImpl& vw, doc::DocImpl& dc )
	: view_   ( vw )
	, doc_    ( dc )
	, pEvHan_ ( &defaultHandler_ )
	, caret_  ( new Caret(wnd) )
#ifndef NO_OLEDNDTAR
	, dndtg_  ( wnd, vw )
#endif
	, bIns_   ( true )
	, bRO_    ( false )
	, lineSelectMode_( false )
	, timerID_( 0 )
{
	// てきとーに情報初期化
	::SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, &keyRepTime_, 0 );
	cur_.tl = cur_.ad = cur_.vl = cur_.rl = 0;
	cur_.vx = cur_.rx = 0; sel_ = cur_;
}

Cursor::~Cursor()
{
}

void Cursor::AddHandler( CurEvHandler* ev )
{
	pEvHan_ = ev;
}

void Cursor::DelHandler( CurEvHandler* ev )
{
	if( ev == pEvHan_ )
		pEvHan_ = &defaultHandler_;
}



//-------------------------------------------------------------------------
// ヘルパー関数群, helper function group
//-------------------------------------------------------------------------

void Cursor::UpdateCaretPos()
{
	// メンバ変数の値を元に、実際にCaretを動かす処理
	int x, y;
	view_.GetOrigin( &x, &y );
	x += cur_.vx;
	y += cur_.vl * view_.fnt().H();

	// 行番号ゾーンにCaretがあっても困るので左に追いやる
	if( 0<x && x<view_.left() )
		x = -view_.left();

	// セット
	caret_->SetPos( x, y );
	pEvHan_->on_move( cur_, sel_ );
}

void Cursor::Redraw( const VPos& s, const VPos& e )
{
	int x, y; // 原点
	view_.GetOrigin( &x, &y );

	POINT sp = {x+s.vx, y+(long)s.vl*view_.fnt().H()};
	POINT ep = {x+e.vx, y+(long)e.vl*view_.fnt().H()};
	if( s > e ) // Swap
		sp.x^=ep.x, ep.x^=sp.x, sp.x^=ep.x,
		sp.y^=ep.y, ep.y^=sp.y, sp.y^=ep.y;
	ep.x+=2;

	// 手抜き16bitチェック入り…
	const long LFT = view_.left();
	const long RHT = view_.right();
	const long TOP = 0;
	const long BTM = view_.bottom();

	if( sp.y == ep.y )
	{
		RECT rc = { Max(LFT,sp.x), sp.y, Min(RHT,ep.x), sp.y+view_.fnt().H() };
		::InvalidateRect( caret_->hwnd(), &rc, FALSE );
	}
	else
	{
		RECT rc = { Max(LFT,sp.x), Max(TOP,sp.y), RHT, Min(BTM, (long)(sp.y+view_.fnt().H())) };
		::InvalidateRect( caret_->hwnd(), &rc, FALSE );
		RECT re = { LFT, Max(TOP,ep.y), Min(RHT,ep.x), Min(BTM, (long)(ep.y+view_.fnt().H())) };
		::InvalidateRect( caret_->hwnd(), &re, FALSE );
		RECT rd = { LFT, Max(TOP,rc.bottom), RHT, Min((long)BTM,re.top) };
		::InvalidateRect( caret_->hwnd(), &rd, FALSE );
	}
}

bool Cursor::getCurPos( const VPos** start, const VPos** end ) const
{
	*start = *end = &cur_;
	if( cur_==sel_ )//|| !caret_->isAlive() )
		return false;
	if( cur_ < sel_ )
		*end = &sel_;
	else
		*start = &sel_;
	return true;
}

bool Cursor::getCurPosUnordered( const VPos** cur, const VPos** sel ) const
{
	*cur =  &cur_;
	*sel = &sel_;
	return cur != sel;
}



//-------------------------------------------------------------------------
// Viewからの指令を処理, Process commands from View
//-------------------------------------------------------------------------

void Cursor::on_setfocus()
{
	caret_->Create( view_.fnt().H(),
		(bIns_ ? 2 : view_.fnt().W()), view_.fnt().LogFont() );
	UpdateCaretPos();
}

void Cursor::on_killfocus()
{
	caret_->Destroy();
	Redraw( cur_, sel_ );
}

void Cursor::on_scroll_begin()
{
	caret_->Hide();
}

void Cursor::on_scroll_end()
{
	UpdateCaretPos();
	caret_->Show();
}

void Cursor::ResetPos()
{
	// 設定変更などに対応, Support for changing settings, etc.
	view_.ConvDPosToVPos( cur_, &cur_ );
	view_.ConvDPosToVPos( sel_, &sel_ );
	UpdateCaretPos();
	if( caret_->isAlive() )
		view_.ScrollTo( cur_ );
}

void Cursor::on_text_update
	( const DPos& s, const DPos& e, const DPos& e2, bool mCur )
{
	VPos* search_base  = NULL;

	if( mCur && s==cur_ && e==sel_ )
	{
		search_base = &cur_;
	}
	else if( mCur && s==sel_ && e==cur_ )
	{
		search_base = &sel_;
	}
	else
	{
		Redraw( cur_, sel_ );
		if( mCur && caret_->isAlive() )
		{
			if( cur_ <= s )
				search_base = &cur_;
		}
		else
		{
			if( s < cur_ )
			{
				if( cur_ <= e )
					cur_ = e2;
				else if( cur_.tl == e.tl )
					cur_.tl=e2.tl, cur_.ad=e2.ad+cur_.ad-e.ad;
				else
					cur_.tl=e2.tl-e.tl;
				view_.ConvDPosToVPos( cur_, &cur_ );
			}
			if( s < sel_ )
				sel_ = cur_;
		}
	}

	if( mCur )
	{
		view_.ConvDPosToVPos( e2, &cur_, search_base );
		sel_ = cur_;
		if( caret_->isAlive() )
			view_.ScrollTo( cur_ );
	}
	UpdateCaretPos();
}



//-------------------------------------------------------------------------
// キー入力への対応, Support for keystrokes
//-------------------------------------------------------------------------

void CurEvHandler::on_char( Cursor& cur, unicode wch )
{
	cur.InputChar( wch );
}

void CurEvHandler::on_ime( Cursor& cur, unicode* str, ulong len )
{
	cur.Input( str, len );
}

void CurEvHandler::on_key( Cursor& cur, int vk, bool sft, bool ctl )
{
	switch( vk )
	{
	case VK_HOME:	cur.Home( ctl, sft );	break;
	case VK_END:	cur.End( ctl, sft );	break;
	case VK_RIGHT:	cur.Right( ctl, sft );	break;
	case VK_LEFT:	cur.Left( ctl, sft );	break;
	case VK_UP:		cur.Up( ctl, sft );		break;
	case VK_DOWN:	cur.Down( ctl, sft );	break;
	case VK_PRIOR:	cur.PageUp( sft );		break;
	case VK_NEXT:	cur.PageDown( sft );	break;
	case VK_DELETE:	cur.Del( ctl );			break;
	case VK_BACK:	cur.DelBack( ctl );		break;
	case VK_INSERT: cur.SetInsMode(!cur.isInsMode()); break;
	case VK_TAB:    cur.Tabulation( sft );	break;
	}
}

void Cursor::on_char( TCHAR ch )
{
	if( !bRO_ && ch!=0x7f
	&& ((unsigned)ch>=0x20 || ch==TEXT('\r') || ch==TEXT('\t')) )
	{
		if( UNICODEBOOL && app().isNT() )
		{ // In unicode mode we have Wide Chars ON NT
			pEvHan_->on_char( *this, ch );
		}
		else
		{ // Use _MBCS
			unicode wc = ch;
			UINT kb_cp = GetInputCP();
#if defined(_MBCS)
			static char prevchar=0;
			unicode wcs[8];
			if( !prevchar && myIsDBCSLeadByteEx( kb_cp, ch ) )
			{
				prevchar = ch; // Store Lead byte
				return;        // And ignore it
			}
			else if( prevchar ) // DBSC
			{
				// We have a DBCS pair to convert to unicode
				char x[2] = { (prevchar) , (ch) };
				int len = ::MultiByteToWideChar( kb_cp, 0, (char*)&x, 2, wcs, 8 );
				pEvHan_->on_ime( *this, wcs, len );
				prevchar = 0; // Reset lead byte
				return;
			}
			else
#endif
			if( ch & 0x80 ) // 非ASCII文字にはトリビアルでない変換が必要
			{
				// Non-ASCII characters require non-trivial conversion.
				unicode wcs[8];
				int len = ::MultiByteToWideChar( kb_cp, /*MB_COMPOSITE*/ 0, (char*)&ch, 1, wcs, 8 );
				pEvHan_->on_ime( *this, wcs, len );
				return;
			}
			pEvHan_->on_char( *this, wc );
		}
	}
}

void Cursor::on_ime_composition( LPARAM lp )
{
#ifndef NO_IME
	view_.ScrollTo( cur_ );
	if( !bRO_ && (lp&GCS_RESULTSTR) )
	{
		unicode* str=NULL;
		ulong    len=0;
		ime().GetString( caret_->hwnd(), &str, &len );
		if( str )
		{
			pEvHan_->on_ime( *this, str, len );
			delete [] str;
		}
	}
#endif // NO_IME
}

void Cursor::on_keydown( int vk, LPARAM flag )
{
	bool sft = (::GetKeyState(VK_SHIFT)>>15)!=0;
	bool ctl = (::GetKeyState(VK_CONTROL)>>15)!=0;
	pEvHan_->on_key( *this, vk, sft, ctl );
}



//-------------------------------------------------------------------------
// モード切替, mode switching
//-------------------------------------------------------------------------

void Cursor::SetInsMode( bool bIns )
{
	bIns_ = bIns;
	on_setfocus();
}

void Cursor::SetROMode( bool bRO )
{
	bRO_ = bRO;
}



//-------------------------------------------------------------------------
// 文字入力・削除, Character input and deletion
//-------------------------------------------------------------------------

void Cursor::InputChar( unicode ch )
{
	// 「上書モード ＆ 選択状態でない ＆ 行末でない」なら右一文字選択
	// If you are in overwriting mode, not selected,
	// and not at the end of a line, select the right character.
	if( !bIns_ && cur_==sel_ && doc_.len(cur_.tl)!=cur_.ad )
		Right( false, true );

	// 入力, character input by the user
	switch(ch)
	{
	case L'\r': Return();     break;
	case L'\t': break; // Nothing to do
	default: Input( &ch, 1 );
	}
}

void Cursor::Input( const unicode* str, ulong len )
{
	if( cur_==sel_ )
		doc_.Execute( Insert( cur_, str, len ) );
	else
		doc_.Execute( Replace( cur_, sel_, str, len ) );
}

void Cursor::Input( const char* str, ulong len )
{
	unicode* ustr = new unicode[ len*4 ];
	if(!ustr) return;
	len = ::MultiByteToWideChar( CP_ACP, 0, str, len, ustr, len*4 );
	Input( ustr, len );
	delete [] ustr;
}
void Cursor::InputAt( const unicode *str, ulong len, int x, int y )
{
	VPos np;
	view_.GetVPos( x, y, &np );
	bool curbeforesel = cur_ < sel_;
	VPos rsel = Min(cur_, sel_);
	VPos rcur = Max(cur_, sel_);

	if( np >= rcur )
	{
		// Insert at new position AFTER selection.
		// Current selection is not affected.
		doc_.Execute( Insert( np, str, len ) );
	}
	else if( np <= rsel )
	{
		// Insert at new position BEFORE selection.
		doc_.Execute( Insert( np, str, len ) );

		// Current selection MUST be shifted down/left.
		// This code is a mess, but I do not know any better.
		if( rcur != rsel )
		{
			ulong difflines = countLines(str, len);
			if( rsel.tl == np.tl )
			{	// Insertion at the begining of the line with selection.
				ulong lll = lastLineLength( str, len );
				int xoffset = difflines==0? lll: lll - np.ad ;
				rsel.ad += xoffset; // We must shift the begining of selection along X
				if( rsel.tl == rcur.tl ) // for single line selection we must also
					rcur.ad += xoffset; // shift the end of selection along X
			}
			rcur.tl += difflines;
			rsel.tl += difflines;
			view_.ConvDPosToVPos(rcur, &rcur);
			view_.ConvDPosToVPos(rsel, &rsel);
		}
	}
	else // if( isInSelection( np ) )
	{
		// Replace the whole selection if inside.
		doc_.Execute( Replace( cur_, sel_, str, len ) );
		return; // Do not restore selection.
	}

	// Restore eventual selection.
	if( rcur != rsel )
	{
		// Only set cur_ and sel_ to the corrected values
		// Do not force moving back (in case we scrolled...)
		// This allows deletion of old selection when moving text around.
		if( curbeforesel )
		{
			cur_ = rsel;
			sel_ = rcur;
			//MoveTo(rcur, false);
			//MoveTo(rsel, true);
		}
		else
		{
			cur_ = rcur;
			sel_ = rsel;
			//MoveTo(rsel, false);
			//MoveTo(rcur, true);
		}
	}

}
void Cursor::InputAt( const char* str, ulong len, int x, int y )
{
	unicode* ustr = new unicode[ len*4 ];
	if(!ustr) return;
	len = ::MultiByteToWideChar( CP_ACP, 0, str, len, ustr, len*4 );
	InputAt( ustr, len, x, y );
	delete [] ustr;
}

void Cursor::DelBack( bool wide )
{
	// 選択状態なら BackSpace == Delete
	// でなければ、 BackSpace == Left + Delete (手抜き
	// Ctrl+BackSpace == Leftword + delete
	if( cur_ == sel_ )
	{
		if( cur_.tl==0 && cur_.ad==0 )
			return;
		Left( wide, /*select*/ wide );
	}
	Del( false );
}

void Cursor::Del( bool wide )
{
	// 選択状態なら cur_ ～ sel_ を削除
	// でなければ、 cur_ ～ rightOf(cur_) を削除
	// Ctrl+Del == Right + delete
	if( wide )
		Right( true, true );
	DPos dp = (cur_==sel_ ? doc_.rightOf(cur_) : (DPos)sel_ );
	if( cur_ != dp )
		doc_.Execute( Delete( cur_, dp ) );
}

// SMART INDENT RETURN
// We must copy the intentation chars at the begining
// of the line and paste them after the \r.
void Cursor::Return()
{   // User pressed the Enter!
	DPos pos = Min(cur_, sel_);
	DPos mp1 = DPos(pos.tl, 0); // begining of line.
	ulong len = doc_.getRangeLength( mp1, pos )+1;
	unicode *p = new unicode[len+1]; // room for the \r and \0...
	if(!p) return; // new failed!
	p[0] = L'\r'; // put the \r
	doc_.getText( &p[1], mp1, pos );
	ulong i;
	for (i=1; i<len && (p[i] == L' ' || p[i] == L'\t'); i++);
	p[i] = '\0'; // Null terminate before the nonspaces.
	Input(p, i);
	delete [] p;
}

void Cursor::DelToEndline( bool wide )
{
	if( cur_!=sel_ )
		MoveCur( Min(cur_, sel_), false );
	End( wide, true);
	if( cur_ != sel_ )
		doc_.Execute( Delete( cur_, sel_ ) );
}

void Cursor::DelToStartline( bool wide )
{
	if( cur_!=sel_ )
		MoveCur( Max(cur_, sel_), false );
	Home( wide, true);
	if( cur_ != sel_ )
		doc_.Execute( Delete( cur_, sel_ ) );
}

//-------------------------------------------------------------------------
// Add the quote string at the begining of each line.
//-------------------------------------------------------------------------
void Cursor::QuoteSelectionW(const unicode *qs, bool shi)
{
	// Set cursors at begining of line
	DPos dm, dM;
	if( cur_ < sel_ ) {
		cur_.ad = 0;
		dm=cur_; dM=sel_;
	} else {
		sel_.ad=0;
		dm=sel_; dM=cur_;
	}
	dm.ad=0;
	DPos ocur=cur_, osel=sel_; // save original selection

	uint qsl = my_lstrlenW(qs); // length of quote string.

	ulong len = doc_.getRangeLength( dm, dM );
	unicode *p = new unicode[len+1];
	if(!p) return;
	doc_.getText( p, dm, dM );

	ulong nlines = shi?0: 1 + dM.tl - dm.tl; // max number of tabs to add/min to remove ;
	unicode *pp = new unicode[len + (nlines + 1)*qsl]; // bufer to copy (un)tabified data
	if(!pp) { delete [] p ; return; } // unable to allocate mem
	ulong i=0, j=0;
	if (shi) {
		// skip a quote?
		if(my_instringW(p, qs))
			i+=qsl;
	} else {
		// add the quote!
		my_lstrcpyW(pp, qs);
		j+=qsl;
		// shift original selection
	}
	for (; i < len; i++)
	{
		pp[j++] = p[i];
		if((p[i] == L'\n' && (!i || p[i-1] == L'\r')) // DOS
		|| (p[i] == L'\n' && (!i || p[i-1] != L'\r')) // UNIX
		|| (p[i] == L'\r' && p[i+1] != '\n')) // MAC
		{ // We are at the begining of a line
			if(shi) {
				// Skip the quote string ?
				if(my_instringW(&p[i+1], qs)) {
					i+=qsl;
				}
			} else {
				// add the quote string?
				if (p[i+1] != '\0') {
					my_lstrcpyW(&pp[j], qs);
					j+=qsl;
				}
			}
		}
	}
	pp[j] = L'\0';
	delete [] p;
	doc_.Execute( Replace(dm, dM, pp, my_lstrlenW(pp)) );
	MoveCur(osel, false);
	MoveCur(ocur, true);

	delete [] pp;
}
void Cursor::QuoteSelection(bool unquote)
{
	QuoteSelectionW(doc_.getCommentStr(), unquote);
}
//-------------------------------------------------------------------------
// Indent/Un-indent selection with Tab/Shit+Tab.
//-------------------------------------------------------------------------
void Cursor::Tabulation(bool shi)
{
	if (cur_.tl == sel_.tl)
	{
		Input(TEXT("\t"), 1);
		return;	 // DONE
	}
	QuoteSelectionW(L"\t", shi); // Quote with a tab!
}
//-------------------------------------------------------------------------
// テキスト取得, Get Text
//-------------------------------------------------------------------------

ki::aarr<unicode> Cursor::getSelectedStr() const
{
	DPos dm=cur_, dM=sel_;
	if( cur_ > sel_ )
		dm=sel_, dM=cur_;

	// テキスト取得, Get Text
	ulong len = doc_.getRangeLength( dm, dM );
	ki::aarr<unicode> ub( new unicode[len+1] );
	doc_.getText( ub.get(), dm, dM );
	return ub;
}

//-------------------------------------------------------------------------
// クリップボード処理, Clipboard processing
//-------------------------------------------------------------------------

void Cursor::Cut()
{
	if( cur_ != sel_ )
	{
		// コピーして削除
		Copy();
		Del( false );
	}
}

void Cursor::Copy()
{
	Clipboard clp( caret_->hwnd(), false );
	if( cur_==sel_ || !clp.isOpened() )
		return;

	DPos dm=cur_, dM=sel_;
	if( cur_ > sel_ )
		dm=sel_, dM=cur_;

	HGLOBAL  h;
	ulong len = doc_.getRangeLength( dm, dM );

	if( UNICODEBOOL || app().isNT() )
	{
		// NT系ならそのままダイレクトに, Direct copy
		// Also on Win9x we can use CF_UNICODETEXT with UNICOWS
		// In MBCS build we still copy in CF_UNICODETEXT if running on NT
		h = ::GlobalAlloc( GMEM_MOVEABLE, (len+1)*2 );
		if (!h) {
			MessageBox(NULL, TEXT("Selection is too large to hold into memory!")
				, NULL, MB_OK|MB_TASKMODAL|MB_TOPMOST) ;
			return;
		}
		unicode *uh = static_cast<unicode*>(::GlobalLock(h));
		doc_.getText( uh, dm, dM );
		// Replace null characters by spaces when copying.
		for (ulong i=0; i < len; i++ )
			if( uh[i] == L'\0' )
				uh[i] = L' ';
		uh[len] = L'\0'; // in case.
		::GlobalUnlock( h );
		if( !clp.SetData( CF_UNICODETEXT, h ) )
			GlobalFree(h); // Could not set data
	}

#if !defined(_UNICODE) || defined(UNICOWS)
	if( !app().isNT() )
	{
		// On 9x With UNICOWS We need to also write to the clipboard in ANSI
		// So that other programs can access the clipboard.
		// Same for pure ansi mode.
		// 9x系なら変換が必要, convert to ANSI before.
		h = ::GlobalAlloc( GMEM_MOVEABLE, (len+1)*3 );
		if (!h) {
			MessageBox(NULL, TEXT("Selection is too large to hold into memory!")
				, NULL, MB_OK|MB_TASKMODAL|MB_TOPMOST) ;
			return;
		}
		unicode *p = new unicode[len+1];
		if(!p) return;
		doc_.getText( p, dm, dM );
		// Replace null characters by spaces when copying.
		for (ulong i=0; i < len; i++ )
			if( p[i] == L'\0' )
				p[i] = L' ';
		p[len] = L'\0'; // in case.

		::WideCharToMultiByte( CP_ACP, 0, p, len+1,
			static_cast<char*>(::GlobalLock(h)), (len+1)*3, NULL, NULL );
		::GlobalUnlock( h );
		delete [] p;
		if( !clp.SetData( CF_TEXT, h ) )
			GlobalFree(h); // Could not set data
	}
#endif // !defined(_UNICODE) || defined(UNICOWS)
}

void Cursor::Paste()
{
	Clipboard clp( caret_->hwnd(), true );
	if( clp.isOpened() )
	{
		Clipboard::Text txt = clp.GetUnicodeText();
		if( txt.data() != NULL )
			doc_.Execute(
				Replace( cur_, sel_, txt.data(), my_lstrlenW(txt.data()) )
			);
	}
}

//-------------------------------------------------------------------------
// More Edit functions
//-------------------------------------------------------------------------
unicode* WINAPI Cursor::InvertCaseW(unicode *str)
{
	if(!str) return NULL;
	for(ulong i=0; str[i] != L'\0'; i++)
	{
		if(my_IsCharLowerW(str[i]))
			str[i] = my_CharUpperSingleW(str[i]);
		else
			str[i] = my_CharLowerSingleW(str[i]);
	}
	return str;
}

unicode* WINAPI Cursor::TrimTrailingSpacesW(unicode *str)
{

	long i, j;
	bool trim = true;
	// Go through the string backward
	for (i = my_lstrlenW(str)-1, j = i; i >= 0; i--)
	{
		if (trim) { // we trim
			if (str[i] == ' ' || str[i] == '\t') {
				continue; // next i.
			} else if (str[i] != '\n' && str[i] != '\r') {
				trim = false; // stop trimming
			}
		} else { // If we stopped trimming
			if (str[i] == '\n' || str[i] == '\r') {
				trim = true; // restart just after the CR|LF
			}
		}
		str[j--] = str[i];
	}
	j++;

	return j!=0 ? &str[j]: NULL; // New string start
}
unicode* WINAPI Cursor::StripFirstCharsW(unicode *p)
{	// Remove first char of each line

	size_t  i, j, len = my_lstrlenW(p);
	for (i=0, j=0; i < len; )
	{
		if((p[i] == L'\n' && (!i || p[i-1] == L'\r')) // DOS
		|| (p[i] == L'\n' && (!i || p[i-1] != L'\r')) // UNIX
		|| (p[i] == L'\r' && p[i+1] != '\n')) // MAC
		{
			i+=p[i+1] != L'\0';
		}

		p[j++] = p[++i];
	}
	p[j] = L'\0';
	return p;
}
unicode* WINAPI Cursor::StripLastCharsW(unicode *p)
{	// Remove last char of each line

	size_t  i, j, len = my_lstrlenW(p);
	for (i=0, j=0; i < len; )
	{
		// Skip if the next character is a CR or LF, while preserving CRLF
		i += p[i+1] != '\0' && ((p[i+1] == '\n' || p[i+1] == '\r') && p[i+2] != '\r');

		p[j++] = p[i++];
	}
	p[j] = L'\0';
	return p;
}

void Cursor::ModSelection(ModProc mfunk)
{
	DPos dm=cur_, dM=sel_;
	DPos ocur=cur_, osel=sel_;
	if( cur_ > sel_ )
		dm=sel_, dM=cur_;
	else if (cur_==sel_)
		return; // Nothing to do.

	ulong len = doc_.getRangeLength( dm, dM );
	unicode *p = new unicode[len+1];
	doc_.getText( p, dm, dM );
	unicode *np = mfunk(p);
	if (np) {
		doc_.Execute( Replace(cur_, sel_, np, my_lstrlenW(np)) );
		MoveCur(osel, false);
		MoveCur(ocur, true);
	}
	delete [] p;
}
void Cursor::UpperCaseSel()
{
#if defined(UNICOWS) || !defined(_UNICODE)
	ModSelection(app().isNT()? CharUpperW: my_CharUpperW);
#else
	ModSelection(CharUpperW);
#endif
}
void Cursor::LowerCaseSel()
{
#if defined(UNICOWS) || !defined(_UNICODE)
	ModSelection(app().isNT()? CharLowerW: my_CharLowerW);
#else
	ModSelection(CharLowerW);
#endif
}
void Cursor::InvertCaseSel()
{
	ModSelection(InvertCaseW);
}
void Cursor::TTSpacesSel()
{
	ModSelection(TrimTrailingSpacesW);
}
void Cursor::StripFirstChar()
{
	if(cur_==sel_) return;
	if(cur_ < sel_)
		cur_.ad=0;
	else
		sel_.ad=0;
	ModSelection(StripFirstCharsW);
}
void Cursor::StripLastChar()
{
	if(cur_==sel_) return;
	ModSelection(StripLastCharsW);
}
//-------------------------------------------------------------------------
// カーソル移動, Cursor movement
//-------------------------------------------------------------------------

void Cursor::MoveCur( const DPos& dp, bool select )
{
	VPos vp;
	view_.ConvDPosToVPos( dp, &vp );
	MoveTo( vp, select );
}

void Cursor::MoveTo( const VPos& vp, bool sel )
{
	if( sel )
	{
		// 選択状態が変わる範囲を再描画
		// Redraw the area where the selection state changes
		Redraw( vp, cur_ );
	}
	else
	{
		// 選択解除される範囲を再描画
		// Redraw the range to be deselected
		if( cur_ != sel_ )
			Redraw( cur_, sel_ );
		sel_ = vp;
	}
	cur_ = vp;
	UpdateCaretPos();
	view_.ScrollTo( cur_ );
}

void Cursor::Home( bool wide, bool select )
{
	VPos np;
	np.ad = np.vx = np.rx = np.rl = 0;
	if( wide ) // 文書の頭へ, Go to the head of the document.
		np.tl = np.vl = 0;
	else // 行の頭へ, To the head of the line
	{
		// 1.07.4 --> 1.08 :: Virtual Home
		// np.tl = cur_.tl, np.vl = cur_.vl-cur_.rl;

		if( cur_.rl == 0 )
			np.tl = cur_.tl, np.vl = cur_.vl-cur_.rl;
		else
			view_.ConvDPosToVPos( doc_.rightOf(DPos(cur_.tl, view_.rlend(cur_.tl,cur_.rl-1))), &np, &cur_ );
	}
	MoveTo( np, select );
}

void Cursor::End( bool wide, bool select )
{
	VPos np;
	if( wide ) // 文書の末尾へ, To the end of the document
	{
		np.tl = doc_.tln()-1;
		np.vl = view_.vln()-1;
	}
	else // 行の末尾へ, To the end of the line
	{
		// 1.07.4 --> 1.08 :: Virtual End
		// np.tl = cur_.tl;
		// np.vl = cur_.vl + view_.rln(np.tl) - 1 - cur_.rl;

		view_.ConvDPosToVPos( DPos(cur_.tl, view_.rlend(cur_.tl,cur_.rl)), &np, &cur_ );
		MoveTo( np, select );
		return;
	}
	np.ad = doc_.len(np.tl);
	np.rl = view_.rln(np.tl)-1;
	np.rx = np.vx = view_.GetLastWidth( np.tl );

	MoveTo( np, select );
}

void Cursor::Ud( int dy, bool select )
{
	// はみ出す場合は、先頭行/終端行で止まるように制限
	// Limit overflow to stop at start/end line
	VPos np = cur_;
	if( (signed)np.vl + dy < 0 )
		dy = -(signed)np.vl;
	else if( np.vl + dy >= view_.vln() )
		dy = view_.vln()-np.vl-1;

	np.vl += dy;
	np.rl += dy;
	if( dy<0 ) // 上へ戻る場合, To go back to the top
	{
		// ジャンプ先論理行の行頭へDash!
		while( (signed)np.rl < 0 )
			np.rl += view_.rln(--np.tl);
	}
	else if( dy>0 ) // 下へ進む場合
	{
		// ジャンプ先論理行の行頭へDash!
		while( (signed)np.rl > 0 )
			np.rl -= view_.rln(np.tl++);
		if( (signed)np.rl < 0 )
			np.rl += view_.rln(--np.tl); //行き過ぎ修正～
	}

	// x座標決定にかかる, x-coordinate determination
	const unicode* str = doc_.tl(np.tl);

	// 右寄せになってる。不自然？, It's right-justified. Unnatural?
	np.ad = (np.rl==0 ? 0 : view_.rlend(np.tl,np.rl-1)+1);
	np.vx = (np.rl==0 ? 0 : view_.fnt().W(&str[np.ad-1]));
	while( np.vx < np.rx && np.ad < view_.rlend(np.tl,np.rl) )
	{
		// 左寄せにしてみた。
		ulong newvx;
		if( str[np.ad] == L'\t' )
			newvx = view_.fnt().nextTab(np.vx);
		else
			newvx = np.vx + view_.fnt().W(&str[np.ad]);
		if( newvx > ulong(np.rx) )
			break;
		np.vx = newvx;
		++np.ad;
	}

	MoveTo( np, select );
}

void Cursor::Up( bool wide, bool select )
{
	Ud( wide?-3:-1, select );
}

void Cursor::Down( bool wide, bool select )
{
	Ud( wide?3:1, select );
}

void Cursor::PageUp( bool select )
{
	Ud( -view_.cy()/NZero(view_.fnt().H()), select );
}

void Cursor::PageDown( bool select )
{
	Ud( view_.cy()/NZero(view_.fnt().H()), select );
}

void Cursor::Left( bool wide, bool select )
{
	VPos np;
	if( cur_!=sel_ && !select )
		np = Min( cur_, sel_ ), np.rx = np.vx;
	else
		view_.ConvDPosToVPos( doc_.leftOf(cur_,wide), &np, &cur_ );
	MoveTo( np, select );
}

void Cursor::Right( bool wide, bool select )
{
	VPos np;
	if( cur_!=sel_ && !select )
		np = Max( cur_, sel_ ), np.rx = np.vx;
	else
		view_.ConvDPosToVPos( doc_.rightOf(cur_,wide), &np, &cur_ );
	MoveTo( np, select );
}

// Go to the matching brace
#if 0
void Cursor::GotoMatchingBrace()
{
    unicode p[8];
    VPos np;
    static const unicode *braces=L"()[]{}<>";
    if( cur_!=sel_ ) return;
	DPos ssel = DPos(cur.tl, cur.ad); // only next char.
	rightOf(&ssel, 0) // Next char
//	ulong len = doc_.getRangeLength( cur_, ssel )+1;
	doc_.getText( p, cur_, &ssel );
	if (*p == '(')
	{

	}
}
#endif

//-------------------------------------------------------------------------
// マウス入力への対応
//-------------------------------------------------------------------------

void Cursor::on_lbutton_dbl( short x, short y )
{
	// 行番号ゾーンの場合は特に何もしない
	if( view_.lna()-view_.fnt().F() < x )
		// 行末の場合も特に何もしない
		if( cur_.ad != doc_.len(cur_.tl) )
		{
			VPos np;
			view_.ConvDPosToVPos( doc_.wordStartOf(cur_), &np, &cur_ );
			MoveTo( np, false );
			Right( true, true );
		}
}

bool Cursor::on_contextmenu( short x, short y )
{
	// Not Tracked
	return false;
}

void Cursor::on_lbutton_down( short x, short y, bool shift )
{
	if( !shift )
	{
		// これまでの選択範囲をクリア
		Redraw( cur_, sel_ );

		// 行番号ゾーンのクリックだったら、行選択モードに
		lineSelectMode_ = ( x < view_.lna()-view_.fnt().F() );

		// 選択開始位置を調整
		view_.GetVPos( x, y, &sel_ );
		if( lineSelectMode_ )
			view_.ConvDPosToVPos( DPos(sel_.tl,0), &sel_, &sel_ );
		cur_ = sel_;
	}

	// 移動！
	MoveByMouse( dragX_=x, dragY_=y );

	// マウス位置の追跡開始
	timerID_ = ::SetTimer( caret_->hwnd(), 178116, keyRepTime_, NULL );
	::SetCapture( caret_->hwnd() );
}

void Cursor::on_lbutton_up( short x, short y )
{
	// 追跡解除
	if( timerID_ != 0 )
	{
		::ReleaseCapture();
		::KillTimer( caret_->hwnd(), timerID_ );
		timerID_ = 0;
	}
}

bool Cursor::on_drag_start( short x, short y )
{
#ifndef NO_OLEDNDSRC
	if( cur_ != sel_ )
	{
		VPos vp;
		view_.GetVPos( x, y, &vp );
		if( isInSelection( vp ) )
		{
			const VPos dm = Min(cur_, sel_);
			const VPos dM = Max(cur_, sel_);
			ulong len = doc_.getRangeLength( dm, dM );
			unicode *p = new unicode[len+1];
			if( p )
			{
				doc_.getText( p, dm, dM );
				OleDnDSourceTxt doDrag(p, len);
				delete p;
				if( doDrag.getEffect() == DROPEFFECT_MOVE )
					doc_.Execute( Delete( cur_, sel_ ) );
			}
			return true;
		}
	}
#endif // NO_OLEDNDSRC
	return false;
}
void Cursor::on_mouse_move( short x, short y, WPARAM fwKeys )
{
	if( timerID_ != 0 )
	{
		// View内部ならMouseMoveに反応
		POINT pt = { dragX_=x, dragY_=y };
		if( PtInRect( &view_.zone(), pt ) )
			MoveByMouse( dragX_, dragY_ );
	}
}

void Cursor::on_timer()
{
	// View外部ならTimerに反応
	POINT pt = { dragX_, dragY_ };
	if( !PtInRect( &view_.zone(), pt ) )
		MoveByMouse( dragX_, dragY_ );
}

void Cursor::MoveByMouse( int x, int y )
{
	VPos vp;
	view_.GetVPos( x, y, &vp, lineSelectMode_ );
	MoveTo( vp, true );
}

//-------------------------------------------------------------------------
// IME
//-------------------------------------------------------------------------

void Cursor::Reconv()
{
#ifndef NO_IME
	if( isSelected() && ime().IsIME() && ime().CanReconv() )
	{
		aarr<unicode> ub = getSelectedStr();
		ulong len=0;
		for(len=0; ub[len]; ++len);
		ime().SetString( caret_->hwnd(), ub.get(), len);
	}
#endif
}

void Cursor::ToggleIME()
{
#ifndef NO_IME
	if( ime().IsIME() )
	{
		ime().SetState( caret_->hwnd(), !ime().GetState( caret_->hwnd() ) );
	}
#endif
}

//-------------------------------------------------------------------------
// 再変換
//-------------------------------------------------------------------------

int Cursor::on_ime_reconvertstring( RECONVERTSTRING* rs )
{
#ifndef NO_IME
	if( ! isSelected() || cur_.tl != sel_.tl )
		return 0;

#ifdef _UNICODE
	aarr<unicode> str = getSelectedStr();
#else
	aarr<char> str;
	{
		aarr<unicode> ub = getSelectedStr();
		ulong len;
		for(len=0; ub[len]; ++len);
		ki::aarr<char> nw( new TCHAR[(len+1)*3] );
		str = nw;
		::WideCharToMultiByte( CP_ACP, 0, ub.get(), -1,
			str.get(), (len+1)*3, NULL, NULL );
	}
#endif
	const ulong len = my_lstrlen(str.get());
	if( rs != NULL )
	{
		rs->dwSize            = sizeof(RECONVERTSTRING) + (len+1)*sizeof(TCHAR);
		rs->dwVersion         = 0;
		rs->dwStrOffset       = sizeof(RECONVERTSTRING);
		rs->dwStrLen          = len;
		rs->dwCompStrOffset   = 0;
		rs->dwCompStrLen      = len;
		rs->dwTargetStrOffset = 0;
		rs->dwTargetStrLen    = len;
		memmove( ((char*)rs)+rs->dwStrOffset, str.get(), (len+1)*sizeof(TCHAR) );

		if( sel_ < cur_ )
		{
			DPos psel_ = sel_;
			MoveCur( cur_, false );
			MoveCur( psel_, true );
		}
	}
	return sizeof(RECONVERTSTRING) + (len+1)*sizeof(TCHAR);
#else
	return 0;
#endif // NO_IME
}

bool Cursor::on_ime_confirmreconvertstring( RECONVERTSTRING* rs )
{
	return false;
}

//=========================================================================
// OLE Drag and Drop handler.
//=========================================================================
#ifndef NO_OLEDNDTAR
OleDnDTarget::OleDnDTarget( HWND hwnd, ViewImpl& vw )
	: refcnt   ( 1 )
	, hwnd_    ( NULL )
	, view_    ( vw )
	, comes_from_center_ ( false )
{
	ki::app().InitModule( ki::App::OLE );
	// Dyamically load because OLE32 might be missing...
	if( app().hOle32() && ::IsWindow( hwnd ) )
	{
		// Lock object (required for Win32s!
		// Useless for newer Windows versions?
		if( S_OK != MyCoLockObjectExternal( this, TRUE, FALSE ) )
		{
			HRESULT (WINAPI *dyn_RegisterDragDrop)(HWND hwnd, IDropTarget *dt) =
				( HRESULT (WINAPI *)(HWND hwnd, IDropTarget *dt) )
				GetProcAddress(app().hOle32(), "RegisterDragDrop");

			if( dyn_RegisterDragDrop && S_OK == dyn_RegisterDragDrop(hwnd, this) )
			{
				// Sucess!
				hwnd_ = hwnd;
				LOGGER( "OleDnDTarget RegisterDragDrop() Sucess!" );
				return;
			}
			else
			{	// Could not Register the Drag&Drop
				// So we must unlock the object.
				MyCoLockObjectExternal( this, FALSE, FALSE );
			}
		}
	}
}
OleDnDTarget::~OleDnDTarget(  )
{
	if( app().hOle32() && hwnd_ && ::IsWindow( hwnd_ ) )
	{
		HRESULT (WINAPI *dyn_RevokeDragDrop)(HWND hwnd) =
			( HRESULT (WINAPI *)(HWND hwnd) )
			GetProcAddress( app().hOle32(), "RevokeDragDrop" );

		if( dyn_RevokeDragDrop )
		{
			LOGGER( "~OleDnDTarget RevokeDragDrop()" );
			dyn_RevokeDragDrop(hwnd_);

			// Release all pointers to the object
			MyCoLockObjectExternal( this, FALSE, TRUE );
		}
	}
}

HRESULT STDMETHODCALLTYPE OleDnDTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	LOGGER( "OleDnDTarget::Drop()" );
	STGMEDIUM stg = { 0 };
	// Try with UNICODE text first!
	FORMATETC fmt = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	POINT pt; pt.x = ptl.x, pt.y = ptl.y;
	ScreenToClient(hwnd_, &pt);

	// Important to inform the source if we are
	// actually copying or moving the selection
	setDropEffect( grfKeyState, pdwEffect );

	if( S_OK == pDataObj->GetData(&fmt, &stg) && stg.hGlobal)
	{
		const unicode *txt = (const unicode *)::GlobalLock(stg.hGlobal);
		if( txt )
		{
			size_t len = my_lstrlenW(txt);
			if( grfKeyState&MK_SHIFT ) // Shift to ignore pt.
				view_.cur().Input( txt, len );
			else
				view_.cur().InputAt( txt, len, pt.x, pt.y );
			::GlobalUnlock(stg.hGlobal);
		}
		// We must only free the buffer when pUnkForRelease is NULL!
		// ::ReleaseStgMedium(&stg);
		if( stg.pUnkForRelease == NULL )
			::GlobalFree(stg.hGlobal);
		return S_OK;
	}

	// Fallback to ANSI TEXT
	fmt.cfFormat = CF_TEXT;
	if( S_OK == pDataObj->GetData(&fmt, &stg) && stg.hGlobal)
	{
		const char *txt = (const char *)::GlobalLock(stg.hGlobal);
		if( txt )
		{
			size_t len = my_lstrlenA(txt);
			if( grfKeyState&MK_SHIFT )
				view_.cur().Input( txt, len );
			else
				view_.cur().InputAt( txt, len, pt.x, pt.y );
			::GlobalUnlock(stg.hGlobal);
		}
		// We must only free the buffer when pUnkForRelease is NULL!
		if( stg.pUnkForRelease == NULL )
			::GlobalFree(stg.hGlobal);
		return S_OK;
	}
	// Shoud I return E_INVALIDARG ??
	return E_UNEXPECTED;
}
HRESULT STDMETHODCALLTYPE OleDnDTarget::QueryInterface(REFIID riid, void **ppvObject)
{
	// Define locally IID_IDropTarget GUID,
	// gcc bloats the exe with a bunch of useless GUIDS otherwise.
	static const IID myIID_IDropTarget = { 0x00000122, 0x0000, 0x0000, {0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
	if( memEQ(&riid, &myIID_IUnknown, sizeof(riid))
	||  memEQ(&riid, &myIID_IDropTarget, sizeof(riid)) )
	{
		LOGGER( "OleDnDTarget::QueryInterface S_OK" );
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	*ppvObject = NULL;
	LOGGER( "OleDnDTarget::QueryInterface E_NOINTERFACE" );
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE OleDnDTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	// LOGGER( "OleDnDTarget::DragOver" );
	setDropEffect( grfKeyState, pdwEffect );

	if( grfKeyState & MK_SHIFT )
	{	// If we have the Shift key down, we do not scroll
		// And the text will be pasted to the caret position
		// without being affected by cursor position.
		return S_OK;
	}
	// Scroll the content when the cursor goes towards the edges
	// of the text view, we consider two text lines/columns margins.
	// We must only scroll if the cursor comes from the central region.
	POINT pt; pt.x = ptl.x; pt.y = ptl.y;
	ScreenToClient(hwnd_, &pt);
	const int h = Min( 2*view_.fnt().H(), Max(1, view_.cy()/8) ); // Scroll vmargin
	const int w = Min( 2*view_.fnt().W(), Max(1, view_.cx()/8) ); // Scroll hmargin

	#define MULT 120
	if( view_.bottom() - pt.y < h )
	{
		if( !comes_from_center_ )
			return S_OK;
		short delta = -(h - (view_.bottom() - pt.y))*MULT / h;
		view_.on_wheel( delta );
		*pdwEffect |= DROPEFFECT_SCROLL;
	}
	else if( pt.y < h )
	{
		if( !comes_from_center_ )
			return S_OK;
		short delta = (h - pt.y)*MULT / h;
		view_.on_wheel( delta );
		*pdwEffect |= DROPEFFECT_SCROLL;
	}
	if( view_.right() - pt.x < w )
	{
		if( !comes_from_center_ )
			return S_OK;
		short delta = (h - (view_.right() - pt.x))*MULT / h;
		view_.on_hwheel( delta );
		*pdwEffect |= DROPEFFECT_SCROLL;
	}
	else if( pt.x < h )
	{
		if( !comes_from_center_ )
			return S_OK;
		short delta = -(h - pt.x)*MULT / h;
		// delta = Clamp((short)-120, delta, (short)-1);
		view_.on_hwheel( delta );
		*pdwEffect |= DROPEFFECT_SCROLL;
	}
	#undef MULT
	// We reached the central region of the edit window.
	comes_from_center_ = true;

	return S_OK;
}
void OleDnDTarget::setDropEffect(DWORD grfKeyState, DWORD *pdwEffect) const
{
	// It is maybe not the best way to do this but it works
	// check for MOVE > COPY and we convert LINK in Copy.
	// Probably there is a smarter way...
	if( *pdwEffect & DROPEFFECT_MOVE )
	{	// The source expect us to move the content?
		// Move or Copy dependig on Control state and if we can.
		if( grfKeyState & MK_CONTROL && *pdwEffect & DROPEFFECT_COPY )
			*pdwEffect = DROPEFFECT_COPY;
		else
			*pdwEffect = DROPEFFECT_MOVE;
	}
	else if( *pdwEffect & DROPEFFECT_COPY )
	{	// We cannot move the content but copy it.
		// if we have no DROPEFFECT_MOVE available
		*pdwEffect = DROPEFFECT_COPY;
	}
//	else if( *pdwEffect & DROPEFFECT_LINK )
//	{
//		// Links are left alone if they are
//		// not bundeled with DROPEFFECT_COPY
//		// Have not seen that yet.
//	}
}

#endif //NO_OLEDNDTAR
//-------------------------------------------------------------------------
