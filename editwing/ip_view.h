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
//	描画基本ルーチン
//
//	利用するには、Canvasオブジェクトから getPainter して使います。
//	画面用デバイスコンテキストのレイヤです。きちんと書いておけば印刷機能を
//	追加するときに楽なのかもしれませんが、そんなことを考える計画性が
//	あるはずもなく極めて適当に…。
//@}
//=========================================================================
class Document;
class Painter
{
public:

	~Painter() {  Destroy(); }

	//@{ 指定位置に一文字出力, Output a single character at the specified position //@}
	void CharOut( unicode ch, int x, int y );

	//@{ 指定位置に文字列を出力, Output a string at the specified position //@}
	void StringOut( const unicode* str, int len, int x, int y );
	void StringOutA( const char* str, int len, int x, int y );

	void DrawCTLs( const unicode* str, int len, int x, int y );

	//@{ 文字色切り替え, text color switching //@}
	void SetColor( int i );

	//@{ 背景色で塗りつぶし, Fill rect with background color //@}
	void Fill( const RECT& rc );

	//@{ 反転, Invert color in the rectangle //@}
	void Invert( const RECT& rc );

	//@{ 線を引く, Draw a line from point x to point y //@}
	void DrawLine( int x1, int y1, int x2, int y2 );

	//@{ クリップ領域設定, (IntersectClipRect())  //@}
	void SetClip( const RECT& rc );

	//@{ クリップ領域解除, SelectClipRgn( dc_, NULL)//@}
	void ClearClip();

	//@{ 半角スペース用記号描画, Symbol drawing for half-width space //@}
	void DrawHSP( int x, int y, int times );

	//@{ 全角スペース用記号描画, Symbol drawing for full-width spaces //@}
	void DrawZSP( int x, int y, int times );

	void SetupDC(HDC hdc);
	void RestoreDC();

public:

	//@{ 高さ, height(pixel) //@}
	CW_INTTYPE H() const { return height_; }

	//@{ 数字幅, digit width (pixel) //@}
	CW_INTTYPE F() const { return figWidth_; }

	//@{ 文字幅, character width (pixel) //@}
	CW_INTTYPE Wc( const unicode ch ) const
	{ // Direclty return the character width!
	  // You must have initialized it before...
		return widthTable_[ ch ];
	}
	CW_INTTYPE W( const unicode* pch ) const // 1.08 サロゲートペア回避, Avoid Surrogate Pair
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

	//@{ 標準文字幅, standard character width (pixel) //@}
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

	//@{ 次のタブ揃え位置を計算, Calculate next tab alignment position //@}
	//int nextTab(int x) const { int t=T(); return (x/t+1)*t; }
	inline int nextTab(int x) const { int t=(int)T(); return ((x+4)/t+1)*t; }
	private: CW_INTTYPE T() const { return widthTable_[ L'\t' ]; } public:

	//@{ 現在のフォント情報, Current font information //@}
	const LOGFONT& LogFont() const { return logfont_; }

	//@{ 特別文字を描画するか否か, Whether to draw special characters or not //@}
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
//	描画可能領域
//
//	ウインドウサイズの変更や折り返しの有無やフォントの設定などに
//	対応して、描画領域のサイズを適当に管理します。やることは
//	とりあえずそれだけ。
//@}
//=========================================================================

class Canvas
{
public:

	Canvas( const View& vw );
	~Canvas();
	//@{ Viewの大きさ変更イベント処理
	//	 @return 折り返し幅が変わったらtrue //@}
	bool on_view_resize( int cx, int cy );

	//@{ 行数変更イベント処理
	//	 @return テキスト領域の幅が変わったらtrue //@}
	bool on_tln_change( ulong tln );

	//@{ フォント変更イベント処理 //@}
	void on_font_change( const VConfig& vc );

	//@{ 設定変更イベント処理 //@}
	void on_config_change( short wrap, bool showln, bool warpSmart );

	void on_config_change_nocalc( short wrap, bool showln, bool warpSmart );

public:

	//@{ [行番号を表示するか否か] //@}
	bool showLN() const { return showLN_; }

	//@{ [-1:折り返し無し  0:窓右端  else:指定文字数] //@}
	int wrapType() const { return wrapType_; }

	bool wrapSmart() const { return warpSmart_; }

	//@{ 折り返し幅(pixel) //@}
	ulong wrapWidth() const { return wrapWidth_; }

	//@{ 表示領域の位置(pixel) //@}
	const RECT& zone() const { return txtZone_; }

private:

	short wrapType_;  // [ -1:折り返し無し  0:窓右端  else:指定文字数 ]
	bool  warpSmart_; // [ Enable wrapping at word boundaries ]
	bool  showLN_;    // [ 行番号を表示するか否か ]

public:
	Painter       font_; // 描画用オブジェクト
private:
	ulong    wrapWidth_; // 折り返し幅(pixel)
	RECT       txtZone_; // テキスト表示域の位置(pixel)
	int         figNum_; // 行番号の桁数

private:

	bool CalcLNAreaWidth();
	void CalcWrapWidth();

private:

	NOCOPY(Canvas);
};



//=========================================================================
//@{
//	行毎の折り返し情報
//@}
//=========================================================================

struct WLine: public ki::sstorage<ulong, false>
{
	// [0]   : その行の折り返し無しでの横幅を格納
	// [1-n] : n行目の終端のindexを格納。
	//
	//   例えば "aaabbb" という論理行を "aaab" "bb" と折るなら
	//   {48, 4, 6} などという長さ３の配列となる。

	ulong& width()      { return (*this)[0]; }
	ulong width() const { return (*this)[0]; }
	ulong rln() const   { return (*this).size()-1; }
};



//=========================================================================
//@{
//	再描画範囲を指定するためのフラグ
//@}
//=========================================================================

enum ReDrawType
{
	LNAREA, // 行番号ゾーンのみ
	LINE,   // 変更のあった一行のみ
	AFTER,  // 変更のあった行以下全部
	ALL     // 全画面
};



//=========================================================================
//@{
//	描画処理を細かく指定する構造体
//@}
//=========================================================================

struct VDrawInfo
{
	const RECT rc;  // 再描画範囲, redraw range
	int XBASE;      // 一番左の文字のx座標, x-coordinate of the leftmost character
	int XMIN;       // テキスト再描画範囲左端, left edge of text redraw range
	int XMAX;       // テキスト再描画範囲右端, Right edge of text redraw range
	int YMIN;       // テキスト再描画範囲上端, Top edge of text redraw range
	int YMAX;       // テキスト再描画範囲下端, Bottom edge of text redraw range
	ulong TLMIN;    // テキスト再描画範囲上端論理行番号, Logical line number at the top of the text redraw range
	int SXB, SXE;   // 選択範囲のx座標, x-coordinate of selection
	int SYB, SYE;   // 選択範囲のy座標, y-coordinate of selection

	explicit VDrawInfo( const RECT& r )
		: rc(r), XBASE(0), XMIN(0),XMAX(0), YMIN(0),YMAX(0)
		, TLMIN(0), SXB(0),SXE(0), SYB(0),SYE(0) {}
};



//=========================================================================
//@{
//	折り返しedテキストの管理・表示等
//
//	Canvasクラスによって計算された領域サイズを参考に、テキストの
//	折り返し処理を実行する。ここで、スクロール制御、描画処理など
//	主要な処理は全て実行することになる。
//@}
//=========================================================================

class ViewImpl
{
public:

	ViewImpl( View& vw, doc::Document &dc );

	//@{ 折り返し方式切替 //@}
	inline void SetWrapType( short wt )
		{ cvs_.on_config_change( wt, cvs_.showLN(), cvs_.wrapSmart() );
		  DoConfigChange(); }

	inline void SetWrapSmart( bool ws )
		{ cvs_.on_config_change( cvs_.wrapType(), cvs_.showLN(), ws );
		  DoConfigChange(); }


	//@{ 行番号表示/非表示切替 //@}
	inline void ShowLineNo( bool show )
		{ cvs_.on_config_change( cvs_.wrapType(), show, cvs_.wrapSmart() ); DoConfigChange(); }

	//@{ 表示色・フォント切替 //@}
	inline void SetFont( const VConfig& vcc, short zoom )
	{
		VConfig vc = vcc;
		vc.fontsize = vc.fontsize * zoom / 100;
		vc.fontsize = vc.fontsize < 1 ? 1 : vc.fontsize;
		cvs_.on_font_change( vc );
		cur_.on_setfocus();
		CalcEveryLineWidth(); // 行幅再計算
		DoConfigChange();
	}

	//@{ All of the above in one go //@}
	inline void SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom )
		{ cvs_.on_config_change_nocalc( wt, showLN, ws );
		  SetFont( vc, zoom ); }

	//@{ テキスト領域のサイズ変更イベント //@}
	inline void on_view_resize( int cx, int cy ) { DoResize( cvs_.on_view_resize( cx, cy ) ); }

	void DoResize( bool wrapWidthChanged );
	void DoConfigChange();

	//@{ テキストデータの更新イベント //@}
	void on_text_update( const DPos& s,
		const DPos& e, const DPos& e2, bool bAft, bool mCur );

	//@{ 描画処理 //@}
	void on_paint( const PAINTSTRUCT& ps );

public:

	//@{ 全表示行数 //@}
	ulong vln() const { return vlNum_; }

	//@{ 一行の表示行数 //@}
	ulong rln( ulong tl ) const { return wrap_[tl].rln(); }

	//@{ 折り返し位置 //@}
	ulong rlend( ulong tl, ulong rl ) const { return wrap_[tl][rl+1]; }

	//@{ 一個でも折り返しが存在するか否か //@}
	bool wrapexists() const { return doc_.tln() != vln(); }

	//@{ カーソル //@}
	Cursor& cur() { return cur_; }

	//@{ フォント //@}
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
	SCROLLINFO rlScr_; // 横スクロール情報（pixel単位）
	SCROLLINFO udScr_; // 縦スクロール情報（行単位）
	ulong udScr_tl_;   // 一番上に表示される論理行のTLine_Index
	ulong udScr_vrl_;  // 一番上に表示される表示行のVRLine_Index

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
//	描画処理など
//
//	このクラスでは、メッセージの分配を行うだけで、実装は
//	Canvas/ViewImpl 等で行う。ので、詳しくはそちらを参照のこと。
//@}
//=========================================================================

class View A_FINAL: public ki::WndImpl, public doc::DocEvHandler
{
public:

	//@{ 何もしないコンストラクタ //@}
	View( doc::Document& d, HWND wnd );
	~View();

	//@{ 折り返し方式切替 //@}
	void SetWrapType( short wt );

	void SetWrapSmart( bool ws);

	//@{ 行番号表示/非表示切替 //@}
	void ShowLineNo( bool show );

	//@{ 表示色・フォント切替 //@}
	void SetFont( const VConfig& vc, short zoom );

	//@{ Set all canva stuff at once (faster) //@}
	void SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom );

	//@{ カーソル //@}
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
