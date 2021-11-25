#ifndef _EDITWING_IP_VIEW_H_
#define _EDITWING_IP_VIEW_H_
#include "ewView.h"
#include "ip_doc.h"
using namespace ki;
#ifndef __ccdoc__
namespace editwing {
namespace view {
#endif



using doc::DocImpl;



//=========================================================================
//@{ @pkg editwing.View.Impl //@}
//@{
//	�`���{���[�`��
//
//	���p����ɂ́ACanvas�I�u�W�F�N�g���� getPainter ���Ďg���܂��B
//	��ʗp�f�o�C�X�R���e�L�X�g�̃��C���ł��B������Ə����Ă����Έ���@�\��
//	�ǉ�����Ƃ��Ɋy�Ȃ̂�������܂��񂪁A����Ȃ��Ƃ��l����v�搫��
//	����͂����Ȃ��ɂ߂ēK���Ɂc�B
//@}
//=========================================================================

class Painter : public Object
{
public:

	~Painter();

	//@{ �w��ʒu�Ɉꕶ���o�� //@}
	void CharOut( unicode ch, int x, int y );

	//@{ �w��ʒu�ɕ�������o�� //@}
	void StringOut( const unicode* str, int len, int x, int y );

	//@{ �����F�؂�ւ� //@}
	void SetColor( int i );

	//@{ �w�i�F�œh��Ԃ� //@}
	void Fill( const RECT& rc );

	//@{ ���] //@}
	void Invert( const RECT& rc );

	//@{ �������� //@}
	void DrawLine( int x1, int y1, int x2, int y2 );

	//@{ �N���b�v�̈�ݒ� //@}
	void SetClip( const RECT& rc );

	//@{ �N���b�v�̈���� //@}
	void ClearClip();

	//@{ ���p�X�y�[�X�p�L���`�� //@}
	void DrawHSP( int x, int y, int times );

	//@{ �S�p�X�y�[�X�p�L���`�� //@}
	void DrawZSP( int x, int y, int times );

public:

	//@{ ����(pixel) //@}
	int H() const { return height_; }

	//@{ ������(pixel) //@}
	int F() const { return figWidth_; }

	//@{ ������(pixel) //@}
	int Wc( unicode ch ) const
		{
			if( widthTable_[ ch ] == -1 )
#ifdef WIN32S
				if(ch > 0x100)
				{
					::GetCharWidthA( dc_, 'x', 'x', widthTable_+ch );
					widthTable_[ ch ] *= 2;
				}
				else
				{
					::GetCharWidthA( dc_, ch, ch, widthTable_+ch );
				}
#else
				::GetCharWidthW( dc_, ch, ch, widthTable_+ch );
#endif
			return widthTable_[ ch ];
		}
	int W( const unicode* pch ) const // 1.08 �T���Q�[�g�y�A���
		{
			unicode ch = *pch;
			if( widthTable_[ ch ] == -1 )
			{
				if( isHighSurrogate(ch) )
				{
					SIZE sz;
					if( ::GetTextExtentPoint32W( dc_, pch, 2, &sz ) )
						return sz.cx;
					int w = 0;
#ifdef WIN32S
					if(ch > 0x100)
					{
						::GetCharWidthA( dc_, 'x', 'x', &w );
						w *= 2;
					}
					else
					{
						::GetCharWidthA( dc_, ch, ch, &w );
					}
#else
					::GetCharWidthW( dc_, ch, ch, &w );
#endif
					return w;
				}
#ifdef WIN32S
				if(ch > 0x100)
				{
					::GetCharWidthA( dc_, 'x', 'x', widthTable_+ch );
					widthTable_[ ch ] *= 2;
				}
				else
				{
					::GetCharWidthA( dc_, ch, ch, widthTable_+ch );
				}
#else
				::GetCharWidthW( dc_, ch, ch, widthTable_+ch );
#endif
			}
			return widthTable_[ ch ];
		}

	//@{ �W��������(pixel) //@}
	int W() const { return widthTable_[ L'x' ]; }

	//@{ ���̃^�u�����ʒu���v�Z //@}
	//int nextTab(int x) const { int t=T(); return (x/t+1)*t; }
	int nextTab(int x) const { int t=T(); return ((x+4)/t+1)*t; }
	private: int T() const { return widthTable_[ L'\t' ]; } public:

	//@{ ���݂̃t�H���g��� //@}
	const LOGFONT& LogFont() const { return logfont_; }

	//@{ ���ʕ�����`�悷�邩�ۂ� //@}
	bool sc( int i ) const { return scDraw_[i]; }

private:

	const HDC    dc_;
	const HFONT  font_;
	const HPEN   pen_;
	const HBRUSH brush_;
	int          height_;
	int*         widthTable_;
	int          figWidth_;
	LOGFONT      logfont_;
	COLORREF     colorTable_[7];
	bool         scDraw_[5];

private:

	Painter( HDC hdc, const VConfig& vc );
	HDC getDC() { return dc_; }
	friend class Canvas;
	NOCOPY(Painter);
};



//=========================================================================
//@{
//	�`��\�̈�
//
//	�E�C���h�E�T�C�Y�̕ύX��܂�Ԃ��̗L����t�H���g�̐ݒ�Ȃǂ�
//	�Ή����āA�`��̈�̃T�C�Y��K���ɊǗ����܂��B��邱�Ƃ�
//	�Ƃ肠�������ꂾ���B
//@}
//=========================================================================

class Canvas : public Object
{
public:

	Canvas( const View& vw );

	//@{ View�̑傫���ύX�C�x���g����
	//	 @return �܂�Ԃ������ς������true //@}
	bool on_view_resize( int cx, int cy );

	//@{ �s���ύX�C�x���g����
	//	 @return �e�L�X�g�̈�̕����ς������true //@}
	bool on_tln_change( ulong tln );

	//@{ �t�H���g�ύX�C�x���g���� //@}
	void on_font_change( const VConfig& vc );

	//@{ �ݒ�ύX�C�x���g���� //@}
	void on_config_change( int wrap, bool showln );

public:

	//@{ [�s�ԍ���\�����邩�ۂ�] //@}
	bool showLN() const { return showLN_; }

	//@{ [-1:�܂�Ԃ�����  0:���E�[  else:�w�蕶����] //@}
	int wrapType() const { return wrapType_; }

	//@{ �܂�Ԃ���(pixel) //@}
	ulong wrapWidth() const { return wrapWidth_; }

	//@{ �\���̈�̈ʒu(pixel) //@}
	const RECT& zone() const { return txtZone_; }

	//@{ �`��p�I�u�W�F�N�g //@}
	Painter& getPainter() const { return *font_; }

private:

	int  wrapType_;  // [ -1:�܂�Ԃ�����  0:���E�[  else:�w�蕶���� ]
	bool showLN_;    // [ �s�ԍ���\�����邩�ۂ� ]

	dptr<Painter> font_; // �`��p�I�u�W�F�N�g
	ulong    wrapWidth_; // �܂�Ԃ���(pixel)
	RECT       txtZone_; // �e�L�X�g�\����̈ʒu(pixel)
	int         figNum_; // �s�ԍ��̌���

private:

	bool CalcLNAreaWidth();
	void CalcWrapWidth();

private:

	NOCOPY(Canvas);
};



//=========================================================================
//@{
//	�s���̐܂�Ԃ����
//@}
//=========================================================================

struct WLine : public storage<ulong>
{
	// [0]   : ���̍s�̐܂�Ԃ������ł̉������i�[
	// [1-n] : n�s�ڂ̏I�[��index���i�[�B
	//
	//   �Ⴆ�� "aaabbb" �Ƃ����_���s�� "aaab" "bb" �Ɛ܂�Ȃ�
	//   {48, 4, 6} �ȂǂƂ��������R�̔z��ƂȂ�B

	WLine() : storage<ulong>(2) {}
	ulong& width()      { return (*this)[0]; }
	ulong width() const { return (*this)[0]; }
	ulong rln() const   { return size()-1; }
};



//=========================================================================
//@{
//	�ĕ`��͈͂��w�肷�邽�߂̃t���O
//@}
//=========================================================================

enum ReDrawType
{
	LNAREA, // �s�ԍ��]�[���̂�
	LINE,   // �ύX�̂�������s�̂�
	AFTER,  // �ύX�̂������s�ȉ��S��
	ALL     // �S���
};



//=========================================================================
//@{
//	�`�揈�����ׂ����w�肷��\����
//@}
//=========================================================================

struct VDrawInfo
{
	const RECT rc;  // �ĕ`��͈�
	int XBASE;      // ��ԍ��̕�����x���W
	int XMIN;       // �e�L�X�g�ĕ`��͈͍��[
	int XMAX;       // �e�L�X�g�ĕ`��͈͉E�[
	int YMIN;       // �e�L�X�g�ĕ`��͈͏�[
	int YMAX;       // �e�L�X�g�ĕ`��͈͉��[
	ulong TLMIN;    // �e�L�X�g�ĕ`��͈͏�[�_���s�ԍ�
	int SXB, SXE;   // �I��͈͂�x���W
	int SYB, SYE;   // �I��͈͂�y���W

	explicit VDrawInfo( const RECT& r ) : rc(r) {}
};



//=========================================================================
//@{
//	�܂�Ԃ�ed�e�L�X�g�̊Ǘ��E�\����
//
//	Canvas�N���X�ɂ���Čv�Z���ꂽ�̈�T�C�Y���Q�l�ɁA�e�L�X�g��
//	�܂�Ԃ����������s����B�����ŁA�X�N���[������A�`�揈���Ȃ�
//	��v�ȏ����͑S�Ď��s���邱�ƂɂȂ�B
//@}
//=========================================================================

class ViewImpl : public Object
{
public:

	ViewImpl( View& vw, DocImpl& dc );

	//@{ �܂�Ԃ������ؑ� //@}
	void SetWrapType( int wt );

	//@{ �s�ԍ��\��/��\���ؑ� //@}
	void ShowLineNo( bool show );

	//@{ �\���F�E�t�H���g�ؑ� //@}
	void SetFont( const VConfig& vc );

		//@{ �e�L�X�g�̈�̃T�C�Y�ύX�C�x���g //@}
		void on_view_resize( int cx, int cy );

	void DoResize( bool wrapWidthChanged );
	void DoConfigChange();

	//@{ �e�L�X�g�f�[�^�̍X�V�C�x���g //@}
	void on_text_update( const DPos& s,
		const DPos& e, const DPos& e2, bool bAft, bool mCur );

	//@{ �`�揈�� //@}
	void on_paint( const PAINTSTRUCT& ps );

public:

	//@{ �S�\���s�� //@}
	ulong vln() const { return vlNum_; }

	//@{ ��s�̕\���s�� //@}
	ulong rln( ulong tl ) const { return wrap_[tl].rln(); }

	//@{ �܂�Ԃ��ʒu //@}
	ulong rlend( ulong tl, ulong rl ) const { return wrap_[tl][rl+1]; }

	//@{ ��ł��܂�Ԃ������݂��邩�ۂ� //@}
	bool wrapexists() const { return doc_.tln() != vln(); }

	//@{ �J�[�\�� //@}
	Cursor& cur() { return cur_; }

	//@{ �t�H���g //@}
	const Painter& fnt() const { return cvs_.getPainter(); }


	void on_hscroll( int code, int pos );
	void on_vscroll( int code, int pos );
	void on_wheel( short delta );

	void GetVPos( int x, int y, VPos* vp, bool linemode=false ) const;
	void GetOrigin( int* x, int* y ) const;
	void ConvDPosToVPos( DPos dp, VPos* vp, const VPos* base=NULL ) const;
	void ScrollTo( const VPos& vp );
	int  GetLastWidth( ulong tl ) const;

public:

	const RECT& zone() const { return cvs_.zone(); }
	int left()  const { return cvs_.zone().left; }
	int right() const { return cvs_.zone().right; }
	int bottom()const { return cvs_.zone().bottom; }
	int lna()   const { return cvs_.zone().left; }
	int cx()    const { return cvs_.zone().right - cvs_.zone().left; }
	int cxAll() const { return cvs_.zone().right; }
	int cy()    const { return cvs_.zone().bottom; }

private:

	const DocImpl&   doc_;
	Canvas           cvs_;
	Cursor           cur_;
	gapbufobj<WLine> wrap_;
	ulong            vlNum_;
	ulong            textCx_;

private:

	void DrawLNA( const VDrawInfo& v, Painter& p );
	void DrawTXT( const VDrawInfo v, Painter& p );
	void Inv( int y, int xb, int xe, Painter& p );

	void CalcEveryLineWidth();
	ulong CalcLineWidth( const unicode* txt, ulong len ) const;
	void ModifyWrapInfo( const unicode* txt, ulong len, WLine& wl, ulong stt );
	void ReWrapAll();
	int ReWrapSingle( const DPos& s );
	int InsertMulti( ulong ti_s, ulong ti_e );
	int DeleteMulti( ulong ti_s, ulong ti_e );
	void UpdateTextCx();
	void ReDraw( ReDrawType r, const DPos* s=NULL );

private:

	HWND hwnd_;
	SCROLLINFO rlScr_; // ���X�N���[�����ipixel�P�ʁj
	SCROLLINFO udScr_; // �c�X�N���[�����i�s�P�ʁj
	ulong udScr_tl_;   // ��ԏ�ɕ\�������_���s��TLine_Index
	ulong udScr_vrl_;  // ��ԏ�ɕ\�������\���s��VRLine_Index

private:

	bool ReSetScrollInfo();
	void ForceScrollTo( ulong tl );
	void UpdateScrollBar();
	ReDrawType TextUpdate_ScrollBar( const DPos& s, const DPos& e, const DPos& e2 );

	ulong tl2vl( ulong tl ) const;
	void GetDrawPosInfo( VDrawInfo& v ) const;
	void InvalidateView( const DPos& dp, bool afterall ) const;
	void ScrollView( int dx, int dy, bool update );
	void UpDown( int dy, bool thumb );
};



//-------------------------------------------------------------------------

inline void ViewImpl::on_view_resize( int cx, int cy )
	{ DoResize( cvs_.on_view_resize( cx, cy ) ); }

inline void ViewImpl::SetWrapType( int wt )
	{ cvs_.on_config_change( wt, cvs_.showLN() );
	  DoConfigChange(); }

inline void ViewImpl::ShowLineNo( bool show )
	{ cvs_.on_config_change( cvs_.wrapType(), show );
	  DoConfigChange(); }

inline void ViewImpl::SetFont( const VConfig& vc )
	{ cvs_.on_font_change( vc );
	  cur_.on_setfocus();
	  CalcEveryLineWidth(); // �s���Čv�Z
	  DoConfigChange(); }

inline void ViewImpl::GetOrigin( int* x, int* y ) const
	{ *x = left()-rlScr_.nPos, *y = -udScr_.nPos*cvs_.getPainter().H(); }



//=========================================================================

}}     // namespace editwing::view
#endif // _EDITWING_IP_VIEW_H_
