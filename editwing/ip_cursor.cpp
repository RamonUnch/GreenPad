#include "stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;
using doc::Insert;
using doc::Delete;
using doc::Replace;

HKL MyGetKeyboardLayout(DWORD dwLayout);


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
	, bIns_   ( true )
	, bRO_    ( false )
	, timerID_( 0 )
	, lineSelectMode_( false )
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
	const int  BTM = view_.bottom();

	if( sp.y == ep.y )
	{
		RECT rc = { Max(LFT,sp.x), sp.y, Min(RHT,ep.x), sp.y+view_.fnt().H() };
		::InvalidateRect( caret_->hwnd(), &rc, FALSE );
	}
	else
	{
		RECT rc = { Max(LFT,sp.x), Max(TOP,sp.y), RHT, Min<int>(BTM,sp.y+view_.fnt().H()) };
		::InvalidateRect( caret_->hwnd(), &rc, FALSE );
		RECT re = { LFT, Max(TOP,ep.y), Min(RHT,ep.x), Min<int>(BTM,ep.y+view_.fnt().H()) };
		::InvalidateRect( caret_->hwnd(), &re, FALSE );
		RECT rd = { LFT, Max(TOP,rc.bottom), RHT, Min<int>((long)BTM,re.top) };
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
	case VK_DELETE:	cur.Del();				break;
	case VK_BACK:	cur.DelBack();			break;
	case VK_INSERT: cur.SetInsMode(!cur.isInsMode()); break;
	case VK_TAB:    cur.Tabulation(sft);	break;
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
		{
			unicode wc = ch;
//			if (IsDBCSLeadByte(ch))
//			{ // store and block DBCSLeadByte
//				prevchar_ = ch;
//				return;
//			}
//			else if (IsDBCSLeadByte(prevchar_))
//			{
//				unsigned x = (prevchar_) | (ch<<8);
//				prevchar_ = 0;
//				::MultiByteToWideChar( CP_ACP, MB_COMPOSITE, (char*)&x, 2, &wc, 1 );
//			}
//			else
			if( ch & 0x80 ) // 非ASCII文字にはトリビアルでない変換が必要
			{
				// Non-ASCII characters require non-trivial conversion.
				UINT kb_cp = CP_ACP; // default ACP

#if defined(UNICOWS) || !defined(TARGET_VER) || (TARGET_VER >= 350)
				LCID lcid = LOWORD(MyGetKeyboardLayout(0));
				TCHAR cpstr[8];
				if(lcid /*::IsValidLocale(lcid, LCID_INSTALLED)*/
				&& ::GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, cpstr, countof(cpstr)))
				{	// This should be the codepage of the local associated with
					// the current keyboard layout
					UINT tcp = String::GetInt(cpstr);
					if (::IsValidCodePage(tcp))
						kb_cp = tcp;
				}
#endif
				::MultiByteToWideChar( kb_cp, MB_COMPOSITE, (char*)&ch, 1, &wc, 1 );
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

void Cursor::DelBack()
{
	// 選択状態なら BackSpace == Delete
	// でなければ、 BackSpace == Left + Delete (手抜き
	if( cur_ == sel_ )
	{
		if( cur_.tl==0 && cur_.ad==0 )
			return;
		Left( false, false );
	}
	Del();
}

void Cursor::Del()
{
	// 選択状態なら cur_ 〜 sel_ を削除
	// でなければ、 cur_ 〜 rightOf(cur_) を削除
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
		Del();
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
		doc_.getText( static_cast<unicode*>(::GlobalLock(h)), dm, dM );
		::GlobalUnlock( h );
		clp.SetData( CF_UNICODETEXT, h );
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

		::WideCharToMultiByte( CP_ACP, 0, p, len+1,
			static_cast<char*>(::GlobalLock(h)), (len+1)*3, NULL, NULL );
		::GlobalUnlock( h );
		clp.SetData( CF_TEXT, h );
		delete [] p;
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
	for(ulong i=0; str[i] != TEXT('\0'); i++)
	{
		if(IsCharLowerW(str[i]))
			str[i] = (wchar_t)(LONG_PTR)CharUpperW((wchar_t *)(LONG_PTR)(str[i]));
		else
			str[i] = (wchar_t)(LONG_PTR)CharLowerW((wchar_t *)(LONG_PTR)(str[i]));
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
	ModSelection(CharUpperW);
}
void Cursor::LowerCaseSel()
{
	ModSelection(CharLowerW);
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
			np.rl += view_.rln(--np.tl); //行き過ぎ修正〜
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

void Cursor::on_mouse_move( short x, short y )
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

