
#include "../kilib/stdafx.h"
#include "ip_view.h"
using namespace ki;
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_scroll.cpp スクロール
//
//		ウインドウサイズはスクロールバーの位置によって
//		描画位置を適当に更新していく処理がここ。
//
//---- ip_text.cpp   文字列操作・他
//---- ip_parse.cpp  キーワード解析
//---- ip_wrap.cpp   折り返し
//---- ip_draw.cpp   描画・他
//---- ip_cursor.cpp カーソルコントロール
//=========================================================================

#if defined(TARGET_VER) && TARGET_VER < 351
#ifdef WIN32S
// Win32s scroll range is mimited to 32767
#define MAX_SCROLL 32000
#define MAX_MULT (32768-MAX_SCROLL)
#else
// Windows NT: scrollrange is limited to 65535
#define MAX_SCROLL 65000
#define MAX_MULT (65536-MAX_SCROLL)
#endif

typedef int (WINAPI *ssnfo_funk)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw);
static int WINAPI MySetScrollInfo_1st(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw);
static ssnfo_funk MySetScrollInfo = MySetScrollInfo_1st;

static int WINAPI MySetScrollInfo_fb(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw)
{
	// Fallback function for NT3.1/3.5 and win32s
	// We must use SetScrollRange but it is mimited to 65535 (32767 on Win32s).
	// So we can avoid oveflow by dividing range and position values
	// In GreenPad we can assume that only nMax can go beyond range.
	int MULT=1;
	int nMax = lpsi->nMax;
	// If we go beyond MAX_SCROLL then we use a divider
	if (nMax > MAX_SCROLL)
	{   // 32000 - 32767 = 767 values MULT from 2-768 (~24M lines)
		MULT = Min( (nMax / MAX_SCROLL) + 1,  MAX_MULT );
		// We store the divider in the last 767 values
		// of the max scroll range.
		nMax = MAX_SCROLL + MULT - 1 ; // 32001 => MULT = 2
	}
	if (lpsi->fMask|SIF_RANGE)
		::SetScrollRange( hwnd, fnBar, lpsi->nMin, nMax, FALSE );

	return ::SetScrollPos( hwnd, fnBar, lpsi->nPos/MULT, fredraw );
}
// First time function.
static int WINAPI MySetScrollInfo_1st(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw)
{
	// Should be supported since Windows NT 3.51.944
	ssnfo_funk dyn_SetScrollInfo = (ssnfo_funk)GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")), "SetScrollInfo");
	if( dyn_SetScrollInfo
	&& !(  app().is9xOSVerLarger( MKVER(4,00,275) ) // Win95 4.00.275
		|| app().isNTOSVerLarger( MKVER(3,51,944) ) // WinNT 3.51.944
	    ) )
	{   // Not supported before 95 build 275
		// Nor NT3.51 before build 944
		dyn_SetScrollInfo = NULL;
	}

	if( dyn_SetScrollInfo )
	{
		MySetScrollInfo = dyn_SetScrollInfo;
		return dyn_SetScrollInfo( hwnd, fnBar, lpsi, fredraw );
	}
	// Use fallback function
	MySetScrollInfo = MySetScrollInfo_fb;
	return MySetScrollInfo_fb( hwnd, fnBar, lpsi, fredraw );
}

typedef int (WINAPI *gsnfo_funk)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
static int WINAPI MyGetScrollInfo_1st(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
static gsnfo_funk MyGetScrollInfo = MyGetScrollInfo_1st;

static int WINAPI MyGetScrollInfo_fb(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
	// Smart Fallback...
	if( lpsi->fMask|SIF_RANGE )
		::GetScrollRange( hwnd, fnBar, &lpsi->nMin, &lpsi->nMax);
	lpsi->nPos = ::GetScrollPos( hwnd, fnBar );
	// Scroll range indicates a multiplier was used...
	if( lpsi->nMax > MAX_SCROLL )
	{ // Apply multipler
		int MULT = lpsi->nMax - MAX_SCROLL + 1; // 32001 => MULT = 2
		lpsi->nMax      *= MULT;
		lpsi->nPos      *= MULT;
		lpsi->nTrackPos *= MULT; // This has to be set by the user.
	}

	return 1; // sucess!
}
// First time function.
static int WINAPI MyGetScrollInfo_1st(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
	// Should be supported since Windows NT 3.51...
	gsnfo_funk dyn_GetScrollInfo = (gsnfo_funk)GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")), "GetScrollInfo");
	if( dyn_GetScrollInfo
	&& !(  app().is9xOSVerLarger(MKVER(4,00,275)) // Win95 4.00.275
		|| app().isNTOSVerLarger(MKVER(3,51,944)) // WinNT 3.51.944
	    ) )
	{
		dyn_GetScrollInfo = NULL;
	}

	if( dyn_GetScrollInfo )
	{
		MyGetScrollInfo = dyn_GetScrollInfo;
		return dyn_GetScrollInfo( hwnd, fnBar, lpsi );
	}
	// Use fallback function
	MyGetScrollInfo = MyGetScrollInfo_fb;
	return MyGetScrollInfo_fb( hwnd, fnBar, lpsi );
}
#else
#define MyGetScrollInfo GetScrollInfo
#define MySetScrollInfo SetScrollInfo
#endif //305<=TARGET_VER<NT3.51

//-------------------------------------------------------------------------
// 描画領域サイズ管理
//-------------------------------------------------------------------------

namespace
{
	static int Log10( ulong n )
	{
		// return Max( 3, Log10(n) + 1 );
		ulong p = 1000;
		int i;
		for( i = 3; i < ULONG_DIGITS; i++ )
		{
			if( n < p ) return i;
			p *= 10;
		}
		return i;
	}
}

bool Canvas::CalcLNAreaWidth()
{
	const int prev = txtZone_.left;
	if( showLN_ )
	{
		txtZone_.left  = (1 + figNum_) * font_.F();
		if( txtZone_.left+4*font_.W() >= txtZone_.right )
			txtZone_.left = 0; // 行番号ゾーンがデカすぎるときは表示しない
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
		wrapWidth_ = txtZone_.right - txtZone_.left - font_.W()/2 - 1;
		break; //Caretの分-3補正
	default:
		wrapWidth_ = wrapType_ * font_.W();
		break;
	}
}

Canvas::Canvas( const View& vw )
	: wrapType_ ( -1 )
	, warpSmart_( false )
	, showLN_   ( false )
	, font_     (  vw.hwnd(), VConfig(TEXT(""),0) )
	, wrapWidth_( 0xffffffff )
	, figNum_   ( 3 )
{
	vw.getClientRect( &txtZone_ );
}

Canvas::~Canvas()
{
	//delete font_;
}

// Return true if warp width has changed.
bool Canvas::on_view_resize( int cx, int cy )
{
	txtZone_.bottom = cy; // Update canva height
	if( txtZone_.right  == cx )
		return false; // Same canvas width, nothing more to do.

	txtZone_.right  = cx; // Canva width was changed.
	CalcLNAreaWidth(); // Is width is too small line number diseapear.
	if( wrapType_ == RIGHTEDGE )
	{
		CalcWrapWidth();
		return true;
	}
	return false; // No rewrapping to do
}

void Canvas::on_font_change( const VConfig& vc )
{
	//HWND hwnd = font_->getWHND();
	//delete font_; // 先にデストラクタを呼ばねばならない…
	              // ってうわー格好悪ぃーーー(T_T)
	//font_ = new Painter( hwnd, vc );
	font_.Destroy();
	font_.Init(vc);

	CalcLNAreaWidth();
	CalcWrapWidth();
}

void Canvas::on_config_change( short wrap, bool showln, bool wrapsmart )
{
	showLN_ = showln;
	wrapType_ = wrap;
	warpSmart_ = wrapsmart;

	CalcLNAreaWidth();
	CalcWrapWidth();
}

void Canvas::on_config_change_nocalc( short wrap, bool showln, bool wrapsmart )
{
	showLN_ = showln;
	wrapType_ = wrap;
	warpSmart_ = wrapsmart;
}

bool Canvas::on_tln_change( ulong tln )
{
	figNum_ = Log10( tln ); // 桁数計算

	if( CalcLNAreaWidth() )
	{
		if( wrapType_ == RIGHTEDGE )
			CalcWrapWidth();
		return true;
	}
	return false;
}



//-------------------------------------------------------------------------
// スクロールバー計算ルーチン
//-------------------------------------------------------------------------
// rl (横スクロール情報)
// max:  view.txt.txtwidth()
// page: view.cx()
// pos:  0〜max-page

// ud (縦スクロール情報)
// max:   view.txt.vln() + page - 1
// page:  view.cy() / view.fnt.H()
// delta: 0〜view.fnt.H()
// pos:   0〜max-page (topの行番号)

bool ViewImpl::ReSetScrollInfo()
{
	const int prevRlPos = rlScr_.nPos;
	const ulong cx = cvs_.zone().right - cvs_.zone().left;
	const ulong cy = cvs_.zone().bottom;

	// 横は変な値にならないよう補正するだけでよい
//	rlScr_.nPage = cx + 1;
//	rlScr_.nMax  = Max( textCx_, cx );
//	rlScr_.nPos  = Min<int>( rlScr_.nPos, rlScr_.nMax-rlScr_.nPage+1 );
	rlScr_.nPage = cx + 1;
	rlScr_.nMax  = Max( textCx_+cvs_.font_.W()/2+1, cx );
	rlScr_.nPos  = Min( rlScr_.nPos, (int)(rlScr_.nMax-rlScr_.nPage+1) );

	// 縦はnPageとnMaxはとりあえず補正
	// nPosは場合によって直し方が異なるので別ルーチンにて
	udScr_.nPage = cy / NZero(cvs_.font_.H()) + 1;
	//udScr_.nMax  = vln() + udScr_.nPage - 2; // Old code (not so nice)

	// WIP: Adjust so that scroll bar appears only when we are shy oneline from the page.
	// PB: Drawing problem when adding/removing line and scroll bar did not kick in.
	// We need to do a proper InvalidateRect in the TextUpdate_ScrollBar()?
	// udScr_.nMax  = vln() + udScr_.nPage - Max( 2, Min<int>( udScr_.nPage-1, vln()+1 ) );

	// Limit more scrolling when there is more tha a single page
	if( vln()*cvs_.font_.H() < cy )
		udScr_.nMax  = vln() + udScr_.nPage - 2;
	else
		udScr_.nMax  = vln() + udScr_.nPage - Max( 2, (int)Min( udScr_.nPage-1, (uint)(vln()+1) ) );

	// 横スクロールが起きたらtrue
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
		// パターン１：現在の画面上端より下で更新された場合
		// スクロールしない
	}
	else if( udScr_tl_ == s.tl )
	{
		// パターン２：現在の画面上端と同じ行で更新された場合
		// 出来るだけ同じ位置を表示し続けようと試みる。

		if( static_cast<ulong>(udScr_.nPos) >= vln() )
		{
			// パターン2-1：しかしそこはすでにEOFよりも下だ！
			// しゃーないので一番下の行を表示
			udScr_.nPos = vln()-1;
			udScr_tl_   = doc_.tln()-1;
			udScr_vrl_  = rln(udScr_tl_)-1;
			ans = ALL;
		}
		else
		{

			// パターン2-2：
			// スクロール無し
			while( udScr_vrl_ >= rln(udScr_tl_) )
			{
				udScr_vrl_ -= rln(udScr_tl_);
				udScr_tl_++;
			}
		}
	}
	else
	{
		// パターン３：現在の画面上端より上で更新された場合
		// 表示内容を変えないように頑張る

		if( e.tl < udScr_tl_ )
		{
			// パターン3-1：変更範囲の終端も、現在行より上の場合
			// 行番号は変わるが表示内容は変わらないで済む
			udScr_.nPos += vl_dif;
			udScr_tl_   += (e2.tl - e.tl);
			ans = LNAREA;
		}
		else
		{
			// パターン3-2：
			// どうしよーもないので適当な位置にスクロール
			ForceScrollTo( e2.tl );
			ans = ALL;
		}
	}

	// どんな再描画をすればよいか返す
	return (rlScrolled ? ALL : ans);
}

void ViewImpl::ScrollTo( const VPos& vp )
{
	// 横フォーカス
	int dx=0;
	if( vp.vx < (signed)rlScr_.nPos )
	{
		dx = vp.vx - rlScr_.nPos;
	}
	else
	{
		const int W = cvs_.font_.W();
		if( rlScr_.nPos + (signed)(rlScr_.nPage-W) <= vp.vx )
			dx = vp.vx - (rlScr_.nPos + rlScr_.nPage) + W;
	}

	// 縦フォーカス
	int dy=0;
	if( vp.vl < (unsigned)udScr_.nPos )
		dy = vp.vl - udScr_.nPos;
	else if( udScr_.nPos + (udScr_.nPage-1) <= vp.vl )
		dy = vp.vl - (udScr_.nPos + udScr_.nPage) + 2;

	// スクロール
	if( dy!=0 )	UpDown( dy, dx==0 );
	if( dx!=0 )	ScrollView( dx, 0, true );
}

void ViewImpl::GetDrawPosInfo( VDrawInfo& v ) const
{
	const int H = cvs_.font_.H();

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

		// 縦座標, Vertical coordinates
		v.YMIN  = y * H;
		v.YMAX  = Min( v.rc.bottom, most_under );
		v.TLMIN = tl;

		// 横座標, Horizontal coordinates
		v.XBASE = left() - rlScr_.nPos;
		v.XMIN  = v.rc.left  - v.XBASE;
		v.XMAX  = v.rc.right - v.XBASE;

		// 選択範囲, Selection range
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
	// スクロール開始通知, Scroll notification start
	cur_.on_scroll_begin();

	const RECT* clip = (dy==0 ? &cvs_.zone() : NULL);
	const int H = cvs_.font_.H();

	// スクロールバー更新, Scrollbar updated
	if( dx != 0 )
	{
		// 範囲チェック, range check
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
		// 範囲チェック…は前処理で終わってる。
		udScr_.nPos += dy;
		::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
		dy *= -H;
	}
	if( dx!=0 || dy!=0 )
	{
		if( -dx>=right() || dx>=right()
		 || -dy>=bottom() || dy>=bottom() )
		{
			// 全画面再描画
			// ちょうど65536の倍数くらいスクロールしたときに、
			// ScrollWindowEx on Win9x だと再描画が変なのを回避。
			::InvalidateRect( hwnd_, NULL, FALSE );
		}
		else
		{
			// 再描画の不要な領域をスクロール
			// Scroll through areas that do not need redrawing
			#ifdef WIN32S
			// On Win32s 1.1 ScrollWindowEx does not work!
			// In our case ScrollWindow() is perfectly fine.
			::ScrollWindow( hwnd_, dx, dy, NULL, clip);
			#else
			::ScrollWindowEx( hwnd_, dx, dy, NULL,
					clip, NULL, NULL, SW_INVALIDATE );
			#endif

			// 即時再描画？, Immediate redraw?
			if( update )
			{
				// 縦スクロールは高速化したいので一工夫, Vertical scrolling is one way to speed up the process.
				if( dy != 0 )
				{
					// 再描画の必要な領域を自分で計算
					// Calculate the area that needs to be redrawn
					RECT rc = { 0, 0, right(), bottom() };
					if( dy < 0 ) rc.top  = rc.bottom + dy;
					else         rc.bottom = dy;

					// インテリマウスの中ボタンクリックによる
					// オートスクロール用カーソルの下の部分を先に描く
					// ２回に分けることで、小さな矩形部分二つで済むので高速
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

	// スクロール終了通知
	cur_.on_scroll_end();
}

void ViewImpl::on_hscroll( int code, int pos )
{
	// 変化量を計算
	int dx;
	switch( code )
	{
	default:           return;
	case SB_LINELEFT:  dx= -cvs_.font_.W(); break;
	case SB_LINERIGHT: dx= +cvs_.font_.W(); break;
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

	// スクロール
	ScrollView( dx, 0, code!=SB_THUMBTRACK );
}

void ViewImpl::on_vscroll( int code, int pos )
{
	// 変化量を計算
	int dy;
	switch( code )
	{
	default:          return;
	case SB_LINEUP:   dy= -1; break;
	case SB_LINEDOWN: dy= +1; break;
	case SB_PAGEUP:   dy= -( cy() / NZero(cvs_.font_.H()) ); break;
	case SB_PAGEDOWN: dy= +( cy() / NZero(cvs_.font_.H()) ); break;
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

	// スクロール
	UpDown( dy, code==SB_THUMBTRACK );
}

int ViewImpl::getNumScrollLines( void )
{
	uint scrolllines = 3; // Number of lines to scroll (default 3).
	if( app().getOSVer() >= 0x0400 )
	{   // Read the system value for the wheel scroll lines.
		UINT numlines;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &numlines, 0 ) )
			scrolllines = numlines; // Sucess!
	}
	// If the number of lines is larger than a single page then we only scroll a page.
	// This automatically takes into account the page scroll mode where
	// SPI_GETWHEELSCROLLLINES value if 0xFFFFFFFF.
	uint nlpage;
	nlpage = Max( (uint)cy() / (uint)NZero(cvs_.font_.H()), 1U );
	// scrolllines can be zero, in this case no scroll should occur.
	return (int)Min( scrolllines, nlpage );
}

void ViewImpl::on_wheel( short delta )
{
	// スクロール
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
	if ( app().getOSVer() >= 0x0600 )
	{ // Introduced in window Vista
		UINT num;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLCHARS, 0, &num, 0 ) )
			scrollnum = num; // Sucess!
	}
	else if( app().getOSVer() >= 0x0400 )
	{   // Read the system value for the wheel scroll lines.
		UINT num;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &num, 0 ) )
			scrollnum = num; // Sucess!
	}
	// Avoid scrolling more that halfa screen horizontally.
	uint nlpage;
	nlpage = Max( (uint)cx() / (uint)NZero(cvs_.font_.W()/2), 1U );
	// scrollnum can be zero, in this case no scroll should occur.
	return (int)Min( scrollnum, nlpage );
}

void ViewImpl::on_hwheel( short delta )
{
	int nl = getNumScrollRaws();
	int dx = nl * cvs_.font_.W() * (int)delta / WHEEL_DELTA;
	if( dx == 0 )
	{ // step is too small, we need to accumulate delta
		accdeltax_ += delta;
		// Recalculate the step.
		dx = nl * cvs_.font_.W() * (int)accdeltax_ / WHEEL_DELTA;
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
  // １．udScr_.nPos + dy が正常範囲に収まるように補正
	if( udScr_.nPos+dy < 0 )
		dy = -udScr_.nPos;
	else if( udScr_.nMax+1-(signed)udScr_.nPage < udScr_.nPos+dy )
		dy = udScr_.nMax+1-udScr_.nPage-udScr_.nPos;

	if( dy==0 )
		return;

  // ２−１．折り返し無しの場合は一気にジャンプ出来る
	if( !wrapexists() )
	{
		udScr_tl_ = udScr_.nPos + dy;
	}

  // ２−２．でなけりゃ、現在位置からの相対サーチ
  // ScrollBarを連続的にドラッグせず一度に一気に飛んだ場合は
  // 1行目や最終行からの相対サーチの方が有効な可能性があるが、
  // その場合は多少速度が遅くなっても描画が引っかかることはないのでＯＫ
	else
	{
		int   rl = dy + udScr_vrl_;
		ulong tl = udScr_tl_;

		if( dy<0 ) // 上へ戻る場合
		{
			// ジャンプ先論理行の行頭へDash!
			while( rl < 0 )
				rl += rln(--tl);
		}
		else/*if( dy>0 )*/ // 下へ進む場合
		{
			// ジャンプ先論理行の行頭へDash!
			while( rl > 0 )
				rl -= rln(tl++);
			if( rl < 0 )
				rl += rln(--tl); //行き過ぎ修正
		}
		udScr_tl_ = tl;
		udScr_vrl_= static_cast<ulong>(rl);
	}

  // ４．画面をスクロール
	ScrollView( 0, dy, !thumb );
}

void ViewImpl::InvalidateView( const DPos& dp, bool afterall ) const
{
	const int H = cvs_.font_.H();

	// 表示域より上での更新, Update above the display area
	if( dp.tl < udScr_tl_ )
	{
		if( afterall )
			::InvalidateRect( hwnd_, NULL, FALSE );
		return;
	}

	// 開始y座標計算, Start y-coordinate calculation
	int r=0, yb=-(signed)udScr_vrl_;
	for( int t=udScr_tl_, ybe=cy() / NZero(H); (unsigned)t<dp.tl; yb+=rln(t++) )
		if( yb >= ybe )
			return;
	for( ; dp.ad>rlend(dp.tl,r); ++r,++yb );
	yb = H * Max( yb, -100 ); // 上にはみ出し過ぎないよう調整, Adjustment to avoid overhang on top
	if( yb >= cy() )
		return;

	// １行目を再描画, Redraw the first line
	int rb = (r==0 ? 0 : rlend(dp.tl,r-1));
 	int xb = left() + Max( (ulong)0,
		CalcLineWidth(doc_.tl(dp.tl)+rb, (ulong) (dp.ad-rb)) - rlScr_.nPos );
	if( xb < right() )
	{
		RECT rc={xb,yb,right(),yb+H};
		::InvalidateRect( hwnd_, &rc, FALSE );
	}

	// 残り, remaining
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
