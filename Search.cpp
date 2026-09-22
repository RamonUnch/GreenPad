
#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "Search.h"
#include "NSearch.h"
#include "RSearch.h"
using namespace ki;
using namespace editwing;
using view::VPos;

//-------------------------------------------------------------------------
// Replacement escape decoding (regexp mode only) and multiline end calc
//-------------------------------------------------------------------------

static ulong DecodeReplacement( const wchar_t* src, wchar_t* dst )
{
	ulong di = 0;
	for( ulong si=0; src[si]!=L'\0'; ++si )
	{
		if( src[si]==L'\\' && src[si+1]!=L'\0' )
		{
			wchar_t n = src[++si];
			switch( n )
			{
			case L't': dst[di++]=L'\t'; break;
			case L'n': dst[di++]=L'\n'; break;
			case L'r': dst[di++]=L'\r'; break;
			case L'f': dst[di++]=L'\f'; break;
			case L'v': dst[di++]=L'\v'; break;
			case L'a': dst[di++]=L'\a'; break;
			case L'x': case L'X': {
				// ASCII + Iso 8859-1 codepoint \xXX
				uint v = 0;
				for( size_t i = 0; i < 2 && src[si+1] ; i++ )
				{
					wchar_t ch = src[++si];
					if( '0'<=ch && ch<='9' ) v = 16*v + ch-'0';
					if( 'A'<=ch && ch<='F' ) v = 16*v + ch-'A'+10;
					if( 'a'<=ch && ch<='f' ) v = 16*v + ch-'a'+10;
				}
				dst[di++] = (wchar_t)v;
				}break;
			case L'u': {
				// UCS2 codepoint \uXXXX
				uint v = 0;
				for( size_t i = 0; i < 4 && src[si+1] ; i++ )
				{
					wchar_t ch = src[++si];
					if( '0'<=ch && ch<='9' ) v = 16*v + ch-'0';
					if( 'A'<=ch && ch<='F' ) v = 16*v + ch-'A'+10;
					if( 'a'<=ch && ch<='f' ) v = 16*v + ch-'a'+10;
				}
				dst[di++] = (wchar_t)v;
				}break;
			case L'U': {
				// UTF-32 codepoint \UXXXXXXXX
				uint v = 0;
				for( size_t i = 0; i < 8 && src[si+1] ; i++ )
				{
					wchar_t ch = src[++si];
					if( '0'<=ch && ch<='9' ) v = 16*v + ch-'0';
					if( 'A'<=ch && ch<='F' ) v = 16*v + ch-'A'+10;
					if( 'a'<=ch && ch<='f' ) v = 16*v + ch-'a'+10;
				}
				if( v < 0x10000 )
				{
					dst[di++] = (wchar_t)v;
				}
				else
				{
					// Surrogate pair...
					dst[di++] = (wchar_t)(0xD800 + (((v-0x10000)>>10)&0x3ff)),
					dst[di++] = (wchar_t)(0xDC00 + (((v-0x10000)    )&0x3ff));
				}
				}break;
			default:   dst[di++]=n; break;
			}
		}
		else
		{
			dst[di++] = src[si];
		}
	}
	dst[di] = L'\0';
	return di;
}

static DPos ReplaceEndPos( const DPos& b, const wchar_t* ustr, ulong ulen )
{
	ulong breaks = 0;
	ulong lastBreakEnd = 0;
	for( ulong i=0; i<ulen; )
	{
		if( ustr[i]==L'\r' )
		{
			++breaks;
			++i;
			if( i<ulen && ustr[i]==L'\n' )
				++i;
			lastBreakEnd = i;
		}
		else if( ustr[i]==L'\n' )
		{
			++breaks;
			++i;
			lastBreakEnd = i;
		}
		else
		{
			++i;
		}
	}
	DPos e;
	if( breaks == 0 )
	{
		e.tl = b.tl;
		e.ad = b.ad + ulen;
	}
	else
	{
		e.tl = b.tl + breaks;
		e.ad = ulen - lastBreakEnd;
	}
	return e;
}



//-------------------------------------------------------------------------

SearchManager::SearchManager( ki::Window& w, editwing::EwEdit& e )
	: DlgImpl( IDD_FINDREPLACE )
	, edit_( e )
	, searcher_( NULL )
	, mainWnd_( w )
	, bIgnoreCase_( true ) // 1.08 default true
	, bRegExp_( false )
	, bDownSearch_( true )
	, bChanged_ (false )
	, inichanged_( false )
{
}

SearchManager::~SearchManager()
{
	if(searcher_)
		delete searcher_;
}

void SearchManager::SaveToINI()
{
	if (inichanged_)
	{
		ki::IniFile ini;
		inichanged_ = false;
		ini.SetSectionAsUserName();
		ini.PutBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
		ini.PutBool( TEXT("SearchRegExp"), bRegExp_ );
	}
}

void SearchManager::LoadFromINI()
{
	ki::IniFile ini;
	inichanged_ = false;
	ini.SetSectionAsUserName();
	bIgnoreCase_ = ini.GetBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
	bRegExp_     = ini.GetBool( TEXT("SearchRegExp"), bRegExp_ );
}

//-------------------------------------------------------------------------
// ダイアログ関係
//-------------------------------------------------------------------------

void SearchManager::ShowDlg()
{
//	GoModal( ::GetParent(edit_.hwnd()) );
	if( isAlive() )
	{
		SetActive();
	}
	else
	{
		GoModeless( ::GetParent(edit_.hwnd()) );
		SetCenter(hwnd(), edit_.hwnd());
		ShowUp();
	}
}

bool SearchManager::TrapMsg(MSG* msg)
{
	if( ! isAlive() || type()==MODAL )
		return false;
	return DlgImpl::PreTranslateMessage(msg);
}

void SearchManager::on_init()
{
	if( bIgnoreCase_ ) CheckItem( IDC_IGNORECASE );
	if( bRegExp_ )     CheckItem( IDC_REGEXP );

	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );
	// Set non multiline selection as find string.
	if( edit_.getCursor().isSelected() && stt->tl == end->tl )
	{
		// 選択されている状態では、基本的にそれをボックスに表示
		ulong dmy;
		aarr<unicode> str = edit_.getCursor().getSelectedStr();

		ulong len=0;
		for( ; str[len]!=L'\0' && str[len]!=L'\n'; ++len );
		str[len] = L'\0';

		if( searcher_ &&
		    searcher_->Search( str.get(), len, 0, &dmy, &dmy ) )
		{
			SetItemText( IDC_FINDBOX, findStr_.c_str() );
		}
		else
		{
		#ifdef _UNICODE
			SetItemText( IDC_FINDBOX, str.get() );
		#else
			char *ab = (char*)TS.alloc( (len+1) * 3 * sizeof(TCHAR) );
			if( ab )
			{
				::WideCharToMultiByte( CP_ACP, 0, str.get(), -1,
					ab, (len+1)*3, NULL, NULL );
				SetItemText( IDC_FINDBOX, ab );
				TS.freelast( ab, (len+1) * 3 * sizeof(TCHAR) );
			}
		#endif
		}
	}
	else
	{
		SetItemText( IDC_FINDBOX, findStr_.c_str() );
	}

	SetItemText( IDC_REPLACEBOX, replStr_.c_str() );

	fillFromHistoric(IDC_FINDBOX,    findHistoric_, countof(findHistoric_) );
	fillFromHistoric(IDC_REPLACEBOX, replHistoric_, countof(replHistoric_) );

	::SetFocus( item(IDC_FINDBOX) );
	SendMsgToItem( IDC_FINDBOX, EM_SETSEL, 0, ::GetWindowTextLength(item(IDC_FINDBOX)) );

	bChanged_ = true;
}

void SearchManager::on_destroy()
{
	bChanged_ = false;
}

bool SearchManager::on_command( UINT cmd, UINT id, HWND ctrl )
{
	if( cmd==CBN_SELCHANGE || cmd == CBN_EDITCHANGE )
	{
		// 文字列変更があったことを記憶
		bChanged_ = true;
	}
	else if( cmd==BN_CLICKED )
	{
		switch( id )
		{
		// チェックボックスの変更があったことを記憶
		case IDC_IGNORECASE:
		case IDC_REGEXP:
			bChanged_ = true;
			break;
		// ボタンが押された場合
		case ID_FINDNEXT:
			on_findnext();
			break;
		case ID_FINDPREV:
			on_findprev();
			break;
		case ID_REPLACENEXT:
			on_replacenext();
			break;
		case ID_REPLACEALL:
			on_replaceall();
			break;
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool SearchManager::on_cancel()
{
	UpdateData();

	saveToHistoric(IDC_FINDBOX,    findHistoric_, countof(findHistoric_) );
	saveToHistoric(IDC_REPLACEBOX, replHistoric_, countof(replHistoric_) );

	return true;
}

void SearchManager::on_findnext()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
	{
		FindNextImpl();
//		End( IDOK );
	}
}

void SearchManager::on_findprev()
{
	UpdateData();
	ConstructSearcher( false );
	if( isReady() )
		FindPrevImpl();
}

void SearchManager::on_replacenext()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
		ReplaceImpl();
}

void SearchManager::on_replaceall()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
		ReplaceAllImpl();
}

void SearchManager::UpdateData()
{
	// ダイアログから変更点を取り込み
	bool IgnoreCase = isItemChecked( IDC_IGNORECASE );
	bool RegExp     = isItemChecked( IDC_REGEXP );

	// Must we save save to ini?
	inichanged_ = bIgnoreCase_ != IgnoreCase || RegExp != bRegExp_;
	bIgnoreCase_ = IgnoreCase;
	bRegExp_ = RegExp;

	SaveToINI();

	TCHAR* str;
	LRESULT n = SendMsgToItem( IDC_FINDBOX, WM_GETTEXTLENGTH );
	str = (TCHAR*)TS.alloc( sizeof(TCHAR) * (n+1) );
	if( str )
	{
		GetItemText( IDC_FINDBOX, n+1, str );
		findStr_ = str;
		AddToComboBoxHistoric(IDC_FINDBOX, str);
		TS.freelast( str, sizeof(TCHAR) * (n+1) );
	}

	n = SendMsgToItem( IDC_REPLACEBOX, WM_GETTEXTLENGTH );
	str = (TCHAR*)TS.alloc( sizeof(TCHAR) * (n+1) );
	if( str )
	{
		GetItemText( IDC_REPLACEBOX, n+1, str );
		replStr_ = str;
		AddToComboBoxHistoric(IDC_REPLACEBOX, str);
		TS.freelast( str, sizeof(TCHAR) * (n+1) );
	}
}

void SearchManager::ConstructSearcher( bool down )
{
	bChanged_ = (bChanged_ || (bDownSearch_ != down));
	if( (bChanged_ || !isReady()) && findStr_.len()!=0 )
	{
		// 検索者作成
		bDownSearch_ = down;
		const unicode *u = findStr_.ConvToWChar();

		if( searcher_ )
			delete searcher_;
		searcher_ = NULL;
		if( bRegExp_ )
			searcher_ = new RSearch( u, !bIgnoreCase_, bDownSearch_ );
		else
			if( bDownSearch_ )
				if( bIgnoreCase_ )
					searcher_ = new NSearch<IgnoreCase>(u);
				else
					searcher_ = new NSearch<CaseSensitive>(u);
			else
				if( bIgnoreCase_ )
					searcher_ = new NSearchRev<IgnoreCase>(u);
				else
					searcher_ = new NSearchRev<CaseSensitive>(u);

		findStr_.FreeWCMem(u);

		// 変更終了フラグ
		bChanged_ = false;
	}
}



//-------------------------------------------------------------------------

void SearchManager::FindNext()
{
	if( !isReady() )
	{
		ShowDlg();
	}
	else
	{
		ConstructSearcher();
		if( isReady() )
			FindNextImpl();
	}
}

void SearchManager::FindPrev()
{
	if( !isReady() )
	{
		ShowDlg();
	}
	else
	{
		ConstructSearcher( false );
		if( isReady() )
			FindPrevImpl();
	}
}



//-------------------------------------------------------------------------
// 実際の処理の実装
//-------------------------------------------------------------------------

void SearchManager::FindNextImpl(bool redo)
{
	// カーソル位置取得
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// 選択範囲ありなら、選択範囲先頭の１文字先から検索
	// そうでなければカーソル位置から検索
	DPos s = *stt;
	if( *stt != *end )
	{
		if( stt->ad == edit_.getDoc().len(stt->tl) )
			s = DPos( stt->tl+1, 0 );
		else
			s = DPos( stt->tl, stt->ad+1 );
	}
	// 検索
	DPos b, e;
	if( FindNextFromImpl( s, &b, &e ) )
	{
		// 見つかったら選択
		edit_.getCursor().MoveCur( b, false );
		edit_.getCursor().MoveCur( e, true );
		return;
	}

	// 見つからなかった場合
	NotFound(!redo);
}

void SearchManager::NotFound(bool GoingDown)
{
	//MsgBox( RzsString(IDS_NOTFOUND).c_str() );
	if (GoingDown) {
		if (IDOK == MsgBox( RzsString(IDS_NOTFOUNDDOWN).c_str(), NULL, MB_OKCANCEL )) {
			edit_.getCursor().MoveCur( DPos(0,0), false );
			FindNextImpl(true);
		}
	} else {
	    MsgBox(RzsString(IDS_NOTFOUND).c_str(), NULL, MB_OK);
	}
}

void SearchManager::FindPrevImpl()
{
	// カーソル位置取得
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	if( stt->ad!=0 || stt->tl!=0 )
	{
		// 選択範囲先頭の１文字前から検索
		DPos s;
		if( stt->ad == 0 )
			s = DPos( stt->tl-1, edit_.getDoc().len(stt->tl-1) );
		else
			s = DPos( stt->tl, stt->ad-1 );

		// 検索
		DPos b, e;
		if( FindPrevFromImpl( s, &b, &e ) )
		{
			// 見つかったら選択
			edit_.getCursor().MoveCur( b, false );
			edit_.getCursor().MoveCur( e, true );
			return;
		}
	}

	// 見つからなかった場合
	NotFound();
}

bool SearchManager::FindNextFromImpl( DPos s, DPos* beg, DPos* end )
{
	// １行ずつサー, Search one line at a time
	const doc::Document& d = edit_.getDoc();
	for( ulong mbg,med,e=d.tln(); s.tl<e; ++s.tl, s.ad=0 )
		if( searcher_ && searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // 発見, Found!
		}
	return false;
}

bool SearchManager::FindPrevFromImpl( DPos s, DPos* beg, DPos* end )
{
	// １行ずつサーチ, Search one line at a time
	const doc::Document& d = edit_.getDoc();
	for( ulong mbg,med; ; s.ad=d.len(--s.tl) )
	{
		if( searcher_ && searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // 発見, Found!
		}
		if( s.tl==0 )
			break;
	}
	return false;
}

void SearchManager::ReplaceImpl()
{
	// カーソル位置取得
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// 選択範囲先頭から検索
	DPos b, e;
	if( FindNextFromImpl( *stt, &b, &e ) )
	{
		if( e == *end )
		{
			const wchar_t* raw = replStr_.ConvToWChar();
			const ulong rawLen = my_lstrlenW( raw );
			unicode* dec = NULL;
			const unicode* ustr = raw;
			ulong ulen = rawLen;
			if( bRegExp_ )
			{
				dec = (unicode *)TS.alloc( (rawLen + 1) * sizeof(unicode) );
				if (!dec) { replStr_.FreeWCMem( raw ); return; }
				ulen = DecodeReplacement( raw, dec );
				ustr = dec;
			}

			// 置換
			edit_.getDoc().Execute( doc::Replace( b, e, ustr, ulen ) );

			DPos nxt = ReplaceEndPos( b, ustr, ulen );
			replStr_.FreeWCMem( raw );
			if( dec ) TS.freelast( dec, (rawLen + 1) * sizeof(unicode) );

			if( FindNextFromImpl( nxt, &b, &e ) )
			{
				// 次を選択
				edit_.getCursor().MoveCur( b, false );
				edit_.getCursor().MoveCur( e, true );
				return;
			}
		}
		else
		{
			// そうでなければとりあえず選択
			edit_.getCursor().MoveCur( b, false );
			edit_.getCursor().MoveCur( e, true );
			return;
		}
	}
	// 見つからなかった場合
	NotFound();
}

void SearchManager::ReplaceAllImpl()
{
	// まず、実行する置換を全てここに登録する
	doc::MacroCommand mcr;

	// 置換後文字列
	const wchar_t* raw = replStr_.ConvToWChar();
	const ulong rawLen = my_lstrlenW( raw );
	unicode* dec = NULL;
	const unicode* ustr = raw;
	ulong ulen = rawLen;
	if( bRegExp_ )
	{
		dec = (unicode *)TS.alloc( (rawLen + 1) * sizeof(unicode) );
		if (!dec) { replStr_.FreeWCMem( raw ); return; }
		ulen = DecodeReplacement( raw, dec );
		ustr = dec;
	}
	// Get selection position
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );
	DPos oristt = *stt;

	// Set begining and end for replace all
	// if multi-line selection
	DPos s(0,0), dend(0, 0);
	bool noselection = true;
	if(stt->tl != end->tl)
	{ // Multi-line selection
		noselection = false;
		s = *stt; // Start of selection
		dend = *end; // End of selection
	}

	// 文書の頭から検索, Search from the beginning of the document (or selection)
	ulong tlAdd = 0;
	long adDif = 0;
	ulong prevTl = 0;
	bool firstHit = true;
	DPos b, e;
	while( FindNextFromImpl( s, &b, &e ) && (noselection || e <= dend) )
	{ // search until the end of selection if any
		if( b.tl == e.tl && b.ad == e.ad )
		{
			if( s.ad < edit_.getDoc().len( s.tl ) )
				s.ad = s.ad + 1;
			else
				s = DPos( s.tl+1, 0 );
			if( s.tl >= edit_.getDoc().tln() )
				break;
			continue;
		}
		if( firstHit )
		{
			prevTl = b.tl;
			firstHit = false;
		}
		else if( b.tl != prevTl )
		{
			adDif = 0;
			prevTl = b.tl;
		}

		s = e;

		ulong obAd = b.ad;
		ulong oeAd = e.ad;
		b.tl += tlAdd; b.ad += adDif;
		e.tl += tlAdd; e.ad += adDif;

		// 置換コマンドを登録
		mcr.Add( new doc::Replace(b,e,ustr,ulen) );
		ulong newBreaks = 0;
		ulong lastSeg = ulen;
		for( ulong ri=0; ri<ulen; )
		{
			if( ustr[ri]==L'\r' )
			{
				++newBreaks;
				++ri;
				if( ri<ulen && ustr[ri]==L'\n' )
					++ri;
				lastSeg = ulen - ri;
			}
			else if( ustr[ri]==L'\n' )
			{
				++newBreaks;
				++ri;
				lastSeg = ulen - ri;
			}
			else
			{
				++ri;
			}
		}
		if( newBreaks == 0 )
			adDif += (long)ulen - (long)(oeAd - obAd);
		else
			adDif = (long)lastSeg - (long)oeAd;
		tlAdd += newBreaks;
	}

	if( mcr.size() > 0 )
	{
		// ここで連続置換
		edit_.getDoc().Execute( mcr );
		// カーソル移動
		DPos lastEnd = ReplaceEndPos( b, ustr, ulen );
		edit_.getCursor().MoveCur( lastEnd, false );

		if (noselection)
		{
			edit_.getCursor().MoveCur( e, false );
		}
		else
		{ // Re-select the text that was modified if needed.
			edit_.getCursor().MoveCur( oristt, false );
			edit_.getCursor().MoveCur( DPos(dend.tl, dend.ad+adDif), true );
		}
		// 閉じる？
		End( IDOK );
	}

	TCHAR str[256+INT_DIGITS+1];
	::wsprintf( str, RzsString(IDS_REPLACEALLDONE).c_str(), mcr.size() );
	MsgBox( str, RzsString(IDS_APPNAME).c_str(), MB_ICONINFORMATION );

	if( dec ) TS.freelast( dec, (rawLen + 1) * sizeof(unicode) );
	replStr_.FreeWCMem( raw );
}
