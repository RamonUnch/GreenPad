#include "stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_wrap.cpp   �܂�Ԃ�
//
//		Document�ŕ�����f�[�^���X�V�����̂��󂯂�
//		View�ł͐܂�Ԃ��ʒu�����X�V����B���̏������R�R�B
//
//---- ip_text.cpp   �����񑀍�E��
//---- ip_parse.cpp  �L�[���[�h���
//---- ip_scroll.cpp �X�N���[��
//---- ip_draw.cpp   �`��E��
//---- ip_cursor.cpp �J�[�\���R���g���[��
//=========================================================================



//-------------------------------------------------------------------------
// ������
//-------------------------------------------------------------------------

ViewImpl::ViewImpl( View& vw, DocImpl& dc )
	: doc_   ( dc )
	, cvs_   ( vw )
	, cur_   ( vw.hwnd(), *this, dc )
	, hwnd_  ( vw.hwnd() )
	, vlNum_ ( 0 )
	, textCx_( 0 )
{
	// �K���ɐ܂�Ԃ���񏉊���
	InsertMulti( 0, doc_.tln()-1 );

	// �K���ɃX�N���[����񏉊���
	udScr_.cbSize = rlScr_.cbSize = sizeof(udScr_);
	udScr_.fMask  = rlScr_.fMask  = SIF_PAGE | SIF_POS | SIF_RANGE;
	udScr_.nMin   = rlScr_.nMin   = 0;
	udScr_.nPos   = rlScr_.nPos   = 0;
	udScr_.fMask |= SIF_DISABLENOSCROLL;
	udScr_tl_     = udScr_vrl_    = 0;
	ReSetScrollInfo();
}



//-------------------------------------------------------------------------
// ��ԕύX�ւ̑Ή�
//-------------------------------------------------------------------------

void ViewImpl::DoResize( bool wrapWidthChanged )
{
	// �܂�Ԃ��ʒu�Čv�Z
	if( wrapWidthChanged )
	{
		ReWrapAll();
		UpdateTextCx();
	}

	// �X�N���[�����ύX
	ReSetScrollInfo();
	if( wrapWidthChanged )
		ForceScrollTo( udScr_tl_ );

	// �ĕ`��
	ReDraw( ALL );
	cur_.ResetPos();
}

void ViewImpl::DoConfigChange()
{
	// �܂�Ԃ��ʒu�Čv�Z
	ReWrapAll();
	UpdateTextCx();

	// �X�N���[�����ύX
	ReSetScrollInfo();
	ForceScrollTo( udScr_tl_ );

	// �ĕ`��
	ReDraw( ALL );
	cur_.ResetPos();
}

void ViewImpl::on_text_update
	( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur )
{
	// �܂��A�܂�Ԃ��ʒu�Čv�Z

	// �u���͈͂̐擪�s�𒲐�
	int r3 = 0, r2 = 1, r1 = ReWrapSingle( s );

	// �c��𒲐�
	if( s.tl != e.tl )
		r2 = DeleteMulti( s.tl+1,  e.tl );
	if( s.tl != e2.tl )
		r3 = InsertMulti( s.tl+1, e2.tl );

	// ���̕ύX�ŉ������c
	// if( "�����Ȃ����ȂĂ͂��Ȃ�" AND "�Z���Ȃ���������\������" )
	//     �����Čv�Z();
	if( !(r1==2 || r3==1) && (r1==0 || r2==0) )
		UpdateTextCx();

	// �X�N���[���o�[�C��
	ReDrawType t = TextUpdate_ScrollBar( s, e, e2 );
	bool doResize = false;

	// �s���ɕω��������āA�s�ԍ��\����̕���ς��Ȃ���Ȃ��
	if( e.tl!=e2.tl && cvs_.on_tln_change( doc_.tln() ) )
	{
		doResize = true;
	}
	else if( bAft && t!=ALL )
	{
		t = AFTER;
	}

	// �J�[�\���ړ�
	cur_.on_text_update( s, e, e2, mCur );

	// �ĕ`��
	if( doResize )
		DoResize( true );
	else
	{
		if( e.tl != e2.tl ) // �s�ԍ��̈�ĕ`��̕K�v������Ƃ�
			ReDraw( LNAREA, 0 );
		ReDraw( t, &s );
	}
}



//-------------------------------------------------------------------------
// �܂�Ԃ��ʒu�v�Z�⏕���[�`��
//-------------------------------------------------------------------------

void ViewImpl::UpdateTextCx()
{
	if( cvs_.wrapType() == NOWRAP )
	{
		// �܂�Ԃ��Ȃ��Ȃ�A�����Ă݂Ȃ��Ɖ����͂킩��Ȃ�
		ulong cx=0;
		for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
			if( cx < wrap_[i].width() )
				cx = wrap_[i].width();
		textCx_ = cx;
	}
	else
	{
		// �܂�Ԃ�����Ȃ�A����:=�܂�Ԃ����Ƃ���
		textCx_ = cvs_.wrapWidth();
	}
}

ulong ViewImpl::CalcLineWidth( const unicode* txt, ulong len ) const
{
	// �s��܂�Ԃ����ɏ������Ƃ��̉������v�Z����
	// �قƂ�ǂ̍s���܂�Ԃ������ŕ\�������e�L�X�g�̏ꍇ�A
	// ���̒l���v�Z���Ă������ƂŁA�����̍��������\�B
	const Painter& p = cvs_.getPainter();

	ulong w=0;
	for( ulong i=0; i<len; ++i )
		if( txt[i] == L'\t' )
			w = p.nextTab(w);
		else
			w += p.W( &txt[i] );
	return w;
}

void ViewImpl::CalcEveryLineWidth()
{
	// �S�Ă̍s�ɑ΂���CalcLineWidth�����s
	// �c���邾���B
	for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
		wrap_[i].width() = CalcLineWidth( doc_.tl(i), doc_.len(i) );
}

void ViewImpl::ModifyWrapInfo(
		const unicode* txt, ulong len, WLine& wl, ulong stt )
{
	// �ݒ蕝�ł̐܂�Ԃ������s����B
	// �s�̓r������̕ύX�̏ꍇ�Astt���J�naddress���w���Ă���
	const Painter& p = cvs_.getPainter();
	const ulong   ww = cvs_.wrapWidth();

	while( stt < len )
	{
		ulong i, w;
		for( w=0,i=stt; i<len; ++i )
		{
			if( txt[i] == L'\t' )
				w = p.nextTab(w);
			else
				w += p.W( &txt[i] );

			if( w>ww )
				break; // �����ݒ�l�𒴂������ł����܂�
		}
		wl.Add( stt = (i==stt?i+1:i) );
	}
}

int ViewImpl::GetLastWidth( ulong tl ) const
{
	if( rln(tl)==1 )
		return wrap_[tl][0];

	ulong beg = rlend(tl,rln(tl)-2);
	return CalcLineWidth( doc_.tl(tl)+beg, doc_.len(tl)-beg );
}

void ViewImpl::ReWrapAll()
{
	// �܂�Ԃ����ɕύX���������ꍇ�ɁA�S�Ă̍s��
	// �܂�Ԃ��ʒu����ύX����B
	const ulong ww = cvs_.wrapWidth();

	ulong vln=0;
	for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
	{
		WLine& wl = wrap_[i];
		wl.ForceSize(1);

		if( wl.width() < ww )
		{
			// �ݒ肵���܂�Ԃ������Z���ꍇ�͈�s�ōςށB
			wl.Add( doc_.len(i) );
			++vln;
		}
		else
		{
			// �����s�ɂȂ�ꍇ
			ModifyWrapInfo( doc_.tl(i), doc_.len(i), wl, 0 );
			vln += wl.rln();
		}
	}
	vlNum_ = vln;
}



//-------------------------------------------------------------------------
// �܂�Ԃ��ʒu�v�Z���C�����[�`��
//-------------------------------------------------------------------------

int ViewImpl::ReWrapSingle( const DPos& s )
{
	// �w�肵����s�̂ݐ܂�Ԃ����C���B
	//
	// �Ԓl��
	//   2: "�܂�Ԃ�����" or "���̍s�����Ɉ�Ԓ����Ȃ���"
	//   1: "���̍s�ȊO�̂ǂ������Œ�"
	//   0: "�������܂ł��̍s�͍Œ����������Z���Ȃ��������"
	// �ŁA��ʃ��[�`����m_TextCx�C���̕K�v����`����B
	//
	// �͍̂ĕ`��͈͂̌v�Z�̂��߂ɁA�\���s���̕ω���Ԃ��Ă������A
	// ����͏�ʃ��[�`������ vln() ���r����΍ςނ��A
	// �ނ��낻�̕��������I�ł��邽�ߔp�~�����B


	// �����ۑ�
	WLine& wl            = wrap_[s.tl];
	const ulong oldVRNum = wl.rln();
	const ulong oldWidth = wl.width();

	// �����X�V
	wl.width() = CalcLineWidth( doc_.tl(s.tl), doc_.len(s.tl) );

	if( wl.width() < cvs_.wrapWidth() )
	{
		// �ݒ肵���܂�Ԃ������Z���ꍇ�͈�s�ōςށB
		wl[1] = doc_.len(s.tl);
		wl.ForceSize( 2 );
	}
	else
	{
		// �����s�ɂȂ�ꍇ
		ulong vr=1, stt=0;
		while( wl[vr] < s.ad ) // while( vr�s�ڂ͕ύX�ӏ�����O )
			stt = wl[ vr++ ];  // stt = ���̍s�̍s���̃A�h���X

		// �ύX�ӏ��ȍ~�̂ݏC��
		wl.ForceSize( vr );
		ModifyWrapInfo( doc_.tl(s.tl), doc_.len(s.tl), wl, stt );
	}

	// �\���s�̑������C��
	vlNum_ += ( wl.rln() - oldVRNum );

	// �܂�Ԃ��Ȃ����Ƒ������̍X�V���K�v
	if( cvs_.wrapType() == NOWRAP )
		if( textCx_ <= wl.width() )
		{
			textCx_ = wl.width();
			return 2;
		}
		else if( textCx_ == oldWidth )
		{
			return 0;
		}
		else
		{
			return 1;
		}
	return 2;
}

int ViewImpl::InsertMulti( ulong ti_s, ulong ti_e )
{
	// �w�肵���������V�����s����ǉ��B
	// ���܂�Ԃ�����������ƌv�Z
	//
	// �Ԓl��
	//   1: "�܂�Ԃ�����" or "���̍s�����Ɉ�Ԓ����Ȃ���"
	//   0: "���̍s�ȊO�̂ǂ������Œ�"
	// �ڂ����� ReWrapSingle() ������B

	ulong dy=0, cx=0;
	for( ulong i=ti_s; i<=ti_e; ++i )
	{
		WLine* pwl = new WLine;
		pwl->Add( CalcLineWidth( doc_.tl(i), doc_.len(i) ) );

		if( pwl->width() < cvs_.wrapWidth() )
		{
			// �ݒ肵���܂�Ԃ������Z���ꍇ�͈�s�ōςށB
			pwl->Add( doc_.len(i) );
			dy++;
			if( cx < pwl->width() )
				cx = pwl->width();
		}
		else
		{
			// �����s�ɂȂ�ꍇ
			ModifyWrapInfo( doc_.tl(i), doc_.len(i), *pwl, 0 );
			dy += pwl->rln();
		}

		wrap_.InsertAt( i, pwl );
	}

	// �\���s�̑������C��
	vlNum_ += dy;

	// �܂�Ԃ��Ȃ����Ƒ������̍X�V���K�v
	if( cvs_.wrapType() == NOWRAP )
	{
		if( textCx_ <= cx )
		{
			textCx_ = cx;
			return 1;
		}
		return 0;
	}
	return 1;
}

int ViewImpl::DeleteMulti( ulong ti_s, ulong ti_e )
{
	// �w�肵���͈͂̍s�����폜
	//
	// �Ԓl��
	//   1: "�܂�Ԃ�����" or "���̍s�ȊO�̂ǂ������Œ�"
	//   0: "�������܂ł��̍s�͍Œ����������Z���Ȃ��������"
	// �ڂ����� ReWrapSingle() ������B

	bool  widthChanged = false;
	ulong dy = 0;

	// �����W���Ȃ���폜
	for( ulong cx=textCx_, i=ti_s; i<=ti_e; ++i )
	{
		WLine& wl = wrap_[i];
		dy += wl.rln();
		if( cx == wl.width() )
			widthChanged = true;
	}
	wrap_.RemoveAt( ti_s, (ti_e-ti_s+1) );

	// �\���s�̑������C��
	vlNum_ -= dy;

	// �܂�Ԃ��Ȃ����Ƒ������̍X�V���K�v
	return ( cvs_.wrapType()==NOWRAP && widthChanged ) ? 0 : 1;
}



//-------------------------------------------------------------------------
// ���W�l�ϊ�
//-------------------------------------------------------------------------

void ViewImpl::ConvDPosToVPos( DPos dp, VPos* vp, const VPos* base ) const
{
	// �␳
	dp.tl = Min( dp.tl, doc_.tln()-1 );
	dp.ad = Min( dp.ad, doc_.len(dp.tl) );

	// �ϊ��̊�_���w�肳��Ă��Ȃ���΁A���_����Ƃ���
	VPos topPos(false); // �O�N���A
	if( base == NULL )
		base = &topPos;

	// �Ƃ肠����base�s���̒l�����Ă���
	ulong vl = base->vl - base->rl;
	ulong rl = 0;
	int vx;

	// �����s���������ꍇ
	//if( dp.tl == base->tl )
	//{
	//	�Ⴆ�� [��] ���������Ƃ��ȂǁA�E�ׂ̕����̉�����
	//	���������Ŏ��̈ʒu�͎Z�o�ł���B������g���ĕ��ʂ�
	//	�J�[�\���ړ��͂����ƍ������ł���͂��ł��邪�A
	//	�Ƃ肠�����ʓ|�������̂ŁA���̂Ƃ��뗪�B
	//}

	// �Ⴄ�s�������ꍇ
	//else
	{
		// vl�����킹��
		ulong tl = base->tl;
		if( tl > dp.tl )      // �ړI�n�������ɂ���ꍇ
			do
				vl -= rln(--tl);
			while( tl > dp.tl );
		else if( tl < dp.tl ) // �ړI�n�����艺�ɂ���ꍇ
			do
				vl += rln(tl++);
			while( tl < dp.tl );

		// rl�����킹��
		ulong stt=0;
		while( wrap_[tl][rl+1] < dp.ad )
			stt = wrap_[tl][++rl];
		vl += rl;

		// x���W�v�Z
		vx = CalcLineWidth( doc_.tl(tl)+stt, dp.ad-stt );
	}

	vp->tl = dp.tl;
	vp->ad = dp.ad;
	vp->vl = vl;
	vp->rl = rl;
	vp->rx = vp->vx = vx;
}

void ViewImpl::GetVPos( int x, int y, VPos* vp, bool linemode ) const
{
// x���W�␳

	x = x - lna() + rlScr_.nPos;

// �܂��s�ԍ��v�Z

	int tl = udScr_tl_;
	int vl = udScr_.nPos - udScr_vrl_;
	int rl = y / fnt().H() + udScr_vrl_;
	if( rl >= 0 ) // View��[��艺�̏ꍇ�A�������𒲂ׂ�
		while( tl < (int)doc_.tln() && (int)rln(tl) <= rl )
		{
			vl += rln(tl);
			rl -= rln(tl);
			++tl;
		}
	else           // View��[����̏ꍇ�A������𒲂ׂ�
		while( 0<=tl && rl<0 )
		{
			vl -= rln(tl);
			rl += rln(tl);
			--tl;
		}

	if( tl == (int)doc_.tln() ) // EOF��艺�ɍs���Ă��܂��ꍇ�̕␳
	{
		--tl, vl-=rln(tl), rl=rln(tl)-1;
		if( linemode )
			x = 0x4fffffff;
	}
	else if( tl == -1 ) // �t�@�C��������ɍs���Ă��܂��ꍇ�̕␳
	{
		tl = vl = rl = 0;
		if( linemode )
			x = 0;
	}
	else
	{
		if( linemode ) // �s�I�����[�h�̏ꍇ
		{
			if( tl == (int)doc_.tln()-1 )
				rl=rln(tl)-1, x=0x4fffffff;
			else
				vl+=rln(tl), rl=0, ++tl, x=0;
		}
	}

	vp->tl = tl;
	vp->vl = vl + rl;
	vp->rl = rl;

// ���ɁA���ʒu���v�Z

	if( rl < static_cast<int>(wrap_[tl].rln()) )
	{
		const unicode* str = doc_.tl(tl);
		const ulong adend = rlend(tl,rl);
		ulong ad = (rl==0 ? 0 : rlend(tl,rl-1));
		int   vx = (rl==0 ? 0 : fnt().W(&str[ad++]));

		while( ad<adend )
		{
			int nvx = (str[ad]==L'\t'
				? fnt().nextTab(vx)
				:  vx + fnt().W(&str[ad])
			);
			if( x+2 < nvx )
				break;
			vx = nvx;
			++ad;
		}

		vp->ad          = ad;
		vp->rx = vp->vx = vx;
	}
	else
	{
		vp->ad = vp->rx = vp->vx = 0;
	}
}
