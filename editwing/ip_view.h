#ifndef _EDITWING_IP_VIEW_H_
#define _EDITWING_IP_VIEW_H_
#include "ewView.h"
#include "ip_doc.h"
#ifndef __ccdoc__
namespace editwing {
namespace view {
#endif

#ifdef SHORT_TABLEWIDTH
#define CW_INTTYPE uchar
#else
#define CW_INTTYPE int
#endif

class View;

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
class Document;
class Painter
{
public:

	~Painter() {  Destroy(); }

	//@{ �w��ʒu�Ɉꕶ���o��, Output a single character at the specified position //@}
	void CharOut( unicode ch, int x, int y );

	//@{ �w��ʒu�ɕ�������o��, Output a string at the specified position //@}
	void StringOut( const unicode* str, int len, int x, int y );
	void StringOutA( const char* str, int len, int x, int y );

	void DrawCTLs( const unicode* str, int len, int x, int y );

	//@{ �����F�؂�ւ�, text color switching //@}
	void SetColor( int i );

	//@{ �w�i�F�œh��Ԃ�, Fill rect with background color //@}
	void Fill( const RECT& rc );

	//@{ ���], Invert color in the rectangle //@}
	void Invert( const RECT& rc );

	//@{ ��������, Draw a line from point x to point y //@}
	void DrawLine( int x1, int y1, int x2, int y2 );

	//@{ �N���b�v�̈�ݒ�, (IntersectClipRect())  //@}
	void SetClip( const RECT& rc );

	//@{ �N���b�v�̈����, SelectClipRgn( dc_, NULL)//@}
	void ClearClip();

	//@{ ���p�X�y�[�X�p�L���`��, Symbol drawing for half-width space //@}
	void DrawHSP( int x, int y, int times );

	//@{ �S�p�X�y�[�X�p�L���`��, Symbol drawing for full-width spaces //@}
	void DrawZSP( int x, int y, int times );

	void SetupDC(HDC hdc);
	void RestoreDC();

public:

	//@{ ����, height(pixel) //@}
	CW_INTTYPE H() const { return height_; }

	//@{ ������, digit width (pixel) //@}
	CW_INTTYPE F() const { return figWidth_; }

	//@{ ������, character width (pixel) //@}
	CW_INTTYPE Wc( const unicode ch ) const
	{ // Direclty return the character width!
	  // You must have initialized it before...
		return widthTable_[ ch ];
	}
	CW_INTTYPE W( const unicode* pch ) const // 1.08 �T���Q�[�g�y�A���, Avoid Surrogate Pair
	{
		unicode ch = *pch;
		if( widthTable_[ ch ] == (CW_INTTYPE)-1 )
		{
			if( isHighSurrogate(ch) )
			{	// We cannot save the width of chars from the extended
				// Unicode plane inside the widthTable_[ ]
				// We could in the future increase the widthTable_[ ]
				// For each extended plane that will have to be mapped.
#ifndef WIN32S
				SIZE sz;
				if( isLowSurrogate(pch[1])
				#if !defined(TARGET_VER) || TARGET_VER >= 310
				&&::GetTextExtentPoint32W( cdc_, pch, 2, &sz )
				#else
				&&::GetTextExtentPointW( cdc_, pch, 2, &sz )
				#endif
				){
					return (CW_INTTYPE)sz.cx; // Valid surrogate pair.
				}
#endif
				// Not a proper surrogate pair fallback to ?? (fast)
				return 2 * widthTable_[ L'?' ];
			}
#ifdef WIN32S
			if( useOutA_ )
			{
				if( ch > 0x007f )
				{	// For non ascii characters we use GetTextExtentPoint
					// so that even on MBCS systems we get the correct answer.
					SIZE sz;
					char strch[8]; // Small buffer for a single multi-byte character.
					DWORD len = WideCharToMultiByte(CP_ACP,0, &ch,1, strch, countof(strch), NULL, NULL);
					if( len && ::GetTextExtentPointA( cdc_, strch, len, &sz ) )
						widthTable_[ ch ] = (CW_INTTYPE)sz.cx;
					else
						widthTable_[ ch ] = 2 * widthTable_[ L'?' ]; // Default ?? width
				}
				else
				{
					#ifndef SHORT_TABLEWIDTH
					::GetCharWidthA( cdc_, ch, ch, widthTable_+ch );
					#else
					int width=0;
					::GetCharWidthA( cdc_, ch, ch, &width );
					widthTable_[ch] = (CW_INTTYPE)width;
					#endif
				}
			}
			else
#endif
			{ // Windows 9x/NT
				if( isInFontRange( ch ) )
				{
					#ifndef SHORT_TABLEWIDTH
					::GetCharWidthW( cdc_, ch, ch, widthTable_+ch );
					#else
					int width=0;
					::GetCharWidthW( cdc_, ch, ch, &width );
					widthTable_[ch] = (CW_INTTYPE)width;
					#endif
				}
				else
				{	// Use GetTextExtentPointW when cdc_ defaults to fallback font
					// TODO: Even beter, get the fallback font and use its char width.
					SIZE sz;
					::GetTextExtentPointW( cdc_, &ch, 1, &sz );
					widthTable_[ ch ] = (CW_INTTYPE)sz.cx;
				}
			}
		}
		return widthTable_[ ch ];
	}

	//@{ �W��������, standard character width (pixel) //@}
	CW_INTTYPE W() const { return widthTable_[ L'x' ]; }


	//@{ Is the character is in the selected font? //@}
	bool isInFontRange( const unicode ch ) const
	{
		if( fontranges_ )
		{
			const WCRANGE *range = fontranges_->ranges;
			for(uint i=0; i < fontranges_->cRanges; i++)
			{
				if( range[i].wcLow <= ch && ch <= range[i].wcLow + range[i].cGlyphs)
					return true;
			}
			return false;
		}
		// If we have no font range then we always use GetCharWidthW.
		return true;
	}

	//@{ ���̃^�u�����ʒu���v�Z, Calculate next tab alignment position //@}
	//int nextTab(int x) const { int t=T(); return (x/t+1)*t; }
	inline int nextTab(int x) const { int t=(int)T(); return ((x+4)/t+1)*t; }
	private: CW_INTTYPE T() const { return widthTable_[ L'\t' ]; } public:

	//@{ ���݂̃t�H���g���, Current font information //@}
	const LOGFONT& LogFont() const { return logfont_; }

	//@{ ���ʕ�����`�悷�邩�ۂ�, Whether to draw special characters or not //@}
	bool sc( int i ) const { return 0 != (scDraw_ & (1u << i)); }

private:

	const HWND   hwnd_;// Window in which we paint
	HDC          dc_;  // Device context used for Painting (non const)
	HDC          cdc_; // Compatible DC used for W() (const)
	HFONT        font_;
	HPEN         pen_;
	HBRUSH       brush_;
	HFONT  oldfont_;   // Old objects to be released before
	HPEN   oldpen_;    // the EndPaint() call.
	HBRUSH oldbrush_;  //
	CW_INTTYPE*  const widthTable_; // int or short [65535] values
	CW_INTTYPE   height_;
	CW_INTTYPE   figWidth_;
	LOGFONT      logfont_;
	GLYPHSET     *fontranges_;
	COLORREF     colorTable_[7];
	byte         scDraw_;
#ifdef WIN32S
	const bool   useOutA_;
#endif

private:

	Painter( HWND hwnd, const VConfig& vc );
	void Init( const VConfig& vc );
	void Destroy();
	HFONT init_font( const VConfig& vc );
	HWND getWHND() { return hwnd_; }
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

class Canvas
{
public:

	Canvas( const View& vw );
	~Canvas();
	//@{ View�̑傫���ύX�C�x���g����
	//	 @return �܂�Ԃ������ς������true //@}
	bool on_view_resize( int cx, int cy );

	//@{ �s���ύX�C�x���g����
	//	 @return �e�L�X�g�̈�̕����ς������true //@}
	bool on_tln_change( ulong tln );

	//@{ �t�H���g�ύX�C�x���g���� //@}
	void on_font_change( const VConfig& vc );

	//@{ �ݒ�ύX�C�x���g���� //@}
	void on_config_change( short wrap, bool showln, bool warpSmart );

	void on_config_change_nocalc( short wrap, bool showln, bool warpSmart );

public:

	//@{ [�s�ԍ���\�����邩�ۂ�] //@}
	bool showLN() const { return showLN_; }

	//@{ [-1:�܂�Ԃ�����  0:���E�[  else:�w�蕶����] //@}
	int wrapType() const { return wrapType_; }

	bool wrapSmart() const { return warpSmart_; }

	//@{ �܂�Ԃ���(pixel) //@}
	ulong wrapWidth() const { return wrapWidth_; }

	//@{ �\���̈�̈ʒu(pixel) //@}
	const RECT& zone() const { return txtZone_; }

private:

	short wrapType_;  // [ -1:�܂�Ԃ�����  0:���E�[  else:�w�蕶���� ]
	bool  warpSmart_; // [ Enable wrapping at word boundaries ]
	bool  showLN_;    // [ �s�ԍ���\�����邩�ۂ� ]

public:
	Painter       font_; // �`��p�I�u�W�F�N�g
private:
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

struct WLine: public ki::sstorage<ulong, false>
{
	// [0]   : ���̍s�̐܂�Ԃ������ł̉������i�[
	// [1-n] : n�s�ڂ̏I�[��index���i�[�B
	//
	//   �Ⴆ�� "aaabbb" �Ƃ����_���s�� "aaab" "bb" �Ɛ܂�Ȃ�
	//   {48, 4, 6} �ȂǂƂ��������R�̔z��ƂȂ�B

	ulong& width()      { return (*this)[0]; }
	ulong width() const { return (*this)[0]; }
	ulong rln() const   { return (*this).size()-1; }
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
	const RECT rc;  // �ĕ`��͈�, redraw range
	int XBASE;      // ��ԍ��̕�����x���W, x-coordinate of the leftmost character
	int XMIN;       // �e�L�X�g�ĕ`��͈͍��[, left edge of text redraw range
	int XMAX;       // �e�L�X�g�ĕ`��͈͉E�[, Right edge of text redraw range
	int YMIN;       // �e�L�X�g�ĕ`��͈͏�[, Top edge of text redraw range
	int YMAX;       // �e�L�X�g�ĕ`��͈͉��[, Bottom edge of text redraw range
	ulong TLMIN;    // �e�L�X�g�ĕ`��͈͏�[�_���s�ԍ�, Logical line number at the top of the text redraw range
	int SXB, SXE;   // �I��͈͂�x���W, x-coordinate of selection
	int SYB, SYE;   // �I��͈͂�y���W, y-coordinate of selection

	explicit VDrawInfo( const RECT& r )
		: rc(r), XBASE(0), XMIN(0),XMAX(0), YMIN(0),YMAX(0)
		, TLMIN(0), SXB(0),SXE(0), SYB(0),SYE(0) {}
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

class ViewImpl
{
public:

	ViewImpl( View& vw, doc::Document &dc );

	//@{ �܂�Ԃ������ؑ� //@}
	inline void SetWrapType( short wt )
		{ cvs_.on_config_change( wt, cvs_.showLN(), cvs_.wrapSmart() );
		  DoConfigChange(); }

	inline void SetWrapSmart( bool ws )
		{ cvs_.on_config_change( cvs_.wrapType(), cvs_.showLN(), ws );
		  DoConfigChange(); }


	//@{ �s�ԍ��\��/��\���ؑ� //@}
	inline void ShowLineNo( bool show )
		{ cvs_.on_config_change( cvs_.wrapType(), show, cvs_.wrapSmart() ); DoConfigChange(); }

	//@{ �\���F�E�t�H���g�ؑ� //@}
	inline void SetFont( const VConfig& vcc, short zoom )
	{
		VConfig vc = vcc;
		vc.fontsize = vc.fontsize * zoom / 100;
		vc.fontsize = vc.fontsize < 1 ? 1 : vc.fontsize;
		cvs_.on_font_change( vc );
		cur_.on_setfocus();
		CalcEveryLineWidth(); // �s���Čv�Z
		DoConfigChange();
	}

	//@{ All of the above in one go //@}
	inline void SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom )
		{ cvs_.on_config_change_nocalc( wt, showLN, ws );
		  SetFont( vc, zoom ); }

	//@{ �e�L�X�g�̈�̃T�C�Y�ύX�C�x���g //@}
	inline void on_view_resize( int cx, int cy ) { DoResize( cvs_.on_view_resize( cx, cy ) ); }

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
	const Painter& fnt() const { return cvs_.font_; }


	void on_hscroll( int code, int pos );
	void on_vscroll( int code, int pos );
	void on_wheel( short delta );
	void on_hwheel( short delta );

	void GetVPos( int x, int y, VPos* vp, bool linemode=false ) const;
	inline void GetOrigin( int* x, int* y ) const
		{ *x = left()-rlScr_.nPos, *y = -udScr_.nPos*cvs_.font_.H(); }
	void ConvDPosToVPos( DPos dp, VPos* vp, const VPos* base=NULL ) const;
	void ScrollTo( const VPos& vp );
	int  GetLastWidth( ulong tl ) const;
	int  getNumScrollLines( void );
	int  getNumScrollRaws( void );

public:

	inline const RECT& zone() const { return cvs_.zone(); }
	inline int left()  const { return cvs_.zone().left; }
	inline int right() const { return cvs_.zone().right; }
	inline int bottom()const { return cvs_.zone().bottom; }
	inline int lna()   const { return cvs_.zone().left; }
	inline int cx()    const { return cvs_.zone().right - cvs_.zone().left; }
	inline int cxAll() const { return cvs_.zone().right; }
	inline int cy()    const { return cvs_.zone().bottom; }

private:

	const doc::Document&   doc_;
	Canvas           cvs_;
	Cursor           cur_;
	ki::gapbufobjnoref<WLine> wrap_;
	ulong            vlNum_;
	ulong            textCx_;
	short            accdelta_;
	short            accdeltax_;

private:

	void DrawLNA( const VDrawInfo& v, Painter& p );
	void DrawTXT( const VDrawInfo& v, Painter& p );
	void Inv( int y, int xb, int xe, Painter& p );

	void CalcEveryLineWidth();
	ulong CalcLineWidth( const unicode* txt, ulong len ) const;
	bool isSpaceLike(unicode ch) const A_XPURE;
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



//=========================================================================

//=========================================================================
//@{ @pkg editwing.View //@}
//@{
//	�`�揈���Ȃ�
//
//	���̃N���X�ł́A���b�Z�[�W�̕��z���s�������ŁA������
//	Canvas/ViewImpl ���ōs���B�̂ŁA�ڂ����͂�������Q�Ƃ̂��ƁB
//@}
//=========================================================================

class View A_FINAL: public ki::WndImpl, public doc::DocEvHandler
{
public:

	//@{ �������Ȃ��R���X�g���N�^ //@}
	View( doc::Document& d, HWND wnd );
	~View();

	//@{ �܂�Ԃ������ؑ� //@}
	void SetWrapType( short wt );

	void SetWrapSmart( bool ws);

	//@{ �s�ԍ��\��/��\���ؑ� //@}
	void ShowLineNo( bool show );

	//@{ �\���F�E�t�H���g�ؑ� //@}
	void SetFont( const VConfig& vc, short zoom );

	//@{ Set all canva stuff at once (faster) //@}
	void SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom );

	//@{ �J�[�\�� //@}
	Cursor& cur();

private:

	doc::Document&      doc_;
	ViewImpl*           impl_;
	static ClsName     className_;

private:

	void    on_create( CREATESTRUCT* cs ) override;
	void    on_destroy() override;
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp ) override;
	void    on_text_update( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur ) override;
	void    on_keyword_change() override;
};


}}     // namespace editwing::view
#endif // _EDITWING_IP_VIEW_H_
