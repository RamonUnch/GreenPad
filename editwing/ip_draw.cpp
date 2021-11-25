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
#if !defined(TARGET_VER) || TARGET_VER>350
		WNDCLASSEX wc    = {0};
#else
		WNDCLASS wc    = {0};
#endif
		wc.lpszClassName = className_;
		wc.style         = CS_DBLCLKS | CS_OWNDC;
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

void View::ShowLineNo( bool show )
	{ impl_->ShowLineNo( show ); }

void View::SetFont( const VConfig& vc )
	{ impl_->SetFont( vc ); }

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
		if( !cur().on_contextmenu( LOWORD(lp), HIWORD(lp) ) )
			return WndImpl::on_message( msg, wp, lp );
		break;

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

	default:
		return WndImpl::on_message( msg, wp, lp );
	}
	return 0;
}



//-------------------------------------------------------------------------
// ���������Ƃ��l�p���h��Ƃ��A���[������{�I�ȏ���
//-------------------------------------------------------------------------

Painter::Painter( HDC hdc, const VConfig& vc )
	: dc_        ( hdc )
	, font_      ( ::CreateFontIndirect( &vc.font ) )
	, pen_       ( ::CreatePen( PS_SOLID, 0, vc.color[CTL] ) )
	, brush_     ( ::CreateSolidBrush( vc.color[BG] ) )
	, widthTable_( new int[65536] )
{
	// ���䕶����`�悷�邩�ۂ��H�̃t���O���L��
	for( int i=0; i<countof(scDraw_); ++i )
		scDraw_[i] = vc.sc[i];

	// �����F���L��
	for( int i=0; i<countof(colorTable_); ++i )
		colorTable_[i] = vc.color[i];
		colorTable_[3] = vc.color[CMT];

	// DC�ɃZ�b�g
	::SelectObject( dc_, font_  );
	::SelectObject( dc_, pen_   );
	::SelectObject( dc_, brush_ );
	::SetBkMode(    dc_, TRANSPARENT );
	::SetMapMode(   dc_, MM_TEXT );

	// �������e�[�u���������iASCII�͈͂̕����ȊO�͒x�������j
	memFF( widthTable_, 65536*sizeof(int) );
#ifdef WIN32S
	::GetCharWidthA( dc_, ' ', '~', widthTable_+' ' );
#else
	::GetCharWidthW( dc_, L' ', L'~', widthTable_+L' ' );
#endif
	widthTable_[L'\t'] = W() * Max(1,vc.tabstep);
	// ���ʃT���Q�[�g�͕������[��
	mem00( widthTable_+0xDC00, (0xE000 - 0xDC00)*sizeof(int) );

	// �����̍ő啝���v�Z
	figWidth_ = 0;
	for( unicode ch=L'0'; ch<=L'9'; ++ch )
		if( figWidth_ < widthTable_[ch] )
			figWidth_ = widthTable_[ch];

	// �����̏��
	TEXTMETRIC met;
	::GetTextMetrics( dc_, &met );
	height_ = met.tmHeight;

	// LOGFONT
	::GetObject( font_, sizeof(LOGFONT), &logfont_ );
}

Painter::~Painter()
{
	// �K���ȕʃI�u�W�F�N�g���������Ď������������
	::SelectObject( dc_, ::GetStockObject( OEM_FIXED_FONT ) );
	::SelectObject( dc_, ::GetStockObject( BLACK_PEN ) );
	::SelectObject( dc_, ::GetStockObject( WHITE_BRUSH ) );
	::DeleteObject( font_ );
	::DeleteObject( pen_ );
	::DeleteObject( brush_ );
	delete [] widthTable_;
}

inline void Painter::CharOut( unicode ch, int x, int y )
{
#ifdef WIN32S
	DWORD dwNum;
	char *psText;
	if(dwNum = WideCharToMultiByte(CP_ACP,NULL,&ch,-1,NULL,0,NULL,FALSE))
	{
		psText = new char[dwNum];
		WideCharToMultiByte(CP_ACP,NULL,&ch,-1,psText,dwNum,NULL,FALSE);
		::TextOutA( dc_, x, y, psText, dwNum-1 );
		delete []psText;
	}
#else
	::TextOutW( dc_, x, y, &ch, 1 );
#endif
}

inline void Painter::StringOut
	( const unicode* str, int len, int x, int y )
{
#ifdef WIN32S
	DWORD dwNum;
	char *psText;
	if(dwNum = WideCharToMultiByte(CP_ACP,NULL,str,-1,NULL,0,NULL,FALSE))
	{
		psText = new char[dwNum];
		WideCharToMultiByte(CP_ACP,NULL,str,-1,psText,dwNum,NULL,FALSE);
		::TextOutA( dc_, x, y, psText, dwNum-1 );
		delete []psText;
	}
#else
	::TextOutW( dc_, x, y, str, len );
#endif
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
	const int w=Wc(0x3000/*L'�@'*/), h=H();
	POINT pt[4] = {
		{ x    , y+h-4 },
		{ x    , y+h-2 },
		{ x+w-3, y+h-2 },
		{ x+w-3, y+h-4 }
	};
	while( times-- )
	{
		if( 0 <= pt[3].x )
			::Polygon( dc_, pt, countof(pt) );
		pt[0].x += w;
		pt[1].x += w;
		pt[2].x += w;
		pt[3].x += w;
	}
}



//-------------------------------------------------------------------------
// �ĕ`�悵�����͈͂� Invalidate ����B
//-------------------------------------------------------------------------

void ViewImpl::ReDraw( ReDrawType r, const DPos* s )
{
	// �܂��X�N���[���o�[���X�V
	UpdateScrollBar();

	switch( r )
	{
	case ALL: // �S���

		::InvalidateRect( hwnd_, NULL, FALSE );
		break;

	case LNAREA: // �s�ԍ��\����̂�

		if( lna() > 0 )
		{
			RECT rc = { 0, 0, lna(), bottom() };
			::InvalidateRect( hwnd_, &rc, FALSE );
		}
		break;

	case LINE: // �w�肵���s�̌㔼
	case AFTER: // �w�肵���s�ȉ��S��

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
	// �`��͈͂̏����ڂ����擾
	Painter& p = cvs_.getPainter();
	VDrawInfo v( ps.rcPaint );
	GetDrawPosInfo( v );

	if( ps.rcPaint.right <= lna()  )
	{
		// case A: �s�ԍ��\����̂ݍX�V
		DrawLNA( v, p );
	}
	else if( lna() <= ps.rcPaint.left )
	{
		// case B: �e�L�X�g�\����̂ݍX�V
		DrawTXT( v, p );
	}
	else
	{
		// case C: �����X�V
		DrawLNA( v, p );
		p.SetClip( cvs_.zone() );
		DrawTXT( v, p );
		p.ClearClip();
	}
}



//-------------------------------------------------------------------------
// �s�ԍ��]�[���`��
//-------------------------------------------------------------------------

void ViewImpl::DrawLNA( const VDrawInfo& v, Painter& p )
{
	//
	// ������̂܂ܑ����Z���s�����[�`��
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

	// �w�ʏ���
	RECT rc = { v.rc.left, v.rc.top, lna(), v.rc.bottom };
	p.Fill( rc );

	if( v.rc.top < v.YMAX )
	{
		// ���E���\��
		int line = lna() - p.F()/2;
		p.DrawLine( line, v.rc.top, line, v.YMAX );
		p.SetColor( LN );

		// �s�ԍ��\��
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
// �e�L�X�g�`��
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
	// �萔�P
//	const int   TAB = p.T();
	const int     H = p.H();
	const ulong TLM = doc_.tln()-1;

	// ��Ɨp�ϐ��P
	RECT  a = { 0, v.YMIN, 0, v.YMIN+p.H() };
	int clr = -1;
	register int   x, x2;
	register ulong i, i2;

	// �_���s�P�ʂ�Loop
	for( ulong tl=v.TLMIN; a.top<v.YMAX; ++tl )
	{
		// �萔�Q
		const unicode* str = doc_.tl(tl);
		const uchar*   flg = doc_.pl(tl);
		const int rYMAX = Min<int>( v.YMAX, a.top+rln(tl)*H );

		// ��Ɨp�ϐ��Q
		ulong stt=0, end, t, n;

		// �\���s�P�ʂ�Loop
		for( ulong rl=0; a.top<rYMAX; ++rl,a.top+=H,a.bottom+=H,stt=end )
		{
			// ��Ɨp�ϐ��R
			end = rlend(tl,rl);
			if( a.bottom<=v.YMIN )
				continue;

			// �e�L�X�g�f�[�^�`��
			for( x2=x=0,i2=i=stt; x<=v.XMAX && i<end; x=x2,i=i2 )
			{
				// n := ����Token�̓�
				t = (flg[i]>>5);
				n = i + t;
				if( n >= end )
					n = end;
				else if( t==7 || t==0 )
					while( n<end && (flg[n]>>5)==0 )
						++n;

				// x2, i2 := ����Token�̉E�[
				i2 ++;
				x2 = (str[i]==L'\t' ? p.nextTab(x2) : x2+p.W(&str[i]));
			//	if( x2 <= v.XMIN )
			//		x=x2, i=i2;
				while( i2<n && x2<=v.XMAX )
					x2 += p.W( &str[i2++] );

				// �ĕ`�悷�ׂ��͈͂Əd�Ȃ��Ă��Ȃ�
				if( x2<=v.XMIN )
					continue;

				// x, i := ���̃g�[�N���̍��[
				if( x<v.XMIN )
				{
					// tab�̕����߂肷���H
					x = x2, i = i2;
					while( v.XMIN<x )
						x -= p.W( &str[--i] );
				}

				// �w�i�h��Ԃ�
				a.left  = x + v.XBASE;
				a.right = x2 + v.XBASE;
				p.Fill( a );

				// �`��
				switch( str[i] )
				{
				case L'\t':
					if( p.sc(scTAB) )
					{
						p.SetColor( clr=CTL );
						for( ; i<i2; ++i, x=p.nextTab(x) )
							p.CharOut( L'>', x+v.XBASE, a.top );
					}
					break;
				case L' ':
					if( p.sc(scHSP) )
						p.DrawHSP( x+v.XBASE, a.top, i2-i );
					break;
				case 0x3000://L'�@':
					if( p.sc(scZSP) )
						p.DrawZSP( x+v.XBASE, a.top, i2-i );
					break;
				default:
					if( clr != (flg[i]&3) )
						p.SetColor( clr=(flg[i]&3) );
					p.StringOut( str+i, i2-i, x+v.XBASE, a.top );
					//p.StringOut( str+i, i2-i, x+v.XBASE, a.top );
					// ���̂����Q�x�`�����Ȃ��Ƃ��܂�������c
					break;
				}
			}

			// �I��͈͂������甽�]
			if( v.SYB<=a.top && a.top<=v.SYE )
				Inv( a.top, a.top==v.SYB?v.SXB:(v.XBASE),
				            a.top==v.SYE?v.SXE:(v.XBASE+x), p );

			// �s�������̗]����w�i�F�h
			if( x<v.XMAX )
			{
				a.left = v.XBASE + Max( v.XMIN, x );
				a.right= v.XBASE + v.XMAX;
				p.Fill( a );
			}
		}

		// �s���L���`�攽�]
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

	// EOF��]����w�i�F�h
	if( a.top < v.rc.bottom )
	{
		a.left   = v.rc.left;
		a.right  = v.rc.right;
		a.bottom = v.rc.bottom;
		p.Fill( a );
	}
}
