
#include "../kilib/stdafx.h"
#include "ip_view.h"
using namespace ki;
using namespace editwing;
using namespace editwing::view;

//=========================================================================
// Gives the actual DPI for the hwnd, fallback to DC for older windows
// LOWORD is dpix, HIWORD is dpiy
static DWORD myGetDpiForWindow(HWND hwnd, HDC hdc)
{
#ifdef PM_DPIAWARE
	if( app().getOSVer() >= 0x0A00 ) // Win10.00
	{	// Supported wince Windows 10, version 1607 [desktop apps only]
		typedef UINT (WINAPI *funk_t)(const HWND hwnd);
		static funk_t funk = (funk_t)1;
		if (funk == (funk_t)1) /* First time */
			funk = (funk_t)GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")), "GetDpiForWindow");

		if (funk)
		{	// We know we have the function
			WORD dpixy = (WORD)funk( hwnd );
			// Same dpi along X and Y is guarenteed
			// for a Window DC on Win10
			return ( dpixy | (dpixy<<16) );
		}
	}
	// Fallback to the DPI found for the DC
	// this value is unfortunately incorect when using per-monitor DPI (Win8)
	// It always return the DPI from main monitor. This is why we MUST use
	// the above GetDpiForWindow() function available on Win10 1607.
	//
	// We could use also GetDpiForMonitor available on Win8 but on
	// Win8 per-monitor dpi awareness does not scale non-client area nor
	// standart dialog, so we only use PerMonitorV2 awareness.
#endif // PM_DPIAWARE
	WORD dpix, dpiy;
	dpix = dpiy = 96;
	if( hdc )
	{
		dpix = (WORD)GetDeviceCaps( hdc, LOGPIXELSX );
		dpiy = (WORD)GetDeviceCaps( hdc, LOGPIXELSY );
	}
	return ( dpix | (dpiy<<16) );
}

//=========================================================================
//---- ip_draw.cpp   描画・他
//
//		折り返しとか色とかを考慮しつつ、実際に描画処理を
//		行うのがここ。あとメッセージディスパッチャなども
//		ついでにこのファイルに。^^;
//
//---- ip_text.cpp   文字列操作・他
//---- ip_parse.cpp  キーワード解析
//---- ip_wrap.cpp   折り返し
//---- ip_scroll.cpp スクロール
//---- ip_cursor.cpp カーソルコントロール
//=========================================================================



//-------------------------------------------------------------------------
// Viewの初期化・解放
//-------------------------------------------------------------------------

View::ClsName
	View::className_ = TEXT("EditWing View");

View::View( doc::Document& d, HWND wnd )
	: WndImpl( className_, WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL )
	, doc_   ( d )
	, impl_  ( NULL )
{
	static bool ClassRegistered = false;
	if( !ClassRegistered )
	{
		// 初回構築時のみ、クラス登録を行う
		ClassRegistered = true;
		WNDCLASS wc      = {0};
		wc.lpszClassName = className_;
		wc.style         = CS_DBLCLKS;
		wc.hCursor       = app().LoadOemCursor( IDC_IBEAM );

		// GlobalIMEを有効にする
		ATOM a = WndImpl::Register( &wc );
		ime().FilterWindows( &a, 1 );
	}

	// 窓作成
	Create( NULL, wnd );
}

View::~View()
{
	// 窓破棄
	Destroy();
}

void View::on_create( CREATESTRUCT* cs )
{
	impl_ = new ViewImpl( *this, doc_ );
	doc_.AddHandler( this );
}

void View::on_destroy()
{
	doc_.DelHandler( this );
	delete impl_;
	impl_ = NULL;
}



//-------------------------------------------------------------------------
// サブオブジェクトにそのまま回す
//-------------------------------------------------------------------------

void View::SetWrapType( short wt )
	{ impl_->SetWrapType( wt ); }

void View::SetWrapSmart( bool ws )
	{ impl_->SetWrapSmart( ws ); }

void View::ShowLineNo( bool show )
	{ impl_->ShowLineNo( show ); }

void View::SetFont( const VConfig& vc, short zoom )
	{ impl_->SetFont( vc, zoom ); }

void View::SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom )
	{ impl_->SetWrapLNandFont( wt, ws, showLN, vc, zoom ); }

void View::on_keyword_change()
	{ ::InvalidateRect( hwnd(), NULL, FALSE ); }

void View::on_text_update
  ( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur )
	{ impl_->on_text_update( s, e, e2, bAft, mCur ); }

Cursor& View::cur()
	{ return impl_->cur(); }

LRESULT View::on_message( UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{

	case WM_PAINT:{
		PAINTSTRUCT ps;
		::BeginPaint( hwnd(), &ps );
		impl_->on_paint( ps );
		::EndPaint( hwnd(), &ps );
		}break;

	case WM_SIZE:
		impl_->on_view_resize( LOWORD(lp), HIWORD(lp) );
		break;

	case WM_HSCROLL:
		impl_->on_hscroll( LOWORD(wp), HIWORD(wp) );
		break;

	case WM_VSCROLL:
		impl_->on_vscroll( LOWORD(wp), HIWORD(wp) );
		break;

	case WM_MOUSEWHEEL:
		if( GetKeyState(VK_CONTROL) & 0x8000 )
		{
			PostMessage(GetParent(hwnd()), WM_MOUSEWHEEL, wp|MK_CONTROL, lp);
			break;
		}
		impl_->on_wheel( HIWORD(wp) );
		break;

	case 0x020E: //WM_MOUSEHWHEEL
		impl_->on_hwheel( (short)HIWORD(wp) );
		break;

	case WM_SETFOCUS:
		cur().on_setfocus();
		break;

	case WM_KILLFOCUS:
		cur().on_killfocus();
		break;

	case WM_TIMER:
		cur().on_timer();
		break;

	case WM_KEYDOWN:
		cur().on_keydown( (int)wp, lp );
		break;

	case WM_CHAR:
		cur().on_char( (TCHAR)wp );
		break;

//	// Handle Edit-like mesages.
//	case WM_CUT:        cur().Cut();       break;
//	case WM_COPY:       cur().Copy();      break;
//	case WM_PASTE:      cur().Paste();     break;
//	case WM_CLEAR:  if( cur().isSelected() ){ cur().Del(false); } break;
//	case WM_UNDO:       doc_.Undo();       break;

	case 0x0109: // WM_UNICHAR
		if( wp != 0xffff /*UNICODE_NOCHAR*/ )
		{	// A valid UTF-32 character was sent in wp.
			if( wp < 0x80 )
				cur().on_char( (TCHAR)wp );
			else
				cur().InputUTF32( static_cast<qbyte>( wp ) );
			return FALSE; // Message Processed.
		}
		return TRUE;

	case WM_INPUTLANGCHANGE:
		cur().on_inputlangchange( (HKL)lp );
		return WndImpl::on_message( msg, wp, lp );

	case WM_LBUTTONDOWN:
		cur().on_lbutton_down( LOWORD(lp), HIWORD(lp), (wp&MK_SHIFT)!=0 );
		break;

	case WM_LBUTTONUP:
		cur().on_lbutton_up( LOWORD(lp), HIWORD(lp) );
		break;

	#ifndef NO_OLEDNDTAR
	case WM_RBUTTONDOWN: {
		if( coolDragDetect( hwnd(), /*pt=*/lp, WM_RBUTTONUP,  PM_NOREMOVE ) )
			cur().on_drag_start( LOWORD(lp), HIWORD(lp) );
		} break;
	#endif // NO_OLEDNDTAR

	case WM_LBUTTONDBLCLK:
		cur().on_lbutton_dbl( LOWORD(lp), HIWORD(lp) );
		break;

	case WM_MOUSEMOVE:
		cur().on_mouse_move( LOWORD(lp), HIWORD(lp), wp );
		break;

	case WM_CONTEXTMENU:
		if( LOWORD(lp) == 0xFFFF && HIWORD(lp) == 0xFFFF)
		{ // User pressed the MENU KEY, use caret pos as lp.
			POINT pt;
			::GetCaretPos(&pt);
			ClientToScreen(hwnd(), &pt);
			lp = MAKELPARAM(pt.x+1, pt.y+impl_->fnt().H()/2);;
		}
		if( !cur().on_contextmenu( LOWORD(lp), HIWORD(lp) ) )
			return WndImpl::on_message( msg, wp, lp );
		break;

#ifndef NO_IME
	case WM_IME_REQUEST:
		switch( wp )
		{
		case IMR_RECONVERTSTRING:
			return cur().on_ime_reconvertstring(
				reinterpret_cast<RECONVERTSTRING*>(lp) );
		case IMR_CONFIRMRECONVERTSTRING:
			return cur().on_ime_confirmreconvertstring(
				reinterpret_cast<RECONVERTSTRING*>(lp) );
		}
		break;

	case WM_IME_STARTCOMPOSITION:
		cur().on_ime_composition( 0 );
		return WndImpl::on_message( msg, wp, lp );

	case WM_IME_COMPOSITION:
		cur().on_ime_composition( lp );
		if( lp&GCS_RESULTSTR )
			break;
		// fall through...
#endif
	default:
		return WndImpl::on_message( msg, wp, lp );
	}
	return 0;
}



//-------------------------------------------------------------------------
// 線を引くとか四角く塗るとか、そーいう基本的な処理
//-------------------------------------------------------------------------
static CW_INTTYPE wtable[65536]; // static width table
static const uchar ctlMap[32] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F',
	'G','H','I','J','K','L','M','N',
	'O','P','Q','R','S','T','U','V',
};
static const uchar ctl2Map[34] = {
	'~', // DEL!
	'W','X','Y','Z','a','b','c','d',
	'e','f','g','h','i','j','k','l',
	'm','n','o','p','q','r','s','t',
	'u','v','w','x','y','z','+','\\',
	'^', // NBSP
};
Painter::Painter( HWND hwnd, const VConfig& vc )
	: hwnd_      ( hwnd )
	, dc_        ( NULL )
	, cdc_       ( NULL )
	, font_      ( NULL )
	, pen_       ( NULL )
	, brush_     ( NULL )
//	, widthTable_( new int[65536] )
	, widthTable_( wtable )
	, height_    ( 16 )
	, figWidth_  ( 8  )
	, fontranges_( NULL )
#ifdef WIN32S
	, useOutA_   ( app().isWin32s() || (!app().isNT() && app().getOOSVer() <= MKVER(4,00,99)) )
#endif
{
	Init( vc );
}
void Painter::Init( const VConfig& vc )
{
	dc_ = ::GetDC(hwnd_);
	cdc_ = ::CreateCompatibleDC( dc_ );
	::ReleaseDC( NULL, dc_ ); dc_ = NULL;

	font_ = init_font( vc );
	brush_ = ::CreateSolidBrush( vc.color[BG] );
	// 制御文字を描画するか否か？のフラグを記憶,
	// Whether to draw control characters or not? flag is stored.
	scDraw_ = vc.sc;

	// 文字色を記憶, Memorize text color
	for( unsigned i=0; i<countof(colorTable_); ++i )
		colorTable_[i] = vc.color[i];
	colorTable_[3] = vc.color[CMT];

	if( !font_ ) // Dummy font, no CDC/Tablewidth to setup.
		return;

	// DCにセット, Setup the Compatible Device Context (CDC)
	::SelectObject( cdc_, font_  );
	::SelectObject( cdc_, brush_ );
	::SetBkMode(    cdc_, TRANSPARENT );
	::SetMapMode(   cdc_, MM_TEXT );

	// 高さの情報, Height Information
	TEXTMETRIC met;
	::GetTextMetrics( cdc_, &met );
	height_ = (CW_INTTYPE) met.tmHeight;

	// Create a pen that is a 16th of font height. (min is 1px)
	pen_ = ::CreatePen( PS_SOLID, height_/16, vc.color[CTL] );
	::SelectObject( cdc_, pen_ );


	// 文字幅テーブル初期化（ASCII範囲の文字以外は遅延処理）
	memFF( widthTable_, 65536*sizeof(*widthTable_) );
	{ // Ascii only characters
		#ifdef WIN32S
			#define GETCHARWIDTH GetCharWidthA
		#else
			#define GETCHARWIDTH GetCharWidthW
		#endif

		#ifndef SHORT_TABLEWIDTH
		::GETCHARWIDTH( cdc_, 0, 127, widthTable_ );
		#else
		int width[128];
		::GETCHARWIDTH( cdc_, 0, 127, width );
		for( int i=0; i <= 127 ; i++)
			widthTable_[i] = static_cast<CW_INTTYPE>(width[i]);
		#endif

		#undef GETCHARWIDTH
	}
	const unicode zsp[2] = { 0x3000, 0x0000 }; // L'　'
	W(zsp); // Initialize width of L'　'

#ifdef WIN32S
	if( !useOutA_ )
#endif
	{	// Initialize width of U+FFFF on unicode drawing,
		// because GetCharWidthW(0xFFFF) crashes on Win95!
		SIZE sz;
		const unicode uniundef = 0xFFFF;
		::GetTextExtentPointW( cdc_, &uniundef, 1, &sz );
		widthTable_[ uniundef ] = static_cast<CW_INTTYPE>(sz.cx);
	}

	// 下位サロゲートは文字幅ゼロ (Lower surrogates have zero character width)
	mem00( widthTable_+0xDC00, (0xE000 - 0xDC00)*sizeof(*widthTable_) );
	// 数字の最大幅を計算, Calculate maximum width of numbers
	figWidth_ = 0;
	for( unicode ch=L'0'; ch<=L'9'; ++ch )
		if( figWidth_ < widthTable_[ch] )
			figWidth_ = widthTable_[ch];

	// C0 control shown as colored 0-9+ A-W
	for( unicode ch=L'\0'; ch<L' '; ++ch )
		widthTable_[ch]  =  widthTable_[ctlMap[ch]];

	// C1 Controls shown as colored W-Z, a-z, +, /,
	// + DEL (127) shown as colored '~'
	if( sc(scZSP) )
		for (unicode ch=127; ch<=160; ++ch)
			widthTable_[ch] = widthTable_[ctl2Map[ch-(unicode)127]];

	// Set the width of a Tabulation
	widthTable_[L'\t'] = W() * Max((uchar)1, vc.tabstep);
	if( widthTable_[L'\t'] == 0 )
		widthTable_[L'\t'] = 1;

	// LOGFONT
	::GetObject( font_, sizeof(LOGFONT), &logfont_ );

	// Try to get the unicode ranges for the selected font.
	typedef DWORD (WINAPI *GetFontUnicodeRanges_type)(HDC hdc,LPGLYPHSET lpgs);
	GetFontUnicodeRanges_type myGetFontUnicodeRanges =
		(GetFontUnicodeRanges_type)GetProcAddress(GetModuleHandle(TEXT("GDI32.DLL")), "GetFontUnicodeRanges");
	if( myGetFontUnicodeRanges )
	{ // We found the function
		DWORD frlen = myGetFontUnicodeRanges( cdc_, NULL );
		LOGGERF(TEXT("GetFontUnicodeRanges->frlen=%lu"), frlen );
		if( frlen && (fontranges_ = reinterpret_cast<GLYPHSET*>( malloc(frlen) )) )
		{
			mem00(fontranges_, frlen);
			fontranges_->cbThis = frlen;
			fontranges_->flAccel = 0;
			if( frlen != myGetFontUnicodeRanges( cdc_, fontranges_ ) )
			{ // Failed!
				free(fontranges_);
				fontranges_ = NULL;
			}
		}
	}
}

HFONT Painter::init_font( const VConfig& vc )
{
	// Create a font that has the correct size with regards to the
	// DPI of the current hwnd.
	if( !vc.fontsize && !vc.font.lfFaceName[0]  )
		return NULL; // Dummy font for first init
	LOGFONT lf;
	memmove( &lf, &vc.font, sizeof(lf) );

	DWORD dpixy = ::myGetDpiForWindow( hwnd_, dc_ );
	lf.lfHeight = -MulDiv(vc.fontsize,  HIWORD(dpixy), 72);
	if( vc.fontwidth )
		lf.lfWidth  = -MulDiv(vc.fontwidth, LOWORD(dpixy), 72);
	return ::CreateFontIndirect( &lf );
}
void Painter::SetupDC(HDC hdc)
{
	//Setup Device Context (DC)
	dc_ = hdc;
	oldfont_ =   (HFONT)::SelectObject( dc_, font_  );
	oldpen_ =     (HPEN)::SelectObject( dc_, pen_   );
	oldbrush_ = (HBRUSH)::SelectObject( dc_, brush_ );
	::SetBkMode(    dc_, TRANSPARENT );
	::SetMapMode(   dc_, MM_TEXT );
}
void Painter::RestoreDC()
{
	// Restore the old fonts for DC.
	// In theory it is not required to restore the old fonts
	// because EndPaint is supposed to do it, but I rather be safe than sorry
	// Plus drmermory complains otherwise.
	// I think it is quite negligible in painting time anyway.
	::SelectObject( dc_, oldfont_ );
	::SelectObject( dc_, oldpen_ );
	::SelectObject( dc_, oldbrush_ );
	// Zero out dc_ to be sure we are not going to use it later by mistake.
	dc_ = NULL;
}

void Painter::Destroy()
{
	// 適当な別オブジェクトをくっつけて自分を解放する
	::SelectObject( cdc_, ::GetStockObject( OEM_FIXED_FONT ) );
	::SelectObject( cdc_, ::GetStockObject( BLACK_PEN ) );
	::SelectObject( cdc_, ::GetStockObject( WHITE_BRUSH ) );
	::DeleteDC( cdc_ ); // Delete compatible DC

	::DeleteObject( font_ );
	::DeleteObject( pen_ );
	::DeleteObject( brush_ );
	if( fontranges_ )
		free(fontranges_);

	dc_ = NULL;
	cdc_ = NULL;
	font_ = NULL;
	pen_ = NULL;
	brush_ = NULL;
	fontranges_ = NULL;
//	delete [] widthTable_;
}

inline void Painter::CharOut( unicode ch, int x, int y )
{
#ifdef WIN32S
	// Actually for now we only use CharOut for ASCII characters
	::TextOutA( dc_, x, y, (char*)&ch, 1 ); // Only ASCII!!!
#else
	// Windows 9x/NT
	::TextOutW( dc_, x, y, &ch, 1 );
#endif
}

inline void Painter::StringOut
	( const unicode* str, int len, int x, int y )
{
#ifdef WIN32S
	if( useOutA_ )
	{
		DWORD dwNum;
		char psTXT1K[1024];
		char *psText = psTXT1K;
		if(!len) return;
		// 1st try to convert to ANSI with a small stack buffer...
		dwNum = ::WideCharToMultiByte( CP_ACP,0, str,len, psText, countof(psTXT1K), NULL,NULL );
		if( !dwNum )
		{
			if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{	// If the small buffer failed, then properly allocate buffer.
				// This happens verty rarely because token length is typically
				// a single word, hence less than 128chars.
				dwNum = ::WideCharToMultiByte(CP_ACP,0, str,len, NULL,0, NULL,NULL);
				if (dwNum)
				{
					psText = (char *)malloc( dwNum * sizeof(char) ); if( !psText ) return;
					dwNum = ::WideCharToMultiByte(CP_ACP,0 ,str,len ,psText,dwNum ,NULL,NULL);
				}
			}
		}
		::TextOutA( dc_, x, y, psText, dwNum );
		if (psText != psTXT1K)
			free( psText );
	}
	else
#endif // WIN32S
	{
		// If unicode text is not 2bytes-aligned then TextOutW can randomly fail
		// To avoid this we must be careful in the Line class...
		BOOL ret = ::TextOutW( dc_, x, y, str, len );
		#ifdef _DEBUG
			if(!ret) LOGGER("TextOutW failed!");
		#endif
	}
}

inline void Painter::StringOutA( const char* str, int len, int x, int y )
{
	::TextOutA( dc_, x, y, str, len );
}

// Print all control characters 0-31 and 127-160
// First copy to a temp buffer char for Win32s and wide for unicode.
// This is much faster than a char by char print
void Painter::DrawCTLs( const unicode *str, int len, int x, int y )
{
	#ifdef WIN32S
	char buf[256];
	#else
	unicode buf[256];
	#endif
	do
	{
		int mx = Min( len, (int)countof(buf) );
		int x2 = x;
		for(int j=0 ; j<mx ; ++j )
		{
			const unicode ch = str[j];
			if      (               ch <= 31 ) buf[j] = ctlMap[ch];
			else if (127 <= ch &&   ch <= 160) buf[j] = ctl2Map[ch-127];
			else                               buf[j] = '?';
			x2 += Wc( buf[j] );
		}
		#ifdef WIN32S
		::TextOutA( dc_, x, y, buf, mx );
		#else
		::TextOutW( dc_, x, y, buf, mx );
		#endif
		len -= countof(buf);
		str += countof(buf);
		x = x2;
	} while ( len > 0 );
}

inline void Painter::SetColor( int i )
{
	::SetTextColor( dc_, colorTable_[i] );
}

inline void Painter::Fill( const RECT& rc )
{
	::FillRect( dc_, &rc, brush_ );
}

inline void Painter::Invert( const RECT& rc )
{
	::InvertRect( dc_, &rc );
}

inline void Painter::DrawLine( int x1, int y1, int x2, int y2 )
{
	::MoveToEx( dc_, x1, y1, NULL );
	::LineTo( dc_, x2, y2 );
}

inline void Painter::SetClip( const RECT& rc )
{
	::IntersectClipRect( dc_, rc.left, rc.top, rc.right, rc.bottom );
}

inline void Painter::ClearClip()
{
	::SelectClipRgn( dc_, NULL );
}

void Painter::DrawHSP( int x, int y, int times )
{
	// 半角スペース記号(ホチキスの芯型)を描く
	// Draw a half-width space symbol (staple core type)
	const int w=Wc(L' '), h=H();
	const int rh = Max(h/4, 4);
	const int pw = Max(h/16, 1);

	POINT pt[4] = {
		{ x+pw    , y+h-rh },
		{ x+pw    , y+h-2*pw },
		{ x+w-2*pw, y+h-2*pw },
		{ x+w-2*pw, y+h-rh-1 }
	};
	while( times-- )
	{
		if( 0 <= pt[3].x )
			::Polyline( dc_, pt, countof(pt) );
		pt[0].x += w;
		pt[1].x += w;
		pt[2].x += w;
		pt[3].x += w;
	}
}

void Painter::DrawZSP( int x, int y, int times )
{
	// 全角スペース記号(平たい四角)を描く
	// Draw a full-width space symbol (flat rectangle)
	const int w=Wc(0x3000/*L'　'*/), h=H();
	const int rh = Max(h/4, 4);
	const int pw = Max(h/16, 1);
	RECT rc = { x+pw, y+h-rh, x+w-pw, y+h-pw };
	while( times-- )
	{
		if( 0 <= rc.right )
			::Rectangle( dc_ , rc.left, rc.top, rc.right, rc.bottom );
		rc.left   += w;
		rc.right  += w;
	}
}



//-------------------------------------------------------------------------
// 再描画したい範囲を Invalidate する。
//-------------------------------------------------------------------------

void ViewImpl::ReDraw( ReDrawType r, const DPos* s )
{
	// まずスクロールバーを更新, First update the scroll-bars
	UpdateScrollBar();

	switch( r )
	{
	case ALL: // 全画面, The whole client area

		::InvalidateRect( hwnd_, NULL, FALSE );
		break;

	case LNAREA: // 行番号表示域のみ, Line number display area only

		if( lna() > 0 )
		{
			RECT rc = { 0, 0, lna(), bottom() };
			::InvalidateRect( hwnd_, &rc, FALSE );
		}
		break;

	case LINE: // 指定した行の後半, Second half of the specified line
	case AFTER: // 指定した行以下全部, Everything below the specified line

		{
			DPos st = ( s->ad==0 ? *s : doc_.leftOf(*s,true) );
			InvalidateView( st, r==AFTER );
		}
	}
}



//-------------------------------------------------------------------------
// WM_PAINTハンドラ
//-------------------------------------------------------------------------

void A_HOT ViewImpl::on_paint( const PAINTSTRUCT& ps )
{
	// 描画範囲の情報を詳しく取得, Obtain detailed information about the drawing area
	Painter& p = cvs_.font_;
	p.SetupDC( ps.hdc );
	VDrawInfo v( ps.rcPaint );
	GetDrawPosInfo( v );
	// Uncomment if you want to see the drawing.
//	Sleep( 200 );
//	FillRect( ps.hdc, &ps.rcPaint, (HBRUSH)(COLOR_HIGHLIGHT+1) );
//	GdiFlush( );
//	Sleep( 200 );

	if( ps.rcPaint.right <= lna()  )
	{
		// case A: 行番号表示域のみ更新, Only the line number display area is updated.
		DrawLNA( v, p );
	}
	else if( lna() <= ps.rcPaint.left )
	{
		// case B: テキスト表示域のみ更新, Update text display area only
		DrawTXT( v, p );
	}
	else
	{
		// case C: 両方更新, Both updates
		DrawLNA( v, p );
		p.SetClip( cvs_.zone() );
		DrawTXT( v, p );
		p.ClearClip();
	}
	p.RestoreDC();
}



//-------------------------------------------------------------------------
// 行番号ゾーン描画, Line Number Zone Drawing
//-------------------------------------------------------------------------

void ViewImpl::DrawLNA( const VDrawInfo& v, Painter& p )
{
	// 背面消去, backward erase
	RECT rc = { v.rc.left, v.rc.top, lna(), v.rc.bottom };
	TCHAR digitsbuf[ULONG_DIGITS+1];
	p.Fill( rc );

	if( v.rc.top < v.YMAX )
	{
		// 境界線表示, Boundary line indication
		int line = lna() - p.F()/2;
		p.DrawLine( line, v.rc.top, line, v.YMAX );
		p.SetColor( LN );

		// 行番号表示, line number indication
		ulong  n = v.TLMIN+1;
		int    y = v.YMIN;
		int edge = lna() - p.F();
		for( ulong i=v.TLMIN; y<v.YMAX; ++i,++n )
		{
			const TCHAR *s = Ulong2lStr( digitsbuf, n );
			int numwidth=0, sl=0;
			while( s[sl] )
				numwidth += p.Wc( s[sl++] );

			#ifdef UNICODE
			p.StringOut( s, sl, edge - numwidth, y );
			#else
			p.StringOutA( s, sl, edge - numwidth, y );
			#endif
			y += p.H() * rln(i);
		}
	}
}



//-------------------------------------------------------------------------
// テキスト描画, text rendering
//-------------------------------------------------------------------------

inline void ViewImpl::Inv( int y, int xb, int xe, Painter& p )
{
	RECT rc = {
		Max( left(),  xb ), y,
		Min( right(), xe ), y+p.H()-1
	};
	p.Invert( rc );
}

void ViewImpl::DrawTXT( const VDrawInfo &v, Painter& p )
{
	if( doc_.isBusy() ) return;
	// 定数１, Constant 1
//	const int   TAB = p.T();
	const int     H = p.H();
	const ulong TLM = doc_.tln()-1;

	// 作業用変数１, Working variable 1
	RECT  a = { 0, v.YMIN, 0, v.YMIN+p.H() };
	int clr = -1;
	register int   x=0, x2;
	register ulong i=0, i2;
	// 論理行単位のLoop. Loop per logical line.
	for( ulong tl=v.TLMIN; a.top<v.YMAX; ++tl )
	{
		// 定数２, Constant 2
		const unicode* str = doc_.tl(tl);
		const uchar*   flg = doc_.pl(tl);
		const int rYMAX = Min( v.YMAX, (int)(a.top+rln(tl)*H) );

		// 作業用変数２, Working variable 2
		ulong stt=0, end, t, n;
		ulong rl=0;
		if( a.top <= -H )
		{	// Skip all warp lines that are outside the view.
			rl = (-a.top)/H - 1;
			a.top    += H * rl;
			a.bottom += H * rl;
			stt = end = rlend(tl,rl);
		}
		// 表示行単位のLoop
		for( ; a.top<rYMAX; ++rl,a.top+=H,a.bottom+=H,stt=end )
		{
			// 作業用変数３, Working Variable 3
			end = rlend(tl,rl);
			if( a.bottom<=v.YMIN )
				continue;

			// テキストデータ描画, text data rendering
			for( x2=x=0, i2=i=stt; x<=v.XMAX && i<end; x=x2,i=i2 )
			{
				// n := 次のTokenの頭, n := next Token head
				t = (flg[i]>>5);
				n = i + t;
				if( n >= end )
					n = end;
				else if( t==7 || t==0 )
					while( n<end && (flg[n]>>5)==0 )
						++n;

				// x2, i2 := このTokenの右端, x2, i2 := right end of this Token
				i2 ++;
				x2 = (str[i]==L'\t' ? p.nextTab(x2) : x2+p.W(&str[i]));
			//	if( x2 <= v.XMIN )
			//		x=x2, i=i2;
				while( i2<n && x2<=v.XMAX )
					x2 += p.W( &str[i2++] );
				// 再描画すべき範囲と重なっていない, Not overlapping with the area that should be redrawn.
				if( x2<=v.XMIN )
					continue;

				// x, i := このトークンの左端, x, i := left end of this token
				if( x<v.XMIN )
				{
					// tabの分が戻りすぎ？
					x = x2, i = i2;
					while( v.XMIN<x )
						x -= p.W( &str[--i] );
				}

				// 背景塗りつぶし, background filling
				a.left  = x + v.XBASE;
				a.right = x2 + v.XBASE;
				p.Fill( a );
				// GdiFlush();
				// 描画, Drawing
				switch( str[i] )
				{
				case L'\t': // 9
					if( p.sc(scTAB) )
					{
						p.SetColor( clr=CTL );
						for( ; i<i2; ++i, x=p.nextTab(x) )
							p.CharOut( L'>', x+v.XBASE, a.top );
					}
					break;
				case L' ': // Normal ASCII space (0x0020)
					if( p.sc(scHSP) )
						p.DrawHSP( x+v.XBASE, a.top, i2-i );
					break;
				case 0x3000://L'　':
					if( p.sc(scZSP) )
						p.DrawZSP( x+v.XBASE, a.top, i2-i );
					break;
				default:
					if( p.sc(scZSP) )
					{
						const unicode u = str[i] ;
						if( u <= 31 || (127 <=u && u <= 160) )
						{ // ASCII C0 and C1 Control caracters + DEL + NBSP
							p.SetColor( clr=CTL );
							p.DrawCTLs( &str[i], i2-i, x+v.XBASE, a.top );
							break;
						}
					}
					if( clr != (flg[i]&3) )
						p.SetColor( clr=(flg[i]&3) );
					p.StringOut( str+i, i2-i, x+v.XBASE, a.top );
					break;
				}
			}

			// 選択範囲だったら反転, If it is a selection, invert it.
			if( v.SYB<=a.top && a.top<=v.SYE )
				Inv( a.top, a.top==v.SYB?v.SXB:(v.XBASE),
				            a.top==v.SYE?v.SXE:(v.XBASE+x), p );

			// 行末より後ろの余白を背景色塗, Background color fill in the margin after the end of the line
			if( x<v.XMAX )
			{
				a.left = v.XBASE + Max( v.XMIN, x );
				a.right= v.XBASE + v.XMAX;
				p.Fill( a );
			}
		}

		// 行末記号描画反転, line end symbol rendering inversion
		SpecialChars sc = (tl==TLM ? scEOF : scEOL);
		if( i==doc_.len(tl) && -32768<x+v.XBASE )
		{
			if( p.sc(sc) )
			{
				static const unicode* const sstr[] = { L"[EOF]", L"/" };
				static const int slen[] = { 5, 1 };
				p.SetColor( clr=CTL );
				p.StringOut( sstr[sc], slen[sc], x+v.XBASE, a.top-H );
			}
			if( v.SYB<a.top && a.top<=v.SYE && sc==scEOL )
				Inv( a.top-H, x+v.XBASE, x+v.XBASE+p.Wc(L'/'), p );
		}
	}

	// EOF後余白を背景色塗, EOF after margin background color fill
	if( a.top < v.rc.bottom )
	{
		a.left   = v.rc.left;
		a.right  = v.rc.right;
		a.bottom = v.rc.bottom;
		p.Fill( a );
	}
}
