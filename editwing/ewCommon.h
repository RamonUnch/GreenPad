#ifndef _EDITWING_COMMON_H_
#define _EDITWING_COMMON_H_
#include "../kilib/kilib.h"
#ifndef __ccdoc__

namespace editwing {
#endif


//=========================================================================
// Unicode関係
//=========================================================================

inline bool isHighSurrogate(unicode ch)
{
	return (0xD800 <= ch && ch <= 0xDBFF);
}

inline bool isLowSurrogate(unicode ch)
{
	return (0xDC00 <= ch && ch <= 0xDFFF);
}

//=========================================================================
//@{ @pkg editwing.Common //@}
//@{
//	テキスト中の位置情報
//@}
//=========================================================================

struct DPos : public ki::Object
{
	//@{ バッファ中のアドレス (0～ ) //@}
	ulong ad;

	//@{ 論理行番号 (0～ ) //@}
	ulong tl;

	bool operator == ( const DPos& r ) const
		{ return (tl==r.tl && ad==r.ad); }
	bool operator != ( const DPos& r ) const
		{ return (tl!=r.tl || ad!=r.ad); }
	bool operator <  ( const DPos& r ) const
		{ return (tl<r.tl || (tl==r.tl && ad<r.ad)); }
	bool operator >  ( const DPos& r ) const
		{ return (tl>r.tl || (tl==r.tl && ad>r.ad)); }
	bool operator <= ( const DPos& r ) const
		{ return (tl<r.tl || (tl==r.tl && ad<=r.ad)); }
	bool operator >= ( const DPos& r ) const
		{ return (tl>r.tl || (tl==r.tl && ad>=r.ad)); }

	DPos( ulong t, ulong a ) : ad(a), tl(t) {}
	DPos() {}
};



//=========================================================================
//@{
//	特殊文字を表す定数値
//@}
//=========================================================================

enum SpecialChars
{
	scEOF = 0, // EOF
	scEOL = 1, // 改行
	scTAB = 2, // タブ
	scHSP = 3, // 半角スペース
	scZSP = 4  // 全角スペース
};



//=========================================================================
//@{
//	単語の種類を表す定数値
//@}
//=========================================================================

enum TokenType
{
	TAB = 0x00, // Tab
	WSP = 0x04, // 半角スペース, half-width space
	ALP = 0x08, // 普通の字, normal character
	 CE = 0x0c, // コメント終了タグ, comment end tag
	 CB = 0x10, // コメント開始タグ, comment start tag
	 LB = 0x14, // 行コメント開始タグ, line comment start tag
	 Q1 = 0x18, // 単一引用符, single quote
	 Q2 = 0x1c  // 二重引用符, double quote
};



//=========================================================================
//@{
//	色指定箇所を表す定数値
//@}
//=========================================================================

enum ColorType
{
	TXT = 0, // 文字色
	CMT = 1, // コメント文字色
	KWD = 2, // キーワード文字色
	//  = 3, // ( コメントアウトされたキーワード文字色 )
	CTL = 4, // 特殊文字色
	BG  = 5, // 背景色
	LN  = 6  // 行番号
};



//=========================================================================
//@{
//	折り返し位置を示す定数値
//@}
//=========================================================================

enum WrapType
{
	NOWRAP    = -1, // 折り返し無し
	RIGHTEDGE =  0  // 右端
};



//=========================================================================
//@{
//	表示設定
//
//	フォント・色・タブ幅・特殊文字の表示、の情報を保持。
//	ただし、強調単語の指定は Document に対して行う。
//@}
//=========================================================================

struct VConfig : public ki::Object
{
	//@{ フォント //@}
	LOGFONT font;
	int fontsize;
	int fontwidth;

	//@{ タブ幅文字数, tab width character count //@}
	int tabstep;

	//@{ 色 //@}
	COLORREF color[7];

	//@{ 特殊文字表示 //@}
	bool sc[5];

	//@{ 危険なデフォルトコンストラクタ //@}
	VConfig() {}

	//@{ フォント関係初期化, Initialize font relationship //@}
	VConfig( const TCHAR* fnam, int fsiz )
	{
		SetFont( fnam,fsiz );
		tabstep    = 4;
		color[TXT] =
		color[CMT] =
		color[KWD] =
		color[CTL] = RGB(0,0,0);
		color[ BG] = RGB(255,255,255);
		color[ LN] = RGB(0,0,0);//255,255,0);
		sc[scEOF]  =
		sc[scEOL]  =
		sc[scTAB]  =
		sc[scHSP]  =
		sc[scZSP]  = false;
	}

	//@{ フォント関係設定 //@}
	void SetFont(
		const TCHAR* fnam, int fsiz
		, uchar fontCS=DEFAULT_CHARSET
		, LONG fw=FW_DONTCARE, BYTE ff=0, int fx=0
		, int qual=DEFAULT_QUALITY )
	{
		// Actual font size in points at 72 DPI.
		fontsize              = fsiz;
		fontwidth             = fx;

		ki::mem00( &font, sizeof(font) );
	//	font.lfHeight         = 0; // We do not set lfHeight
	//	font.lfWidth          = 0; // because it must be set for
	//	font.lfEscapement     = 0; // each DC
	//	font.lfOrientation    = 0;
		font.lfWeight         = fw; // FW_DONTCARE;
		font.lfItalic         = ff&1; // FALSE
		font.lfUnderline      = ff&2; // FALSE
		font.lfStrikeOut      = ff&4; // FALSE
		font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
		font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
		font.lfQuality        = qual;
		font.lfPitchAndFamily = VARIABLE_PITCH|FF_DONTCARE;
		font.lfCharSet        = fontCS;

		my_lstrcpyn( font.lfFaceName, fnam, LF_FACESIZE-1 );

		// ki::LOGGER( "VConfig::SetFont() end" );
	}

	//@{ タブ幅設定 //@}
	void SetTabStep( int tab )
	{
		tabstep = Max( 1, tab );
	}
};



//=========================================================================

}      // namespace editwing
#endif // _EDITWING_COMMON_H_
