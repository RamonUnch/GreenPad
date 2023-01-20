#include "stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_draw.cpp   �`��E��
//
//		�܂�Ԃ��Ƃ��F�Ƃ����l�����A���ۂɕ`�揈����
//		�s���̂������B���ƃ��b�Z�[�W�f�B�X�p�b�`���Ȃǂ�
//		���łɂ��̃t�@�C���ɁB^^;
//
//---- ip_text.cpp   �����񑀍�E��
//---- ip_parse.cpp  �L�[���[�h���
//---- ip_wrap.cpp   �܂�Ԃ�
//---- ip_scroll.cpp �X�N���[��
//---- ip_cursor.cpp �J�[�\���R���g���[��
//=========================================================================



//-------------------------------------------------------------------------
// View�̏������E���
//-------------------------------------------------------------------------

View::ClsName
	View::className_ = TEXT("EditWing View");

View::View( doc::Document& d, HWND wnd )
	: WndImpl( className_, WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL )
	, doc_   ( d.impl() )
{
	static bool ClassRegistered = false;
	if( !ClassRegistered )
	{
		// ����\�z���̂݁A�N���X�o�^���s��
		ClassRegistered = true;
		WNDCLASS wc    = {0};
		wc.lpszClassName = className_;
		wc.style         = CS_DBLCLKS;
		wc.hCursor       = app().LoadOemCursor( IDC_IBEAM );

		// GlobalIME��L���ɂ���
		ATOM a = WndImpl::Register( &wc );
		ime().FilterWindows( &a, 1 );
	}

	// ���쐬
	Create( NULL, wnd );
}

View::~View()
{
	// ���j��
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
	impl_ = NULL;
}



//-------------------------------------------------------------------------
// �T�u�I�u�W�F�N�g�ɂ��̂܂܉�
//-------------------------------------------------------------------------

void View::SetWrapType( int wt )
	{ impl_->SetWrapType( wt ); }

void View::SetWrapSmart( bool ws )
	{ impl_->SetWrapSmart( ws ); }

void View::ShowLineNo( bool show )
	{ impl_->ShowLineNo( show ); }

void View::SetFont( const VConfig& vc )
	{ impl_->SetFont( vc ); }

void View::SetWrapLNandFont( int wt, bool ws, bool showLN, const VConfig& vc )
	{ impl_->SetWrapLNandFont( wt, ws, showLN, vc ); }

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

	case WM_LBUTTONDOWN:
		cur().on_lbutton_down( LOWORD(lp), HIWORD(lp), (wp&MK_SHIFT)!=0 );
		break;

	case WM_LBUTTONUP:
		cur().on_lbutton_up( LOWORD(lp), HIWORD(lp) );
		break;

	case WM_LBUTTONDBLCLK:
		cur().on_lbutton_dbl( LOWORD(lp), HIWORD(lp) );
		break;

	case WM_MOUSEMOVE:
		cur().on_mouse_move( LOWORD(lp), HIWORD(lp) );
		break;

	case WM_CONTEXTMENU:
		if( LOWORD(lp) == 0xFFFF && HIWORD(lp) == 0xFFFF)
		{ // User pressed the MENU KEY, use caret pos as lp.
			POINT pt;
			::GetCaretPos(&pt);
			ClientToScreen(hwnd(), &pt);
			lp = MAKELPARAM(pt.x+1, pt.y+impl().fnt().H()/2);;
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
// ���������Ƃ��l�p���h��Ƃ��A���[������{�I�ȏ���
//-------------------------------------------------------------------------
static CW_INTTYPE wtable[65536]; // static width table
static const uchar ctlMap[32] = {
	'0','1','2','3','4','5','6','7',
	'8','>','A','B','C','D','E','F',
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
	, dc_        ( ::GetDC(hwnd) )
	, cdc_       ( ::CreateCompatibleDC( dc_ ) )
	, font_      ( ::CreateFontIndirect( &vc.font ) )
	, pen_       ( ::CreatePen( PS_SOLID, 0, vc.color[CTL] ) )
	, brush_     ( ::CreateSolidBrush( vc.color[BG] ) )
//	, widthTable_( new int[65536] )
	, widthTable_( wtable )
	, fontranges_( NULL )
#ifdef WIN32S
	, useOutA_   ( app().isWin32s() || (!app().isNT() && app().getOOSVer() <= MKVER(4,00,99)) )
#endif
{
	// Release the DC that was captured just to make cdc_.
	::ReleaseDC( NULL, dc_ ); dc_ = NULL;

	// ���䕶����`�悷�邩�ۂ��H�̃t���O���L��,
	// Whether to draw control characters or not? flag is stored.
	for( unsigned i=0; i<countof(scDraw_); ++i )
		scDraw_[i] = vc.sc[i];

	// �����F���L��, Memorize text color
	for( unsigned i=0; i<countof(colorTable_); ++i )
		colorTable_[i] = vc.color[i];
	colorTable_[3] = vc.color[CMT];

	// DC�ɃZ�b�g, Setup a Compatible Device Context (CDC)
	::SelectObject( cdc_, font_  );
	::SelectObject( cdc_, pen_   );
	::SelectObject( cdc_, brush_ );
	::SetBkMode(    cdc_, TRANSPARENT );
	::SetMapMode(   cdc_, MM_TEXT );

	// �������e�[�u���������iASCII�͈͂̕����ȊO�͒x�������j
	memFF( widthTable_, 65536*sizeof(*widthTable_) );
#ifdef WIN32S
	{ // Ascii only characters, so the ansi version should always be fine.
		#ifndef SHORT_TABLEWIDTH
		::GetCharWidthA( cdc_, ' ', '~', widthTable_+' ' );
		#else
		int width['~'-' '+1];
		::GetCharWidthA( cdc_, ' ', '~', width );
		for( int i=0; i <= '~' - ' '; i++)
			widthTable_[' ' + i] = (CW_INTTYPE)width[i];
		#endif
	}
#else // NT/9x
	{
		#ifndef SHORT_TABLEWIDTH
		::GetCharWidthW( cdc_, L' ', L'~', widthTable_+L' ' );
		#else
		int width['~'-' '+1];
		::GetCharWidthW( cdc_, ' ', '~', width );
		int i;
		for( i=0; i <= '~' - ' '; i++)
			widthTable_[' ' + i] = (CW_INTTYPE)width[i];
		#endif
	}
#endif // WIN32S

	const unicode zsp=0x3000; // L'�@'
	W(&zsp); // Initialize width of L'�@'

	// ���ʃT���Q�[�g�͕������[�� (Lower surrogates have zero character width)
	mem00( widthTable_+0xDC00, (0xE000 - 0xDC00)*sizeof(*widthTable_) );
	// �����̍ő啝���v�Z, Calculate maximum width of numbers
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
	widthTable_[L'\t'] = NZero(W() * Max(1, vc.tabstep));

	// �����̏��, Height Information
	TEXTMETRIC met;
	::GetTextMetrics( cdc_, &met );
	height_ = (CW_INTTYPE) met.tmHeight;

	// LOGFONT
	::GetObject( font_, sizeof(LOGFONT), &logfont_ );

	// Try to get the unicode ranges for the selected font.
	typedef DWORD (WINAPI *GetFontUnicodeRanges_type)(HDC hdc,LPGLYPHSET lpgs);
	GetFontUnicodeRanges_type myGetFontUnicodeRanges =
		(GetFontUnicodeRanges_type)GetProcAddress(GetModuleHandle(TEXT("GDI32.DLL")), "GetFontUnicodeRanges");
	if( myGetFontUnicodeRanges )
	{ // We found the function
		DWORD frlen = myGetFontUnicodeRanges( cdc_, NULL );
		if( frlen && (fontranges_ = (GLYPHSET*) new BYTE[frlen]) )
		{
			mem00(fontranges_, frlen);
			fontranges_->cbThis = frlen;
			fontranges_->flAccel = 0;
			if( frlen != myGetFontUnicodeRanges( cdc_, fontranges_ ) )
			{ // Failed!
				delete [] ((BYTE*)fontranges_);
				fontranges_ = NULL;
			}
		}
	}
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

Painter::~Painter()
{
	// �K���ȕʃI�u�W�F�N�g���������Ď������������
	::SelectObject( cdc_, ::GetStockObject( OEM_FIXED_FONT ) );
	::SelectObject( cdc_, ::GetStockObject( BLACK_PEN ) );
	::SelectObject( cdc_, ::GetStockObject( WHITE_BRUSH ) );
	::DeleteDC( cdc_ ); // Delete compatible DC

	::DeleteObject( font_ );
	::DeleteObject( pen_ );
	::DeleteObject( brush_ );
	if( fontranges_ )
		delete [] ((BYTE *)fontranges_); // As BYTES...
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
					psText = new char[dwNum]; if( !psText ) return;
					dwNum = ::WideCharToMultiByte(CP_ACP,0 ,str,len ,psText,dwNum ,NULL,NULL);
				}
			}
			// Last fallback: truncate each widechar to a byte.
			// Needed for Win32s beta 61.
			dwNum = Min( len, (int)countof(psTXT1K) );
			for( DWORD i=0; i<dwNum; i++)
				psText[i] = (char)str[i];
		}
		BOOL ret = ::TextOutA( dc_, x, y, psText, dwNum );
		#ifdef USE_ORIGINAL_MEMMAN
		if( !ret ) ::TextOutA( dc_, x, y, psText, dwNum ); // What the fuck?
		#endif
		if (psText != psTXT1K)
			delete [] psText;
	}
	else
#endif // WIN32S
	{
	    // It seems that when USING ORIGINAL MEMMAN TextOutW can Fail
	    // and we must draw the text twice. There must be some race condition
	    // and I cannot figure it out.
		BOOL ret = ::TextOutW( dc_, x, y, str, len );
		#ifdef USE_ORIGINAL_MEMMAN
		if( !ret ) ::TextOutW( dc_, x, y, str, len ); // What the fuck?
		#endif
	}
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
	// ���p�X�y�[�X�L��(�z�`�L�X�̐c�^)��`��
	// Draw a half-width space symbol (staple core type)
	const int w=Wc(L' '), h=H();
	POINT pt[4] = {
		{ x    , y+h-4 },
		{ x    , y+h-2 },
		{ x+w-3, y+h-2 },
		{ x+w-3, y+h-5 }
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
	// �S�p�X�y�[�X�L��(�������l�p)��`��
	// Draw a full-width space symbol (flat rectangle)
	const int w=Wc(0x3000/*L'�@'*/), h=H();
	RECT rc = { x, y+h-4, x+w-2, y+h-1 };
	while( times-- )
	{
		if( 0 <= rc.right ) // Useless check? It was done before so I still do it!
			::Rectangle( dc_ , rc.left, rc.top, rc.right, rc.bottom );
		rc.left   += w;
		rc.right  += w;
	}
}



//-------------------------------------------------------------------------
// �ĕ`�悵�����͈͂� Invalidate ����B
//-------------------------------------------------------------------------

void ViewImpl::ReDraw( ReDrawType r, const DPos* s )
{
	// �܂��X�N���[���o�[���X�V, First update the scroll-bars
	UpdateScrollBar();

	switch( r )
	{
	case ALL: // �S���, The whole client area

		::InvalidateRect( hwnd_, NULL, FALSE );
		break;

	case LNAREA: // �s�ԍ��\����̂�, Line number display area only

		if( lna() > 0 )
		{
			RECT rc = { 0, 0, lna(), bottom() };
			::InvalidateRect( hwnd_, &rc, FALSE );
		}
		break;

	case LINE: // �w�肵���s�̌㔼, Second half of the specified line
	case AFTER: // �w�肵���s�ȉ��S��, Everything below the specified line

		{
			DPos st = ( s->ad==0 ? *s : doc_.leftOf(*s,true) );
			InvalidateView( st, r==AFTER );
		}
	}
}



//-------------------------------------------------------------------------
// WM_PAINT�n���h��
//-------------------------------------------------------------------------

void ViewImpl::on_paint( const PAINTSTRUCT& ps )
{
	// �`��͈͂̏����ڂ����擾, Obtain detailed information about the drawing area
	Painter& p = cvs_.getPainter();
	p.SetupDC( ps.hdc );
	VDrawInfo v( ps.rcPaint );
	GetDrawPosInfo( v );
//	// Uncomment if you want to see the drawing.
//	Sleep( 200 );
//	FillRect( ps.hdc, &ps.rcPaint, (HBRUSH)(COLOR_HIGHLIGHT+1) );
//	GdiFlush( );
//	Sleep( 200 );

	if( ps.rcPaint.right <= lna()  )
	{
		// case A: �s�ԍ��\����̂ݍX�V, Only the line number display area is updated.
		DrawLNA( v, p );
	}
	else if( lna() <= ps.rcPaint.left )
	{
		// case B: �e�L�X�g�\����̂ݍX�V, Update text display area only
		DrawTXT( v, p );
	}
	else
	{
		// case C: �����X�V, Both updates
		DrawLNA( v, p );
		p.SetClip( cvs_.zone() );
		DrawTXT( v, p );
		p.ClearClip();
	}
	p.RestoreDC();
}



//-------------------------------------------------------------------------
// �s�ԍ��]�[���`��, Line Number Zone Drawing
//-------------------------------------------------------------------------

void ViewImpl::DrawLNA( const VDrawInfo& v, Painter& p )
{
	//
	// ������̂܂ܑ����Z���s�����[�`��, Routine for additions as string
	//
	struct strint {
		strint( ulong num ) {
			int i=11;
			while( num ) digit[--i] = (unicode)(L'0'+(num%10)), num/=10;
			while(  i  ) digit[--i] = L' ';
		}
		void operator++() {
			int i=10;
			do
				if( digit[i] == L'9' )
					digit[i] = L'0';
				else
					{ ++digit[i]; return; }
			while( digit[--i] != L' ' );
			digit[i] = L'1';
		}
		void Output( Painter& f, int x, int y ) {
			for( unicode* p=digit+10; *p!=L' '; --p,x-=f.F() )
				f.CharOut( *p, x, y );
		}
		unicode digit[11];
	};

	// �w�ʏ���, backward erase
	RECT rc = { v.rc.left, v.rc.top, lna(), v.rc.bottom };
	p.Fill( rc );

	if( v.rc.top < v.YMAX )
	{
		// ���E���\��, Boundary line indication
		int line = lna() - p.F()/2;
		p.DrawLine( line, v.rc.top, line, v.YMAX );
		p.SetColor( LN );

		// �s�ԍ��\��, line number indication
		strint n = v.TLMIN+1;
		int    y = v.YMIN;
		int edge = lna() - p.F()*2;
		for( ulong i=v.TLMIN; y<v.YMAX; ++i,++n )
		{
			n.Output( p, edge, y );
			y += p.H() * rln(i);
		}
	}
}



//-------------------------------------------------------------------------
// �e�L�X�g�`��, text rendering
//-------------------------------------------------------------------------

inline void ViewImpl::Inv( int y, int xb, int xe, Painter& p )
{
	RECT rc = {
		Max( left(),  xb ), y,
		Min( right(), xe ), y+p.H()-1
	};
	p.Invert( rc );
}

void ViewImpl::DrawTXT( const VDrawInfo v, Painter& p )
{
	// �萔�P, Constant 1
//	const int   TAB = p.T();
	const int     H = p.H();
	const ulong TLM = doc_.tln()-1;

	// ��Ɨp�ϐ��P, Working variable 1
	RECT  a = { 0, v.YMIN, 0, v.YMIN+p.H() };
	int clr = -1;
	register int   x=0, x2;
	register ulong i=0, i2;

	// �_���s�P�ʂ�Loop. Loop per logical line.
	for( ulong tl=v.TLMIN; a.top<v.YMAX; ++tl )
	{
		// �萔�Q, Constant 2
		const unicode* str = doc_.tl(tl);
		const uchar*   flg = doc_.pl(tl);
		const int rYMAX = Min( v.YMAX, (int)(a.top+rln(tl)*H) );

		// ��Ɨp�ϐ��Q, Working variable 2
		ulong stt=0, end, t, n;

		// �\���s�P�ʂ�Loop
		for( ulong rl=0; a.top<rYMAX; ++rl,a.top+=H,a.bottom+=H,stt=end )
		{
			// ��Ɨp�ϐ��R, Working Variable 3
			end = rlend(tl,rl);
			if( a.bottom<=v.YMIN )
				continue;

			// �e�L�X�g�f�[�^�`��, text data rendering
			for( x2=x=0, i2=i=stt; x<=v.XMAX && i<end; x=x2,i=i2 )
			{
				// n := ����Token�̓�, n := next Token head
				t = (flg[i]>>5);
				n = i + t;
				if( n >= end )
					n = end;
				else if( t==7 || t==0 )
					while( n<end && (flg[n]>>5)==0 )
						++n;

				// x2, i2 := ����Token�̉E�[, x2, i2 := right end of this Token
				i2 ++;
				x2 = (str[i]==L'\t' ? p.nextTab(x2) : x2+p.W(&str[i]));
			//	if( x2 <= v.XMIN )
			//		x=x2, i=i2;
				while( i2<n && x2<=v.XMAX )
					x2 += p.W( &str[i2++] );
				// �ĕ`�悷�ׂ��͈͂Əd�Ȃ��Ă��Ȃ�, Not overlapping with the area that should be redrawn.
				if( x2<=v.XMIN )
					continue;

				// x, i := ���̃g�[�N���̍��[, x, i := left end of this token
				if( x<v.XMIN )
				{
					// tab�̕����߂肷���H
					x = x2, i = i2;
					while( v.XMIN<x )
						x -= p.W( &str[--i] );
				}

				// �w�i�h��Ԃ�, background filling
				a.left  = x + v.XBASE;
				a.right = x2 + v.XBASE;
				p.Fill( a );
				// GdiFlush();
				// �`��, Drawing
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
				case 0x3000://L'�@':
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
							unicode c = str[i];
							c = c<32? ctlMap[c]: ctl2Map[c-127];
							p.CharOut( c, x+v.XBASE, a.top );
							break;
						}
					}
					if( clr != (flg[i]&3) )
						p.SetColor( clr=(flg[i]&3) );
					p.StringOut( str+i, i2-i, x+v.XBASE, a.top );
					//p.StringOut( str+i, i2-i, x+v.XBASE, a.top );
					// ���̂����Q�x�`�����Ȃ��Ƃ��܂�������c
					// RAMON: I take care of it in the StringOut function.
					// If TextOut fails, I paint a second time.
					break;
				}
			}

			// �I��͈͂������甽�], If it is a selection, invert it.
			if( v.SYB<=a.top && a.top<=v.SYE )
				Inv( a.top, a.top==v.SYB?v.SXB:(v.XBASE),
				            a.top==v.SYE?v.SXE:(v.XBASE+x), p );

			// �s�������̗]����w�i�F�h, Background color fill in the margin after the end of the line
			if( x<v.XMAX )
			{
				a.left = v.XBASE + Max( v.XMIN, x );
				a.right= v.XBASE + v.XMAX;
				p.Fill( a );
			}
		}

		// �s���L���`�攽�], line end symbol rendering inversion
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

	// EOF��]����w�i�F�h, EOF after margin background color fill
	if( a.top < v.rc.bottom )
	{
		a.left   = v.rc.left;
		a.right  = v.rc.right;
		a.bottom = v.rc.bottom;
		p.Fill( a );
	}
}
