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
	, wrapWidth_( 0xffffffff )
	, figNum_   ( 3 )
	, font_     ( new Painter( ::GetDC(vw.hwnd()),
	                  VConfig(TEXT("FixedSys"),14) ) )
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
	udScr_.nPage = cy / cvs_.getPainter().H() + 1;
	udScr_.nMax  = vln() + udScr_.nPage - 2;

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
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
	::SetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
	::SetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
#elif defined(TARGET_VER) && TARGET_VER<=350 && TARGET_VER>310
	if(app().isNewShell())
	{
		::SetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
		::SetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
	}
	else
	{
		::SetScrollRange( hwnd_, SB_HORZ, rlScr_.nMin, rlScr_.nMax, FALSE );
		::SetScrollPos( hwnd_, SB_HORZ, rlScr_.nPos, TRUE );
		::SetScrollRange( hwnd_, SB_VERT, udScr_.nMin, udScr_.nMax, FALSE );
		::SetScrollPos( hwnd_, SB_VERT, udScr_.nPos, TRUE );
	}
#else
	::SetScrollRange( hwnd_, SB_HORZ, rlScr_.nMin, rlScr_.nMax, FALSE );
	::SetScrollPos( hwnd_, SB_HORZ, rlScr_.nPos, TRUE );
	::SetScrollRange( hwnd_, SB_VERT, udScr_.nMin, udScr_.nMax, FALSE );
	::SetScrollPos( hwnd_, SB_VERT, udScr_.nPos, TRUE );
#endif
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
		int  top = v.rc.top / H;
		while( y + (signed)rln(tl) <= top )
			y += rln( tl++ );

		// �c���W
		v.YMIN  = y * H;
		v.YMAX  = Min( v.rc.bottom, most_under );
		v.TLMIN = tl;

		// �����W
		v.XBASE = left() - rlScr_.nPos;
		v.XMIN  = v.rc.left  - v.XBASE;
		v.XMAX  = v.rc.right - v.XBASE;

		// �I��͈�
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
	// �X�N���[���J�n�ʒm
	cur_.on_scroll_begin();

	const RECT* clip = (dy==0 ? &cvs_.zone() : NULL);
	const int H = cvs_.getPainter().H();

	// �X�N���[���o�[�X�V
	if( dx != 0 )
	{
		// �͈̓`�F�b�N
		if( rlScr_.nPos+dx < 0 )
			dx = -rlScr_.nPos;
		else if( rlScr_.nMax-(signed)rlScr_.nPage < rlScr_.nPos+dx ) 
			dx = rlScr_.nMax-rlScr_.nPage-rlScr_.nPos+1;

		rlScr_.nPos += dx;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		::SetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
#elif defined(TARGET_VER) && TARGET_VER<=350 && TARGET_VER>310
		if(app().isNewShell())
		{
			::SetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
		}
		else
		{
			::SetScrollPos( hwnd_, SB_HORZ, rlScr_.nPos, TRUE );
		}
#else
		::SetScrollPos( hwnd_, SB_HORZ, rlScr_.nPos, TRUE );
#endif
		dx = -dx;
	}
	if( dy != 0 )
	{
		// �͈̓`�F�b�N�c�͑O�����ŏI����Ă�B

		udScr_.nPos += dy;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		::SetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
#elif defined(TARGET_VER) && TARGET_VER<=350 && TARGET_VER>310
		if(app().isNewShell())
		{
			::SetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
		}
		else
		{
			::SetScrollPos( hwnd_, SB_VERT, udScr_.nPos, TRUE );
		}
#else
		::SetScrollPos( hwnd_, SB_VERT, udScr_.nPos, TRUE );
#endif
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
			::ScrollWindowEx( hwnd_, dx, dy, NULL, 
					clip, NULL, NULL, SW_INVALIDATE );

			// �����ĕ`��H
			if( update )
			{
				// �c�X�N���[���͍������������̂ň�H�v
				if( dy != 0 )
				{
					// �ĕ`��̕K�v�ȗ̈�������Ōv�Z
					RECT rc = {0,0,right(),bottom()};
					if( dy < 0 ) rc.top  = rc.bottom + dy;
					else         rc.bottom = dy;

					// �C���e���}�E�X�̒��{�^���N���b�N�ɂ��
					// �I�[�g�X�N���[���p�J�[�\���̉��̕������ɕ`��
					// �Q��ɕ����邱�ƂŁA�����ȋ�`������ōςނ̂ō���
					::ValidateRect( hwnd_, &rc );
					::UpdateWindow( hwnd_ );
					::InvalidateRect( hwnd_, &rc, FALSE );
				}
				::UpdateWindow( hwnd_ );
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
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		{
			SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			::GetScrollInfo( hwnd_, SB_HORZ, &si );
			dx = si.nTrackPos - rlScr_.nPos;
			break;
		}
#else
		dx = pos - rlScr_.nPos; break;
#endif
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
	case SB_PAGEUP:   dy= -(cy() / cvs_.getPainter().H()); break;
	case SB_PAGEDOWN: dy= +(cy() / cvs_.getPainter().H()); break;
	case SB_THUMBTRACK:
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		{
			SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			::GetScrollInfo( hwnd_, SB_VERT, &si );
			dy = si.nTrackPos - udScr_.nPos;
			break;
		}
#else
		dy = pos - udScr_.nPos; break;
#endif
	case SB_TOP:      dy = -udScr_.nPos; break;
	case SB_BOTTOM:   dy = udScr_.nMax+1-(signed)udScr_.nPage-udScr_.nPos; break;
	}

	// �X�N���[��
	UpDown( dy, code==SB_THUMBTRACK );
}

void ViewImpl::on_wheel( short delta )
{
	// �X�N���[��
	UpDown( -delta / WHEEL_DELTA * 3, false );
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
	for( int t=udScr_tl_, ybe=cy()/H; (unsigned)t<dp.tl; yb+=rln(t++) )
		if( yb >= ybe )
			return;
	for( ; dp.ad>rlend(dp.tl,r); ++r,++yb );
	yb = H * Max( yb, -100 ); // ��ɂ͂ݏo���߂��Ȃ��悤����
	if( yb >= cy() )
		return;

	// �P�s�ڂ��ĕ`��
	int rb = (r==0 ? 0 : rlend(dp.tl,r-1));
	int xb = left() + Max( 0UL,
		CalcLineWidth(doc_.tl(dp.tl)+rb,dp.ad-rb) -rlScr_.nPos );
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
