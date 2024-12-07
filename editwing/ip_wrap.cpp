
#include "../kilib/stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_wrap.cpp   折り返し
//
//		Documentで文字列データが更新されるのを受けて
//		Viewでは折り返し位置情報を更新する。その処理がココ。
//
//---- ip_text.cpp   文字列操作・他
//---- ip_parse.cpp  キーワード解析
//---- ip_scroll.cpp スクロール
//---- ip_draw.cpp   描画・他
//---- ip_cursor.cpp カーソルコントロール
//=========================================================================



//-------------------------------------------------------------------------
// 初期化
//-------------------------------------------------------------------------

ViewImpl::ViewImpl( View& vw, doc::Document& dc )
	: doc_   ( dc )
	, cvs_   ( vw )
	, cur_   ( vw.hwnd(), *this, dc )
	, vlNum_ ( 0 )
	, textCx_( 0 )
	, accdelta_  ( 0 )
	, accdeltax_ ( 0 )
	, hwnd_  ( vw.hwnd() )
{
	// 適当に折り返し情報初期化
	InsertMulti( 0, doc_.tln()-1 );

	// 適当にスクロール情報初期化
	udScr_.cbSize = rlScr_.cbSize = sizeof(udScr_);
	udScr_.fMask  = rlScr_.fMask  = SIF_PAGE | SIF_POS | SIF_RANGE;
	udScr_.fMask |= SIF_DISABLENOSCROLL;
	udScr_.nMin   = rlScr_.nMin   = 0;
	udScr_.nMax   = rlScr_.nMax   = 1;
	udScr_.nPage  = rlScr_.nPage  = 1;
	udScr_.nPos   = rlScr_.nPos   = 0;
	udScr_.nTrackPos = rlScr_.nTrackPos = 0;
	udScr_tl_     = udScr_vrl_    = 0;
	//ReSetScrollInfo();
}

//-------------------------------------------------------------------------
// 状態変更への対応
//-------------------------------------------------------------------------

void ViewImpl::DoResize( bool wrapWidthChanged )
{
	// 折り返し位置再計算
	if( wrapWidthChanged )
	{
		ReWrapAll();
		UpdateTextCx();
	}

	// スクロール情報変更
	ReSetScrollInfo();
	if( wrapWidthChanged )
		ForceScrollTo( udScr_tl_ );

	// 再描画
	ReDraw( ALL );
	cur_.ResetPos();
}

void ViewImpl::DoConfigChange()
{
	// 折り返し位置再計算
	ReWrapAll();
	UpdateTextCx();

	// スクロール情報変更
	ReSetScrollInfo();
	ForceScrollTo( udScr_tl_ );

	// 再描画
	ReDraw( ALL );
	cur_.ResetPos();
}

void ViewImpl::on_text_update
	( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur )
{
	// まず、折り返し位置再計算

	// 置換範囲の先頭行を調整
	int r3 = 0, r2 = 1, r1 = ReWrapSingle( s );

	// 残りを調整
	if( s.tl != e.tl )
		r2 = DeleteMulti( s.tl+1,  e.tl );
	if( s.tl != e2.tl )
		r3 = InsertMulti( s.tl+1, e2.tl );

	// この変更で横幅が…
	// if( "長くなったなてはいない" AND "短くなっちゃった可能性あり" )
	//     横幅再計算();
	if( !(r1==2 || r3==1) && (r1==0 || r2==0) )
		UpdateTextCx();

	// スクロールバー修正
	ReDrawType t = TextUpdate_ScrollBar( s, e, e2 );
	bool doResize = false;

	// 行数に変化があって、行番号表示域の幅を変えなきゃならん時
	if( e.tl!=e2.tl && cvs_.on_tln_change( doc_.tln() ) )
	{
		doResize = true;
	}
	else if( bAft && t!=ALL )
	{
		t = AFTER;
	}

	// カーソル移動
	cur_.on_text_update( s, e, e2, mCur );

	// 再描画
	if( doResize )
		DoResize( true );
	else
	{
		if( e.tl != e2.tl ) // 行番号領域再描画の必要があるとき
			ReDraw( LNAREA, 0 );
		ReDraw( t, &s );
	}
}



//-------------------------------------------------------------------------
// 折り返し位置計算補助ルーチン, Wrap-around position calculation assistance routine
//-------------------------------------------------------------------------

void ViewImpl::UpdateTextCx()
{
	if( cvs_.wrapType() == NOWRAP )
	{
		// 折り返しなしなら、数えてみないと横幅はわからない
		ulong cx=0;
		for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
			if( cx < wrap_[i].width() )
				cx = wrap_[i].width();
		textCx_ = cx;
	}
	else
	{
		// 折り返しありなら、横幅:=折り返し幅とする
		textCx_ = cvs_.wrapWidth();
	}
}

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize ("-O3")
#endif
ulong A_HOT ViewImpl::CalcLineWidth( const unicode* txt, ulong len ) const
{
	// 行を折り返さずに書いたときの横幅を計算する
	// ほとんどの行が折り返し無しで表示されるテキストの場合、
	// この値を計算しておくことで、処理の高速化が可能。
	// Calculate the width of the text when lines are written without wrapping
	// For text where most of the lines are displayed without wrapping.
	// Calculating this value can speed up the process.
	const Painter& p = cvs_.font_;

	ulong w=0;
	for( ulong i=0; i<len ; ++i )
	{
		if( txt[i] < 128 )
			if( txt[i] != L'\t' )
				w += p.Wc( txt[i] );
			else
				w = p.nextTab(w);
		else
			w += p.W( &txt[i] );
	}
	return w;
}
#ifdef __GNUC__
#pragma GCC pop_options
#endif

void ViewImpl::CalcEveryLineWidth()
{
	// 全ての行に対してCalcLineWidthを実行
	// …するだけ。
	for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
		wrap_[i].width() = CalcLineWidth( doc_.tl(i), doc_.len(i) );
}

bool ViewImpl::isSpaceLike(unicode ch) const
{
	static const bool spacelike[128] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0-15
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 16-31
		1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1, //  !"#$%&'()* 32-47
		0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,1, // 48-63
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 64-79
		0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1, // 80-95
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 96-111
		0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1, // 96-127
	};
	// Use Table for ASCII character and also inlcude all C1 control
	// And also all characters beyond 0x2500 (Box Drawing and above)
	// This includes all ideographic characters and all extended planes
	return ch < 128? spacelike[ch]: ch < 0x00a0 || ch >= 0x2500 ;
}

void ViewImpl::ModifyWrapInfo(
		const unicode* txt, ulong len, WLine& wl, ulong stt )
{
	// 設定幅での折り返しを実行する。
	// 行の途中からの変更の場合、sttが開始addressを指している
	const Painter& p = cvs_.font_;
	const bool smart = cvs_.wrapSmart();
	const ulong   ww = smart? cvs_.wrapWidth()-p.W()/2: cvs_.wrapWidth();

	while( stt < len )
	{
		ulong i, w;
		for( w=0,i=stt; i<len; ++i )
		{
			if( txt[i] == L'\t' )
				w = p.nextTab(w);
			else
				w += p.W( &txt[i] );
			if( w > ww )
			{
				if( smart )
				{	// Prefer wrapping on words
					// Back-track to previous spacelike char.
					ulong oi = i;
					ulong ow = w;
					// If we meet a surrogate we will stop
					// and because low surrogate have a zero width
					// they will never be separated from high surrogate.
					while( !isSpaceLike( txt[i] ) && stt < i )
						w -= p.W( &txt[i--] );
					if( stt == i )
						w = ow, i = oi;
					w += p.Wc( txt[i] );
					i++;
				}
				break;
			}
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
	// 折り返し幅に変更があった場合に、全ての行の
	// 折り返し位置情報を変更する。
	// If there is a change in the wrapping width,
	// the wrapping position information for all lines is changed.
	const ulong ww = cvs_.wrapWidth();

	ulong vln=0;
	for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
	{
		WLine& wl = wrap_[i];
		wl.ForceSize(1);

		if( wl.width() < ww )
		{
			// 設定した折り返し幅より短い場合は一行で済む。
			wl.Add( doc_.len(i) );
			++vln;
		}
		else
		{
			// 複数行になる場合
			ModifyWrapInfo( doc_.tl(i), doc_.len(i), wl, 0 );
			vln += wl.rln();
		}
	}
	vlNum_ = vln;
}



//-------------------------------------------------------------------------
// 折り返し位置計算メインルーチン Folding Position Calculation Main Routine
//-------------------------------------------------------------------------

int ViewImpl::ReWrapSingle( const DPos& s )
{
	// 指定した一行のみ折り返しを修正。
	//
	// 返値は
	//   2: "折り返しあり" or "この行が横に一番長くなった"
	//   1: "この行以外のどこかが最長"
	//   0: "さっきまでこの行は最長だったが短くなっちゃった"
	// で、上位ルーチンにm_TextCx修正の必要性を伝える。
	//
	// 昔は再描画範囲の計算のために、表示行数の変化を返していたが、
	// これは上位ルーチン側で vln() を比較すれば済むし、
	// むしろその方が効率的であるため廃止した。


	// 旧情報保存, Save old info
	WLine& wl            = wrap_[s.tl];
	const ulong oldVRNum = wl.rln();
	const ulong oldWidth = wl.width();

	// 横幅更新, Update width
	wl.width() = CalcLineWidth( doc_.tl(s.tl), doc_.len(s.tl) );

	if( wl.width() < cvs_.wrapWidth() )
	{
		// 設定した折り返し幅より短い場合は一行で済む。
		// Only one line is needed
		wl[1] = doc_.len(s.tl);
		wl.ForceSize( 2 );
	}
	else
	{
		// 複数行になる場合, There are multiple lines
		ulong vr=1, stt=0;
		while( wl[vr] < s.ad ) // while( vr行目は変更箇所より手前 )
			stt = wl[ vr++ ];  // stt = 次の行の行頭のアドレス, next line stt

		// 変更箇所以降のみ修正, Corrected only after the point of change
		wl.ForceSize( vr );
		ModifyWrapInfo( doc_.tl(s.tl), doc_.len(s.tl), wl, stt );
	}

	// 表示行の総数を修正, Fix total number of display rows
	vlNum_ += ( wl.rln() - oldVRNum );

	// 折り返しなしだと総横幅の更新が必要
	// Without wrapping, total width needs to be updated
	if( cvs_.wrapType() == NOWRAP )
	{
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
	}
	return 2;
}

int ViewImpl::InsertMulti( ulong ti_s, ulong ti_e )
{
	// 指定した分だけ新しく行情報を追加。
	// ＆折り返し情報もきちんと計算
	//
	// 返値は
	//   1: "折り返しあり" or "この行が横に一番長くなった"
	//   0: "この行以外のどこかが最長"
	// 詳しくは ReWrapSingle() を見よ。

	ulong dy=0, cx=0;
	for( ulong i=ti_s; i<=ti_e; ++i )
	{
		WLine pwl(2);
		pwl.Add( CalcLineWidth( doc_.tl(i), doc_.len(i) ) );

		if( pwl.width() < cvs_.wrapWidth() )
		{
			// 設定した折り返し幅より短い場合は一行で済む。
			pwl.Add( doc_.len(i) );
			dy++;
			if( cx < pwl.width() )
				cx = pwl.width();
		}
		else
		{
			// 複数行になる場合
			ModifyWrapInfo( doc_.tl(i), doc_.len(i), pwl, 0 );
			dy += pwl.rln();
		}

		wrap_.InsertAt( i, pwl );
	}

	// 表示行の総数を修正
	vlNum_ += dy;

	// 折り返しなしだと総横幅の更新が必要
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
	// 指定した範囲の行情報を削除
	//
	// 返値は
	//   1: "折り返しあり" or "この行以外のどこかが最長"
	//   0: "さっきまでこの行は最長だったが短くなっちゃった"
	// 詳しくは ReWrapSingle() を見よ。

	bool  widthChanged = false;
	ulong dy = 0;

	// 情報収集しながら削除
	for( ulong cx=textCx_, i=ti_s; i<=ti_e; ++i )
	{
		WLine& wl = wrap_[i];
		dy += wl.rln();
		if( cx == wl.width() )
			widthChanged = true;
	}
	wrap_.RemoveAt( ti_s, (ti_e-ti_s+1) );

	// 表示行の総数を修正
	vlNum_ -= dy;

	// 折り返しなしだと総横幅の更新が必要
	return ( cvs_.wrapType()==NOWRAP && widthChanged ) ? 0 : 1;
}



//-------------------------------------------------------------------------
// 座標値変換
//-------------------------------------------------------------------------

void ViewImpl::ConvDPosToVPos( DPos dp, VPos* vp, const VPos* base ) const
{
	// 補正, Corrections
	dp.tl = Min( dp.tl, doc_.tln()-1 );
	dp.ad = Min( dp.ad, doc_.len(dp.tl) );

	// 変換の基準点が指定されていなければ、原点を基準とする
	// If no reference point is specified for the conversion,
	// the origin is used as the reference point
	VPos topPos(false); // ０クリア
	if( base == NULL )
		base = &topPos;

	// とりあえずbase行頭の値を入れておく
	// Put the value at the beginning of the BASE line for now
	ulong vl = base->vl - base->rl;
	ulong rl = 0;
	int vx;

	// 同じ行内だった場合, If they were the same line
	//if( dp.tl == base->tl )
	//{
	//	例えば [→] を押したときなど、右隣の文字の横幅を
	//	足すだけで次の位置は算出できる。これを使って普通の
	//	カーソル移動はずっと高速化できるはずであるが、
	//	とりあえず面倒くさいので、今のところ略。
	//}

	// 違う行だった場合, If they were a different line
	//else
	{
		// vlを合わせる, match vl
		ulong tl = base->tl;
		if( tl > dp.tl )      // 目的地が基準より上にある場合, dest above ref
			do
				vl -= rln(--tl);
			while( tl > dp.tl );
		else if( tl < dp.tl ) // 目的地が基準より下にある場合, dest below ref
			do
				vl += rln(tl++);
			while( tl < dp.tl );

		// rlを合わせる match rl
		ulong stt=0;
		while( wrap_[tl][rl+1] < dp.ad )
			stt = wrap_[tl][++rl];
		vl += rl;

		// x座標計算, calculate x cordinate
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
// x座標補正

	x = x - lna() + rlScr_.nPos;

// まず行番号計算

	int tl = udScr_tl_;
	int vl = udScr_.nPos - udScr_vrl_;
	int rl = y / NZero(fnt().H()) + udScr_vrl_;
	if( rl >= 0 ) // View上端より下の場合、下方向を調べる
		while( tl < (int)doc_.tln() && (int)rln(tl) <= rl )
		{
			vl += rln(tl);
			rl -= rln(tl);
			++tl;
		}
	else           // View上端より上の場合、上方向を調べる
		while( 0<=tl && rl<0 )
		{
			vl -= rln(tl);
			rl += rln(tl);
			--tl;
		}

	if( tl == (int)doc_.tln() ) // EOFより下に行ってしまう場合の補正
	{
		--tl, vl-=rln(tl), rl=rln(tl)-1;
		if( linemode )
			x = 0x4fffffff;
	}
	else if( tl == -1 ) // ファイル頭より上に行ってしまう場合の補正
	{
		tl = vl = rl = 0;
		if( linemode )
			x = 0;
	}
	else
	{
		if( linemode ) // 行選択モードの場合
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

// 次に、横位置を計算

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
