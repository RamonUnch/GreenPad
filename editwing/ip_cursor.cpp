#include "stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;
using doc::Insert;
using doc::Delete;
using doc::Replace;



//=========================================================================
//---- ip_cursor.cpp �J�[�\���R���g���[��
//
//		�J���b�g��\��������IME�ɓK���ɑΉ�������F�X�B
//		�Ƃ���ŋ^��Ȃ̂��� Caret ���āu�J���b�g�v��
//		�ǂނ̂��u�L�����b�g�v�Ɠǂނ̂��H
//
//---- ip_text.cpp   �����񑀍�E��
//---- ip_parse.cpp  �L�[���[�h���
//---- ip_wrap.cpp   �܂�Ԃ�
//---- ip_scroll.cpp �X�N���[��
//---- ip_draw.cpp   �`��E��
//=========================================================================



//-------------------------------------------------------------------------
// Caret����p���b�p�[
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
// �J�[�\��������
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
	// �Ă��Ɓ[�ɏ�񏉊���
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
// �w���p�[�֐��Q
//-------------------------------------------------------------------------

void Cursor::UpdateCaretPos()
{
	// �����o�ϐ��̒l�����ɁA���ۂ�Caret�𓮂�������
	int x, y;
	view_.GetOrigin( &x, &y );
	x += cur_.vx;
	y += cur_.vl * view_.fnt().H();

	// �s�ԍ��]�[����Caret�������Ă�����̂ō��ɒǂ����
	if( 0<x && x<view_.left() )
		x = -view_.left();

	// �Z�b�g
	caret_->SetPos( x, y );
	pEvHan_->on_move( cur_, sel_ );
}

void Cursor::Redraw( const VPos& s, const VPos& e )
{
	int x, y; // ���_
	view_.GetOrigin( &x, &y );

	POINT sp = {x+s.vx, y+s.vl*view_.fnt().H()};
	POINT ep = {x+e.vx, y+e.vl*view_.fnt().H()};
	if( s > e ) // Swap
		sp.x^=ep.x, ep.x^=sp.x, sp.x^=ep.x,
		sp.y^=ep.y, ep.y^=sp.y, sp.y^=ep.y;
	ep.x+=2;

	// �蔲��16bit�`�F�b�N����c
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




//-------------------------------------------------------------------------
// View����̎w�߂�����
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
	// �ݒ�ύX�ȂǂɑΉ�
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
// �L�[���͂ւ̑Ή�
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
	}
}

void Cursor::on_char( TCHAR ch )
{
	if( !bRO_ && ch!=0x7f
	 && ((unsigned)ch>=0x20 || ch==TEXT('\r') || ch==TEXT('\t')) )
	{
	#ifdef _UNICODE
		pEvHan_->on_char( *this, ch );
	#else
		unicode wc = ch;
		if( ch & 0x80 ) // ��ASCII�����ɂ̓g���r�A���łȂ��ϊ����K�v
			::MultiByteToWideChar( CP_ACP, MB_COMPOSITE, &ch, 1, &wc, 1 );
		pEvHan_->on_char( *this, wc );
	#endif
	}
}

void Cursor::on_ime_composition( LPARAM lp )
{
	view_.ScrollTo( cur_ );
	if( !bRO_ && (lp&GCS_RESULTSTR) )
	{
		unicode* str;
		ulong    len;
		ime().GetString( caret_->hwnd(), &str, &len );
		if( str )
		{
			pEvHan_->on_ime( *this, str, len );
			delete [] str;
		}
	}
}

void Cursor::on_keydown( int vk, LPARAM flag )
{
	bool sft = (::GetKeyState(VK_SHIFT)>>15)!=0;
	bool ctl = (::GetKeyState(VK_CONTROL)>>15)!=0;
	pEvHan_->on_key( *this, vk, sft, ctl );
}



//-------------------------------------------------------------------------
// ���[�h�ؑ�
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
// �������́E�폜
//-------------------------------------------------------------------------

void Cursor::InputChar( unicode ch )
{
	// �u�㏑���[�h �� �I����ԂłȂ� �� �s���łȂ��v�Ȃ�E�ꕶ���I��
	if( !bIns_ && cur_==sel_ && doc_.len(cur_.tl)!=cur_.ad )
		Right( false, true );

	// ����
	Input( &ch, 1 );
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
	len = ::MultiByteToWideChar( CP_ACP, 0, str, len, ustr, len*4 );
	Input( ustr, len );
	delete [] ustr;
}

void Cursor::DelBack()
{
	// �I����ԂȂ� BackSpace == Delete
	// �łȂ���΁A BackSpace == Left + Delete (�蔲��
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
	// �I����ԂȂ� cur_ �` sel_ ���폜
	// �łȂ���΁A cur_ �` rightOf(cur_) ���폜
	DPos dp = (cur_==sel_ ? doc_.rightOf(cur_) : (DPos)sel_ );
	if( cur_ != dp )
		doc_.Execute( Delete( cur_, dp ) );
}



//-------------------------------------------------------------------------
// �e�L�X�g�擾
//-------------------------------------------------------------------------

ki::aarr<unicode> Cursor::getSelectedStr() const
{
	DPos dm=cur_, dM=sel_;
	if( cur_ > sel_ )
		dm=sel_, dM=cur_;

	// �e�L�X�g�擾
	int len = doc_.getRangeLength( dm, dM );
	ki::aarr<unicode> ub( new unicode[len+1] );
	doc_.getText( ub.get(), dm, dM );
	return ub;
}

//-------------------------------------------------------------------------
// �N���b�v�{�[�h����
//-------------------------------------------------------------------------

void Cursor::Cut()
{
	if( cur_ != sel_ )
	{
		// �R�s�[���č폜
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
	unicode* p;
	int    len = doc_.getRangeLength( dm, dM );

	if( app().isNT() )
	{
		// NT�n�Ȃ炻�̂܂܃_�C���N�g��
		h = ::GlobalAlloc( GMEM_MOVEABLE, (len+1)*2 );
		doc_.getText( static_cast<unicode*>(::GlobalLock(h)), dm, dM );
		::GlobalUnlock( h );
		clp.SetData( CF_UNICODETEXT, h );
	}
	else
	{
		// 9x�n�Ȃ�ϊ����K�v
		h = ::GlobalAlloc( GMEM_MOVEABLE, (len+1)*3 );
		p = new unicode[len+1];
		doc_.getText( p, dm, dM );
		::WideCharToMultiByte( CP_ACP, 0, p, -1,
			static_cast<char*>(::GlobalLock(h)), (len+1)*3, NULL, NULL );
		::GlobalUnlock( h );
		clp.SetData( CF_TEXT, h );
		delete [] p;
	}
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
// �J�[�\���ړ�
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
		// �I����Ԃ��ς��͈͂��ĕ`��
		Redraw( vp, cur_ );
	}
	else
	{
		// �I�����������͈͂��ĕ`��
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
	if( wide ) // �����̓���
		np.tl = np.vl = 0;
	else // �s�̓���
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
	if( wide ) // �����̖�����
	{
		np.tl = doc_.tln()-1;
		np.vl = view_.vln()-1;
	}
	else // �s�̖�����
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
	// �͂ݏo���ꍇ�́A�擪�s/�I�[�s�Ŏ~�܂�悤�ɐ���
	VPos np = cur_;
	if( (signed)np.vl + dy < 0 )
		dy = -(signed)np.vl;
	else if( np.vl + dy >= view_.vln() )
		dy = view_.vln()-np.vl-1;

	np.vl += dy;
	np.rl += dy;
	if( dy<0 ) // ��֖߂�ꍇ
	{
		// �W�����v��_���s�̍s����Dash!
		while( (signed)np.rl < 0 )
			np.rl += view_.rln(--np.tl);
	}
	else if( dy>0 ) // ���֐i�ޏꍇ
	{
		// �W�����v��_���s�̍s����Dash!
		while( (signed)np.rl > 0 )
			np.rl -= view_.rln(np.tl++);
		if( (signed)np.rl < 0 )
			np.rl += view_.rln(--np.tl); //�s���߂��C���`
	}

	// x���W����ɂ�����
	const unicode* str = doc_.tl(np.tl);

	// �E�񂹂ɂȂ��Ă�B�s���R�H
	np.ad = (np.rl==0 ? 0 : view_.rlend(np.tl,np.rl-1)+1);
	np.vx = (np.rl==0 ? 0 : view_.fnt().W(&str[np.ad-1]));
	while( np.vx < np.rx && np.ad < view_.rlend(np.tl,np.rl) )
	{
		// ���񂹂ɂ��Ă݂��B
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
	Ud( -view_.cy()/view_.fnt().H(), select );
}

void Cursor::PageDown( bool select )
{
	Ud( view_.cy()/view_.fnt().H(), select );
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
// �}�E�X���͂ւ̑Ή�
//-------------------------------------------------------------------------

void Cursor::on_lbutton_dbl( short x, short y )
{
	// �s�ԍ��]�[���̏ꍇ�͓��ɉ������Ȃ�
	if( view_.lna()-view_.fnt().F() < x )
		// �s���̏ꍇ�����ɉ������Ȃ�
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
		// ����܂ł̑I��͈͂��N���A
		Redraw( cur_, sel_ );

		// �s�ԍ��]�[���̃N���b�N��������A�s�I�����[�h��
		lineSelectMode_ = ( x < view_.lna()-view_.fnt().F() );

		// �I���J�n�ʒu�𒲐�
		view_.GetVPos( x, y, &sel_ );
		if( lineSelectMode_ )
			view_.ConvDPosToVPos( DPos(sel_.tl,0), &sel_, &sel_ );
		cur_ = sel_;
	}

	// �ړ��I
	MoveByMouse( dragX_=x, dragY_=y );

	// �}�E�X�ʒu�̒ǐՊJ�n
	timerID_ = ::SetTimer( caret_->hwnd(), 178116, keyRepTime_, NULL );
	::SetCapture( caret_->hwnd() );
}

void Cursor::on_lbutton_up( short x, short y )
{
	// �ǐՉ���
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
		// View�����Ȃ�MouseMove�ɔ���
		POINT pt = { dragX_=x, dragY_=y };
		if( PtInRect( &view_.zone(), pt ) )
			MoveByMouse( dragX_, dragY_ );
	}
}

void Cursor::on_timer()
{
	// View�O���Ȃ�Timer�ɔ���
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
	if( isSelected() && ime().IsIME() && ime().CanReconv() )
	{
		aarr<unicode> ub = getSelectedStr();
		ulong len;
		for(len=0; ub[len]; ++len);
		ime().SetString( caret_->hwnd(), ub.get(), len);
	}
}

void Cursor::ToggleIME()
{
	if( ime().IsIME() )
	{
		ime().SetState( caret_->hwnd(), !ime().GetState( caret_->hwnd() ) );
	}
}

//-------------------------------------------------------------------------
// �ĕϊ�
//-------------------------------------------------------------------------

int Cursor::on_ime_reconvertstring( RECONVERTSTRING* rs )
{
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
	const ulong len = ::lstrlen(str.get());
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
}

bool Cursor::on_ime_confirmreconvertstring( RECONVERTSTRING* rs )
{
	return false;
}

