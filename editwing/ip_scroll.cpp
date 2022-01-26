#include "stdafx.h"
#include "ip_view.h"
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

typedef int (WINAPI *ssnfo_funk)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw);
static int MySetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fredraw)
{
	// Should be supported since Windows NT 3.51.944
	static ssnfo_funk dyn_SetScrollInfo = (ssnfo_funk)(-1);
	if( dyn_SetScrollInfo == (ssnfo_funk)(-1) ) {
		dyn_SetScrollInfo = (ssnfo_funk)GetProcAddress(GetModuleHandleA("USER32.DLL"), "SetScrollInfo");
		if(!( (!App::isNT() && App::getOSBuild()>=275) 
		||    (App::isNT() && App::getOSVer()>=351 && App::getOSBuild()>=944)) ) 
		{   // Not supported before 95 build 275
			// Nor NT3.51 before build 944
			dyn_SetScrollInfo = NULL;
		}
	}

	if( dyn_SetScrollInfo )
		return dyn_SetScrollInfo( hwnd, fnBar, lpsi, fredraw );

	// Fallback...
	if (lpsi->fMask|SIF_RANGE) 
		::SetScrollRange( hwnd, fnBar, lpsi->nMin, lpsi->nMax, FALSE );
	return ::SetScrollPos( hwnd, fnBar, lpsi->nPos, fredraw );
}
typedef int (WINAPI *gsnfo_funk)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
static int MyGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
	// Should be supported since Windows NT 3.51...
	static gsnfo_funk dyn_GetScrollInfo = (gsnfo_funk)(-1);
	if( dyn_GetScrollInfo == (gsnfo_funk)(-1) ) {
		dyn_GetScrollInfo = (gsnfo_funk)GetProcAddress(GetModuleHandleA("USER32.DLL"), "GetScrollInfo");
		if(!( (!App::isNT() && App::getOSBuild()>=275) 
		||    (App::isNT() && App::getOSVer()>=351 && App::getOSBuild()>=944)) ) 
		{
			dyn_GetScrollInfo = NULL;
		}
	}

	if( dyn_GetScrollInfo ) {
		return dyn_GetScrollInfo( hwnd, fnBar, lpsi );
	}

	// Fallback...
	lpsi->nPos = ::GetScrollPos( hwnd, fnBar );
	if(lpsi->fMask|SIF_RANGE) 
		::GetScrollRange( hwnd, fnBar, &lpsi->nMin, &lpsi->nMax);
	return 1;
}

//-------------------------------------------------------------------------
// 描画領域サイズ管理
//-------------------------------------------------------------------------

namespace
{
	static int Log10( ulong n )
	{
		const static ulong power_of_ten[] =
			{ 1, 10, 100, 1000, 10000, 100000, 1000000,
			  10000000, 100000000, 1000000000 }; // 10^0 ～ 10^9
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
		wrapWidth_ = txtZone_.right - txtZone_.left - 3;
		break; //Caretの分-3補正
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
	font_ = NULL; // 先にデストラクタを呼ばねばならない…
	              // ってうわー格好悪ぃーーー(T_T)
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
// pos:  0～max-page

// ud (縦スクロール情報)
// max:   view.txt.vln() + page - 1
// page:  view.cy() / view.fnt.H()
// delta: 0～view.fnt.H()
// pos:   0～max-page (topの行番号)

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
	rlScr_.nMax  = Max( textCx_+3, cx );
	rlScr_.nPos  = Min<int>( rlScr_.nPos, rlScr_.nMax-rlScr_.nPage+1 );

	// 縦はnPageとnMaxはとりあえず補正
	// nPosは場合によって直し方が異なるので別ルーチンにて
	udScr_.nPage = cy / NZero(cvs_.getPainter().H()) + 1;
	udScr_.nMax  = vln() + udScr_.nPage - 2;

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
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	::MySetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
	::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
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
		const int W = cvs_.getPainter().W();
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

		// 縦座標
		v.YMIN  = y * H;
		v.YMAX  = Min( v.rc.bottom, most_under );
		v.TLMIN = tl;

		// 横座標
		v.XBASE = left() - rlScr_.nPos;
		v.XMIN  = v.rc.left  - v.XBASE;
		v.XMAX  = v.rc.right - v.XBASE;

		// 選択範囲
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
	// スクロール開始通知
	cur_.on_scroll_begin();

	const RECT* clip = (dy==0 ? &cvs_.zone() : NULL);
	const int H = cvs_.getPainter().H();

	// スクロールバー更新
	if( dx != 0 )
	{
		// 範囲チェック
		if( rlScr_.nPos+dx < 0 )
			dx = -rlScr_.nPos;
		else if( rlScr_.nMax-(signed)rlScr_.nPage < rlScr_.nPos+dx ) 
			dx = rlScr_.nMax-rlScr_.nPage-rlScr_.nPos+1;

		rlScr_.nPos += dx;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		::SetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
#elif defined(TARGET_VER) && TARGET_VER<=350 && TARGET_VER>300
		::MySetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
#else
		::SetScrollPos( hwnd_, SB_HORZ, rlScr_.nPos, TRUE );
#endif
		dx = -dx;
	}
	if( dy != 0 )
	{
		// 範囲チェック…は前処理で終わってる。

		udScr_.nPos += dy;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		::SetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
#elif defined(TARGET_VER) && TARGET_VER<=350 && TARGET_VER>300
		::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
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
			// 全画面再描画
			// ちょうど65536の倍数くらいスクロールしたときに、
			// ScrollWindowEx on Win9x だと再描画が変なのを回避。
			::InvalidateRect( hwnd_, NULL, FALSE );
		}
		else
		{
			// 再描画の不要な領域をスクロール
			::ScrollWindowEx( hwnd_, dx, dy, NULL, 
					clip, NULL, NULL, SW_INVALIDATE );

			// 即時再描画？
			if( update )
			{
				// 縦スクロールは高速化したいので一工夫
				if( dy != 0 )
				{
					// 再描画の必要な領域を自分で計算
					RECT rc = {0,0,right(),bottom()};
					if( dy < 0 ) rc.top  = rc.bottom + dy;
					else         rc.bottom = dy;

					// インテリマウスの中ボタンクリックによる
					// オートスクロール用カーソルの下の部分を先に描く
					// ２回に分けることで、小さな矩形部分二つで済むので高速
					::ValidateRect( hwnd_, &rc );
					::UpdateWindow( hwnd_ );
					::InvalidateRect( hwnd_, &rc, FALSE );
				}
				::UpdateWindow( hwnd_ );
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
	case SB_LINELEFT:  dx= -cvs_.getPainter().W(); break;
	case SB_LINERIGHT: dx= +cvs_.getPainter().W(); break;
	case SB_PAGELEFT:  dx= -(cx()>>1); break;
	case SB_PAGERIGHT: dx= +(cx()>>1); break;
	case SB_THUMBTRACK:
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>305)
		{
			SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			si.nTrackPos = pos;
			::MyGetScrollInfo( hwnd_, SB_HORZ, &si );
			dx = si.nTrackPos - rlScr_.nPos;
			break;
		}
#else
		dx = pos - rlScr_.nPos; break;
#endif
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
	case SB_PAGEUP:   dy= -( cy() / NZero(cvs_.getPainter().H()) ); break;
	case SB_PAGEDOWN: dy= +( cy() / NZero(cvs_.getPainter().H()) ); break;
	case SB_THUMBTRACK:
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>305)
		{
			SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			si.nTrackPos = pos; // in case
			::MyGetScrollInfo( hwnd_, SB_VERT, &si );
			dy = si.nTrackPos - udScr_.nPos;
			break;
		}
#else
		dy = pos - udScr_.nPos; break;
#endif
	case SB_TOP:      dy = -udScr_.nPos; break;
	case SB_BOTTOM:   dy = udScr_.nMax+1-(signed)udScr_.nPage-udScr_.nPos; break;
	}

	// スクロール
	UpDown( dy, code==SB_THUMBTRACK );
}

void ViewImpl::on_wheel( short delta )
{
	// スクロール
	UpDown( -delta / WHEEL_DELTA * 3, false );
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

  // ２－１．折り返し無しの場合は一気にジャンプ出来る
	if( !wrapexists() )
	{
		udScr_tl_ = udScr_.nPos + dy;
	}

  // ２－２．でなけりゃ、現在位置からの相対サーチ
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
		else if( dy>0 ) // 下へ進む場合
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
	const int H = cvs_.getPainter().H();

	// 表示域より上での更新
	if( dp.tl < udScr_tl_ )
	{
		if( afterall )
			::InvalidateRect( hwnd_, NULL, FALSE );
		return;
	}

	// 開始y座標計算
	int r=0, yb=-(signed)udScr_vrl_;
	for( int t=udScr_tl_, ybe=cy() / NZero(H); (unsigned)t<dp.tl; yb+=rln(t++) )
		if( yb >= ybe )
			return;
	for( ; dp.ad>rlend(dp.tl,r); ++r,++yb );
	yb = H * Max( yb, -100 ); // 上にはみ出し過ぎないよう調整
	if( yb >= cy() )
		return;

	// １行目を再描画
	int rb = (r==0 ? 0 : rlend(dp.tl,r-1));
 	int xb = left() + Max( (ulong)0,
		CalcLineWidth(doc_.tl(dp.tl)+rb, (ulong) (dp.ad-rb)) - rlScr_.nPos );
	if( xb < right() )
	{
		RECT rc={xb,yb,right(),yb+H};
		::InvalidateRect( hwnd_, &rc, FALSE );
	}

	// 残り
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
