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

	DPos( ulong t, ulong a ) : tl(t), ad(a) {}
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
	WSP = 0x04, // 半角スペース
	ALP = 0x08, // 普通の字
	 CE = 0x0c, // コメント終了タグ
	 CB = 0x10, // コメント開始タグ
	 LB = 0x14, // 行コメント開始タグ
	 Q1 = 0x18, // 単一引用符
	 Q2 = 0x1c  // 二重引用符
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

	//@{ タブ幅文字数 //@}
	int tabstep;

	//@{ 色 //@}
	COLORREF color[7];

	//@{ 特殊文字表示 //@}
	bool sc[5];

	//@{ 危険なデフォルトコンストラクタ //@}
	VConfig() {}

	//@{ フォント関係初期化 //@}
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
	void SetFont( const TCHAR* fnam, int fsiz )
	{
		SetFont( fnam,fsiz,DEFAULT_CHARSET );
	}
	void SetFont( const TCHAR* fnam, int fsiz, char fontCS )
	{
		SetFont( fnam,fsiz,fontCS,DEFAULT_QUALITY );
	}
	void SetFont( const TCHAR* fnam, int fsiz, char fontCS, int qual )
	{
		fontsize              = fsiz;
		font.lfWidth          = 0;
		font.lfEscapement     = 0;
		font.lfOrientation    = 0;
		font.lfWeight         = FW_DONTCARE;
		font.lfItalic         = FALSE;
		font.lfUnderline      = FALSE;
		font.lfStrikeOut      = FALSE;
		font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
		font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
		font.lfQuality        = qual;
		font.lfPitchAndFamily = VARIABLE_PITCH|FF_DONTCARE;
		font.lfCharSet        = fontCS;

		::lstrcpy( font.lfFaceName, fnam );

		HDC h = ::GetDC( NULL );
		font.lfHeight = -MulDiv(fsiz, ::GetDeviceCaps(h,LOGPIXELSY), 72);
		::ReleaseDC( NULL, h );
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
