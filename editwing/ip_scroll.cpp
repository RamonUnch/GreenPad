#include "stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_scroll.cpp �X�N���[��
//
//		�E�C���h�E�T�C�Y�̓X�N���[���o�[�̈ʒu�ɂ����
//		�`��ʒu��K���ɍX�V���Ă��������������B
//
//---- ip_text.cpp   �����񑀍�E��
//---- ip_parse.cpp  �L�[���[�h���
//---- ip_wrap.cpp   �܂�Ԃ�
//---- ip_draw.cpp   �`��E��
//---- ip_cursor.cpp �J�[�\���R���g���[��
//=========================================================================

#if defined(TARGET_VER) && TARGET_VER<351 && TARGET_VER>=305
typedef int (WINAPI *ssnfo_funk)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw);
static int MySetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw)
{
	// Should be supported since Windows NT 3.51.944
	static ssnfo_funk dyn_SetScrollInfo = (ssnfo_funk)(-1);
	if( dyn_SetScrollInfo == (ssnfo_funk)(-1) ) {
		dyn_SetScrollInfo = (ssnfo_funk)GetProcAddress(GetModuleHandleA("USER32.DLL"), "SetScrollInfo");
		if(!(  (!App::isNT() && App::getOSVer()>=400 && App::getOSBuild()>=275) // Win95 4.00.275
			|| ( App::isNT() && App::getOSVer()>=351 && App::getOSBuild()>=944))// WinNT 3.51.944
		    )
		{   // Not supported before 95 build 275
			// Nor NT3.51 before build 944
			dyn_SetScrollInfo = NULL;
		}
	}
	if( dyn_SetScrollInfo )
		return dyn_SetScrollInfo( hwnd, fnBar, lpsi, fredraw );

	// Smart Fallback...
	// We must use SetScrollRange but it is mimited to 65535.
	// So we can avoid oveflow by dividing range and position values
	// In GreenPad we can assume that only nMax can go beyond range.
	int MULT=1;
	int nMax = lpsi->nMax;
	// If we go beyond 65400 then we use a divider
	if (nMax > 65400)
	{   // 65501 - 65535 = 35 values MULT from 2-136
		// Like this the new range is around 8912896 instead of 65535
		MULT = Min( nMax / 65536 + 1,  136 );
		// We store the divider in the last 135 values
		// of the max scroll range.
		nMax = 65400 + MULT - 1 ; // 65401 => MULT = 2
	}

	if (lpsi->fMask|SIF_RANGE)
		::SetScrollRange( hwnd, fnBar, lpsi->nMin, nMax, FALSE );

	return ::SetScrollPos( hwnd, fnBar, lpsi->nPos/MULT, fredraw );
}
typedef int (WINAPI *gsnfo_funk)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
static int MyGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
	// Should be supported since Windows NT 3.51...
	static gsnfo_funk dyn_GetScrollInfo = (gsnfo_funk)(-1);
	if( dyn_GetScrollInfo == (gsnfo_funk)(-1) ) {
		dyn_GetScrollInfo = (gsnfo_funk)GetProcAddress(GetModuleHandleA("USER32.DLL"), "GetScrollInfo");
		if(!(  (!App::isNT() && App::getOSVer()>=400 && App::getOSBuild()>=275) // Win95 4.00.275
			|| ( App::isNT() && App::getOSVer()>=351 && App::getOSBuild()>=944))// WinNT 3.51.944
		    )
		{
			dyn_GetScrollInfo = NULL;
		}
	}
	if( dyn_GetScrollInfo )
		return dyn_GetScrollInfo( hwnd, fnBar, lpsi );

	// Smart Fallback...
	if( lpsi->fMask|SIF_RANGE )
		::GetScrollRange( hwnd, fnBar, &lpsi->nMin, &lpsi->nMax);
	lpsi->nPos = ::GetScrollPos( hwnd, fnBar );
	// Scroll range indicates a multiplier was used...
	if( lpsi->nMax > 65400 )
	{ // Apply multipler
		int MULT = lpsi->nMax - 65400 + 1; // 65401 => MULT = 2
		lpsi->nMax      *= MULT;
		lpsi->nPos      *= MULT;
		lpsi->nTrackPos *= MULT; // This own has to be set by the user.
	}
	return 1; // sucess!
}
#else
#define MyGetScrollInfo GetScrollInfo
#define MySetScrollInfo SetScrollInfo
#endif //305<=TARGET_VER<NT3.51

//-------------------------------------------------------------------------
// �`��̈�T�C�Y�Ǘ�
//-------------------------------------------------------------------------

namespace
{
	static int Log10( ulong n )
	{
		const static ulong power_of_ten[] =
			{ 1, 10, 100, 1000, 10000, 100000, 1000000,
			  10000000, 100000000, 1000000000 }; // 10^0 �` 10^9
		int c=3;
		if( power_of_ten[9] <= n )
			c=10;
		else
			while( power_of_ten[c] <= n )
				c++;
		return c; // 3<=c<=10 s.t. 10^(c-1) <= n < 10^c
	}
}

bool Canvas::CalcLNAreaWidth()
{
	const int prev = txtZone_.left;
	if( showLN_ )
	{
		txtZone_.left  = (1 + figNum_) * font_->F();
		if( txtZone_.left+font_->W() >= txtZone_.right )
			txtZone_.left = 0; // �s�ԍ��]�[�����f�J������Ƃ��͕\�����Ȃ�
	}
	else
	{
		txtZone_.left = 0;
	}

	return (prev != txtZone_.left);
}

void Canvas::CalcWrapWidth()
{
	switch( wrapType_ )
	{
	case NOWRAP:
		wrapWidth_ = 0xffffffff;
		break;
	case RIGHTEDGE:
		wrapWidth_ = txtZone_.right - txtZone_.left - 3;
		break; //Caret�̕�-3�␳
	default:
		wrapWidth_ = wrapType_ * font_->W();
		break;
	}
}

Canvas::Canvas( const View& vw )
	: wrapType_ ( -1 )
	, showLN_   ( false )
	, font_     ( new Painter( ::GetDC(vw.hwnd()),
	              VConfig(TEXT("FixedSys"),14) ) )
	, wrapWidth_( 0xffffffff )
	, figNum_   ( 3 )
{
	vw.getClientRect( &txtZone_ );
}

bool Canvas::on_view_resize( int cx, int cy )
{
	txtZone_.right  = cx;
	txtZone_.bottom = cy;

	CalcLNAreaWidth();
	if( wrapType_ == RIGHTEDGE )
	{
		CalcWrapWidth();
		return true;
	}
	return false;
}

void Canvas::on_font_change( const VConfig& vc )
{
	HDC dc = font_->getDC();
	font_ = NULL; // ��Ƀf�X�g���N�^���Ă΂˂΂Ȃ�Ȃ��c
	              // ���Ă���[�i�D�����[�[�[(T_T)
	font_ = new Painter( dc, vc );

	CalcLNAreaWidth();
	CalcWrapWidth();
}

void Canvas::on_config_change( int wrap, bool showln )
{
	showLN_ = showln;
	wrapType_ = wrap;

	CalcLNAreaWidth();
	CalcWrapWidth();
}

bool Canvas::on_tln_change( ulong tln )
{
	figNum_ = Log10( tln ); // �����v�Z

	if( CalcLNAreaWidth() )
	{
		if( wrapType_ == RIGHTEDGE )
			CalcWrapWidth();
		return true;
	}
	return false;
}



//-------------------------------------------------------------------------
// �X�N���[���o�[�v�Z���[�`��
//-------------------------------------------------------------------------
// rl (���X�N���[�����)
// max:  view.txt.txtwidth()
// page: view.cx()
// pos:  0�`max-page

// ud (�c�X�N���[�����)
// max:   view.txt.vln() + page - 1
// page:  view.cy() / view.fnt.H()
// delta: 0�`view.fnt.H()
// pos:   0�`max-page (top�̍s�ԍ�)

bool ViewImpl::ReSetScrollInfo()
{
	const int prevRlPos = rlScr_.nPos;
	const ulong cx = cvs_.zone().right - cvs_.zone().left;
	const ulong cy = cvs_.zone().bottom;

	// ���͕ςȒl�ɂȂ�Ȃ��悤�␳���邾���ł悢
//	rlScr_.nPage = cx + 1;
//	rlScr_.nMax  = Max( textCx_, cx );
//	rlScr_.nPos  = Min<int>( rlScr_.nPos, rlScr_.nMax-rlScr_.nPage+1 );
	rlScr_.nPage = cx + 1;
	rlScr_.nMax  = Max( textCx_+3, cx );
	rlScr_.nPos  = Min<int>( rlScr_.nPos, rlScr_.nMax-rlScr_.nPage+1 );

	// �c��nPage��nMax�͂Ƃ肠�����␳
	// nPos�͏ꍇ�ɂ���Ē��������قȂ�̂ŕʃ��[�`���ɂ�
	udScr_.nPage = cy / NZero(cvs_.getPainter().H()) + 1;
	// Adjust so that scroll bar appears only when we are shy oneline from the page.
	udScr_.nMax  = vln() + udScr_.nPage - Max( 2, Min<int>( udScr_.nPage-1, doc_.tln()+1 ) );
	//udScr_.nMax  = vln() + udScr_.nPage - 2; // Old code (not so nice)

	// ���X�N���[�����N������true
	return (prevRlPos != rlScr_.nPos);
}

void ViewImpl::ForceScrollTo( ulong tl )
{
	udScr_.nPos = tl2vl(tl);
	udScr_tl_   = tl;
	udScr_vrl_  = 0;
}

ulong ViewImpl::tl2vl( ulong tl ) const
{
	if( vln() == doc_.tln() )
		return tl;

	ulong vl=0;
	for( ulong i=0; i<tl; ++i )
		vl += rln(i);
	return vl;
}

void ViewImpl::UpdateScrollBar()
{
	::MySetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
	::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
}

ReDrawType ViewImpl::TextUpdate_ScrollBar
	( const DPos& s, const DPos& e, const DPos& e2 )
{
	const ulong prevUdMax = udScr_.nMax;
	const bool rlScrolled = ReSetScrollInfo();
	const long vl_dif = (udScr_.nMax - prevUdMax);
	ReDrawType ans =
		(vl_dif!=0 || s.tl!=e2.tl ? AFTER : LINE);

	if( udScr_tl_ < s.tl )
	{
		// �p�^�[���P�F���݂̉�ʏ�[��艺�ōX�V���ꂽ�ꍇ
		// �X�N���[�����Ȃ�
	}
	else if( udScr_tl_ == s.tl )
	{
		// �p�^�[���Q�F���݂̉�ʏ�[�Ɠ����s�ōX�V���ꂽ�ꍇ
		// �o���邾�������ʒu��\���������悤�Ǝ��݂�B

		if( static_cast<ulong>(udScr_.nPos) >= vln() )
		{
			// �p�^�[��2-1�F�����������͂��ł�EOF���������I
			// ����[�Ȃ��̂ň�ԉ��̍s��\��
			udScr_.nPos = vln()-1;
			udScr_tl_   = doc_.tln()-1;
			udScr_vrl_  = rln(udScr_tl_)-1;
			ans = ALL;
		}
		else
		{
			// �p�^�[��2-2�F
			// �X�N���[������
			while( udScr_vrl_ >= rln(udScr_tl_) )
			{
				udScr_vrl_ -= rln(udScr_tl_);
				udScr_tl_++;
			}
		}
	}
	else
	{
		// �p�^�[���R�F���݂̉�ʏ�[����ōX�V���ꂽ�ꍇ
		// �\�����e��ς��Ȃ��悤�Ɋ撣��

		if( e.tl < udScr_tl_ )
		{
			// �p�^�[��3-1�F�ύX�͈͂̏I�[���A���ݍs����̏ꍇ
			// �s�ԍ��͕ς�邪�\�����e�͕ς��Ȃ��ōς�
			udScr_.nPos += vl_dif;
			udScr_tl_   += (e2.tl - e.tl);
			ans = LNAREA;
		}
		else
		{
			// �p�^�[��3-2�F
			// �ǂ�����[���Ȃ��̂œK���Ȉʒu�ɃX�N���[��
			ForceScrollTo( e2.tl );
			ans = ALL;
		}
	}

	// �ǂ�ȍĕ`�������΂悢���Ԃ�
	return (rlScrolled ? ALL : ans);
}

void ViewImpl::ScrollTo( const VPos& vp )
{
	// ���t�H�[�J�X
	int dx=0;
	if( vp.vx < (signed)rlScr_.nPos )
	{
		dx = vp.vx - rlScr_.nPos;
	}
	else
	{
		const int W = cvs_.getPainter().W();
		if( rlScr_.nPos + (signed)(rlScr_.nPage-W) <= vp.vx )
			dx = vp.vx - (rlScr_.nPos + rlScr_.nPage) + W;
	}

	// �c�t�H�[�J�X
	int dy=0;
	if( vp.vl < (unsigned)udScr_.nPos )
		dy = vp.vl - udScr_.nPos;
	else if( udScr_.nPos + (udScr_.nPage-1) <= vp.vl )
		dy = vp.vl - (udScr_.nPos + udScr_.nPage) + 2;

	// �X�N���[��
	if( dy!=0 )	UpDown( dy, dx==0 );
	if( dx!=0 )	ScrollView( dx, 0, true );
}

void ViewImpl::GetDrawPosInfo( VDrawInfo& v ) const
{
	const int H = cvs_.getPainter().H();

	long most_under = (vln()-udScr_.nPos)*H;
	if( most_under <= v.rc.top )
	{
		v.YMIN  = v.rc.top;
		v.YMAX  = most_under;
	}
	else
	{
		int    y = -(signed)udScr_vrl_;
		ulong tl = udScr_tl_;
		int  top = v.rc.top / NZero(H);
		while( y + (signed)rln(tl) <= top )
			y += rln( tl++ );

		// �c���W, Vertical coordinates
		v.YMIN  = y * H;
		v.YMAX  = Min( v.rc.bottom, most_under );
		v.TLMIN = tl;

		// �����W, Horizontal coordinates
		v.XBASE = left() - rlScr_.nPos;
		v.XMIN  = v.rc.left  - v.XBASE;
		v.XMAX  = v.rc.right - v.XBASE;

		// �I��͈�, Selection range
		v.SXB = v.SXE = v.SYB = v.SYE = 0x7fffffff;

		const VPos *bg, *ed;
		if( cur_.getCurPos( &bg, &ed ) )
		{
			v.SXB = bg->vx - rlScr_.nPos + left();
			v.SXE = ed->vx - rlScr_.nPos + left();
			v.SYB = (bg->vl - udScr_.nPos) * H;
			v.SYE = (ed->vl - udScr_.nPos) * H;
		}
	}
}

void ViewImpl::ScrollView( int dx, int dy, bool update )
{
	// �X�N���[���J�n�ʒm, Scroll notification start
	cur_.on_scroll_begin();

	const RECT* clip = (dy==0 ? &cvs_.zone() : NULL);
	const int H = cvs_.getPainter().H();

	// �X�N���[���o�[�X�V, Scrollbar updated
	if( dx != 0 )
	{
		// �͈̓`�F�b�N, range check
		if( rlScr_.nPos+dx < 0 )
			dx = -rlScr_.nPos;
		else if( rlScr_.nMax-(signed)rlScr_.nPage < rlScr_.nPos+dx )
			dx = rlScr_.nMax-rlScr_.nPage-rlScr_.nPos+1;

		rlScr_.nPos += dx;
		::MySetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
		dx = -dx;
	}
	if( dy != 0 )
	{
		// �͈̓`�F�b�N�c�͑O�����ŏI����Ă�B
		udScr_.nPos += dy;
		::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
		dy *= -H;
	}
	if( dx!=0 || dy!=0 )
	{
		if( -dx>=right() || dx>=right()
		 || -dy>=bottom() || dy>=bottom() )
		{
			// �S��ʍĕ`��
			// ���傤��65536�̔{�����炢�X�N���[�������Ƃ��ɁA
			// ScrollWindowEx on Win9x ���ƍĕ`�悪�ςȂ̂�����B
			::InvalidateRect( hwnd_, NULL, FALSE );
		}
		else
		{
			// �ĕ`��̕s�v�ȗ̈���X�N���[��
			// Scroll through areas that do not need redrawing
			::ScrollWindowEx( hwnd_, dx, dy, NULL,
					clip, NULL, NULL, SW_INVALIDATE );

			// �����ĕ`��H, Immediate redraw?
			if( update )
			{
				// �c�X�N���[���͍������������̂ň�H�v, Vertical scrolling is one way to speed up the process.
				if( dy != 0 )
				{
					// �ĕ`��̕K�v�ȗ̈�������Ōv�Z
					// Calculate the area that needs to be redrawn
					RECT rc = { 0, 0, right(), bottom() };
					if( dy < 0 ) rc.top  = rc.bottom + dy;
					else         rc.bottom = dy;

					// �C���e���}�E�X�̒��{�^���N���b�N�ɂ��
					// �I�[�g�X�N���[���p�J�[�\���̉��̕������ɕ`��
					// �Q��ɕ����邱�ƂŁA�����ȋ�`������ōςނ̂ō���
					// By clicking the middle button of the IntelliMouse
					// Draw the area under the cursor for auto scrolling first.
					// By splitting it into two parts,
					// only two small rectangles are needed, which is faster.
					::ValidateRect( hwnd_, &rc );
					//::UpdateWindow( hwnd_ );
					::InvalidateRect( hwnd_, &rc, FALSE );
				}
				// RAMON: I much prefer to have a slightly sloppy drawing
				// on slow computers rather than have my scroll messages pile up
				// and keep scrolling the window when I stopped scrolling.
				// Reactivity is more important than perfect drawing.
				// This is why I commented out the ::UpdateWindow() calls
				//::UpdateWindow( hwnd_ );
			}
		}
	}

	// �X�N���[���I���ʒm
	cur_.on_scroll_end();
}

void ViewImpl::on_hscroll( int code, int pos )
{
	// �ω��ʂ��v�Z
	int dx;
	switch( code )
	{
	default:           return;
	case SB_LINELEFT:  dx= -cvs_.getPainter().W(); break;
	case SB_LINERIGHT: dx= +cvs_.getPainter().W(); break;
	case SB_PAGELEFT:  dx= -(cx()>>1); break;
	case SB_PAGERIGHT: dx= +(cx()>>1); break;
	case SB_THUMBTRACK:
	{
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
		si.nTrackPos = pos;
		::MyGetScrollInfo( hwnd_, SB_HORZ, &si );
		dx = si.nTrackPos - rlScr_.nPos;
		break;
	}
	case SB_LEFT:    dx = -rlScr_.nPos; break;
	case SB_RIGHT:   dx = rlScr_.nMax+1-(signed)rlScr_.nPage-rlScr_.nPos; break;
	}

	// �X�N���[��
	ScrollView( dx, 0, code!=SB_THUMBTRACK );
}

void ViewImpl::on_vscroll( int code, int pos )
{
	// �ω��ʂ��v�Z
	int dy;
	switch( code )
	{
	default:          return;
	case SB_LINEUP:   dy= -1; break;
	case SB_LINEDOWN: dy= +1; break;
	case SB_PAGEUP:   dy= -( cy() / NZero(cvs_.getPainter().H()) ); break;
	case SB_PAGEDOWN: dy= +( cy() / NZero(cvs_.getPainter().H()) ); break;
	case SB_THUMBTRACK:
	{
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
		si.nTrackPos = pos; // in case
		::MyGetScrollInfo( hwnd_, SB_VERT, &si );
		dy = si.nTrackPos - udScr_.nPos;
		break;
	}
	case SB_TOP:      dy = -udScr_.nPos; break;
	case SB_BOTTOM:   dy = udScr_.nMax+1-(signed)udScr_.nPage-udScr_.nPos; break;
	}

	// �X�N���[��
	UpDown( dy, code==SB_THUMBTRACK );
}

int ViewImpl::getNumScrollLines( void )
{
	uint scrolllines = 3; // Number of lines to scroll (default 3).
	if( App::getOSVer() >= 400 )
	{   // Read the system value for the wheel scroll lines.
		UINT numlines;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &numlines, 0 ) )
			scrolllines = numlines; // Sucess!
	}
	// If the number of lines is larger than a single page then we only scroll a page.
	// This automatically takes into account the page scroll mode where
	// SPI_GETWHEELSCROLLLINES value if 0xFFFFFFFF.
	uint nlpage;
	nlpage = Max( (uint)cy() / (uint)NZero(cvs_.getPainter().H()), 1U );
	// scrolllines can be zero, in this case no scroll should occur.
	return (int)Min( scrolllines, nlpage );
}

void ViewImpl::on_wheel( short delta )
{
	// �X�N���[��
	int nl = getNumScrollLines();
	int step = (-(int)delta * nl) / WHEEL_DELTA;

	if( step == 0 )
	{ // step is too small, we need to accumulate delta
		accdelta_ += delta;
		// Recalculate the step.
		step = (-(int)accdelta_ * nl) / WHEEL_DELTA;
		if( step )
		{
			UpDown( step, false );
			// set accumulator to the remainder.
			accdelta_ -= (-step * WHEEL_DELTA) / nl;
		}
	}
	else
	{
		UpDown( step, false );
	}
}
int ViewImpl::getNumScrollRaws( void )
{
	uint scrollnum = 3; // Number of lines to scroll (default 3).
	// TODO: Get more accurate version numbers for scroll wheel support?
	if ( App::getOSVer() >= 600 )
	{ // Introduced in window Vista
		UINT num;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLCHARS, 0, &num, 0 ) )
			scrollnum = num; // Sucess!
	}
	else if( App::getOSVer() >= 400 )
	{   // Read the system value for the wheel scroll lines.
		UINT num;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &num, 0 ) )
			scrollnum = num; // Sucess!
	}
	// Avoid scrolling more that halfa screen horizontally.
	uint nlpage;
	nlpage = Max( (uint)cx() / (uint)NZero(cvs_.getPainter().W()/2), 1U );
	// scrollnum can be zero, in this case no scroll should occur.
	return (int)Min( scrollnum, nlpage );
}

void ViewImpl::on_hwheel( short delta )
{
	int nl = getNumScrollRaws();
	int dx = nl * cvs_.getPainter().W() * (int)delta / WHEEL_DELTA;
	if( dx == 0 )
	{ // step is too small, we need to accumulate delta
		accdeltax_ += delta;
		// Recalculate the step.
		dx = nl * cvs_.getPainter().W() * (int)accdeltax_ / WHEEL_DELTA;
		if( dx )
		{
			ScrollView( dx, 0, 0 );
			accdeltax_ = 0;
		}
	}
	else
	{
		ScrollView( dx, 0, 0 );
	}
}

void ViewImpl::UpDown( int dy, bool thumb )
{
  // �P�DudScr_.nPos + dy ������͈͂Ɏ��܂�悤�ɕ␳
	if( udScr_.nPos+dy < 0 )
		dy = -udScr_.nPos;
	else if( udScr_.nMax+1-(signed)udScr_.nPage < udScr_.nPos+dy )
		dy = udScr_.nMax+1-udScr_.nPage-udScr_.nPos;
	if( dy==0 )
		return;

  // �Q�|�P�D�܂�Ԃ������̏ꍇ�͈�C�ɃW�����v�o����
	if( !wrapexists() )
	{
		udScr_tl_ = udScr_.nPos + dy;
	}

  // �Q�|�Q�D�łȂ����A���݈ʒu����̑��΃T�[�`
  // ScrollBar��A���I�Ƀh���b�O������x�Ɉ�C�ɔ�񂾏ꍇ��
  // 1�s�ڂ�ŏI�s����̑��΃T�[�`�̕����L���ȉ\�������邪�A
  // ���̏ꍇ�͑������x���x���Ȃ��Ă��`�悪���������邱�Ƃ͂Ȃ��̂łn�j
	else
	{
		int   rl = dy + udScr_vrl_;
		ulong tl = udScr_tl_;

		if( dy<0 ) // ��֖߂�ꍇ
		{
			// �W�����v��_���s�̍s����Dash!
			while( rl < 0 )
				rl += rln(--tl);
		}
		else if( dy>0 ) // ���֐i�ޏꍇ
		{
			// �W�����v��_���s�̍s����Dash!
			while( rl > 0 )
				rl -= rln(tl++);
			if( rl < 0 )
				rl += rln(--tl); //�s���߂��C��
		}
		udScr_tl_ = tl;
		udScr_vrl_= static_cast<ulong>(rl);
	}

  // �S�D��ʂ��X�N���[��
	ScrollView( 0, dy, !thumb );
}

void ViewImpl::InvalidateView( const DPos& dp, bool afterall ) const
{
	const int H = cvs_.getPainter().H();

	// �\�������ł̍X�V
	if( dp.tl < udScr_tl_ )
	{
		if( afterall )
			::InvalidateRect( hwnd_, NULL, FALSE );
		return;
	}

	// �J�ny���W�v�Z
	int r=0, yb=-(signed)udScr_vrl_;
	for( int t=udScr_tl_, ybe=cy() / NZero(H); (unsigned)t<dp.tl; yb+=rln(t++) )
		if( yb >= ybe )
			return;
	for( ; dp.ad>rlend(dp.tl,r); ++r,++yb );
	yb = H * Max( yb, -100 ); // ��ɂ͂ݏo���߂��Ȃ��悤����
	if( yb >= cy() )
		return;

	// �P�s�ڂ��ĕ`��
	int rb = (r==0 ? 0 : rlend(dp.tl,r-1));
 	int xb = left() + Max( (ulong)0,
		CalcLineWidth(doc_.tl(dp.tl)+rb, (ulong) (dp.ad-rb)) - rlScr_.nPos );
	if( xb < right() )
	{
		RECT rc={xb,yb,right(),yb+H};
		::InvalidateRect( hwnd_, &rc, FALSE );
	}

	// �c��
	int ye;
	yb += H;
	if( afterall )
		xb=0, ye=cy();
	else
		xb=left(), ye=Min(cy(),yb+(int)(H*(rln(dp.tl)-r-1)));
	if( yb < ye )
	{
		RECT rc={xb,yb,right(),ye};
		::InvalidateRect( hwnd_, &rc, FALSE );
	}
}
