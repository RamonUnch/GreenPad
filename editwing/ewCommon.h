#ifndef _EDITWING_COMMON_H_
#define _EDITWING_COMMON_H_
#include "../kilib/kilib.h"
#ifndef __ccdoc__
namespace editwing {
#endif


//=========================================================================
// Unicode�֌W
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
//	�e�L�X�g���̈ʒu���
//@}
//=========================================================================

struct DPos : public ki::Object
{
	//@{ �o�b�t�@���̃A�h���X (0�` ) //@}
	ulong ad;

	//@{ �_���s�ԍ� (0�` ) //@}
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
//	���ꕶ����\���萔�l
//@}
//=========================================================================

enum SpecialChars
{
	scEOF = 0, // EOF
	scEOL = 1, // ���s
	scTAB = 2, // �^�u
	scHSP = 3, // ���p�X�y�[�X
	scZSP = 4  // �S�p�X�y�[�X
};



//=========================================================================
//@{
//	�P��̎�ނ�\���萔�l
//@}
//=========================================================================

enum TokenType
{
	TAB = 0x00, // Tab
	WSP = 0x04, // ���p�X�y�[�X
	ALP = 0x08, // ���ʂ̎�
	 CE = 0x0c, // �R�����g�I���^�O
	 CB = 0x10, // �R�����g�J�n�^�O
	 LB = 0x14, // �s�R�����g�J�n�^�O
	 Q1 = 0x18, // �P����p��
	 Q2 = 0x1c  // ��d���p��
};



//=========================================================================
//@{
//	�F�w��ӏ���\���萔�l
//@}
//=========================================================================

enum ColorType
{
	TXT = 0, // �����F
	CMT = 1, // �R�����g�����F
	KWD = 2, // �L�[���[�h�����F
	//  = 3, // ( �R�����g�A�E�g���ꂽ�L�[���[�h�����F )
	CTL = 4, // ���ꕶ���F
	BG  = 5, // �w�i�F
	LN  = 6  // �s�ԍ�
};



//=========================================================================
//@{
//	�܂�Ԃ��ʒu�������萔�l
//@}
//=========================================================================

enum WrapType
{
	NOWRAP    = -1, // �܂�Ԃ�����
	RIGHTEDGE =  0  // �E�[
};



//=========================================================================
//@{
//	�\���ݒ�
//
//	�t�H���g�E�F�E�^�u���E���ꕶ���̕\���A�̏���ێ��B
//	�������A�����P��̎w��� Document �ɑ΂��čs���B
//@}
//=========================================================================

struct VConfig : public ki::Object
{
	//@{ �t�H���g //@}
	LOGFONT font;
	int fontsize;

	//@{ �^�u�������� //@}
	int tabstep;

	//@{ �F //@}
	COLORREF color[7];

	//@{ ���ꕶ���\�� //@}
	bool sc[5];

	//@{ �댯�ȃf�t�H���g�R���X�g���N�^ //@}
	VConfig() {}

	//@{ �t�H���g�֌W������ //@}
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

	//@{ �t�H���g�֌W�ݒ� //@}
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

	//@{ �^�u���ݒ� //@}
	void SetTabStep( int tab )
	{
		tabstep = Max( 1, tab );
	}
};



//=========================================================================

}      // namespace editwing
#endif // _EDITWING_COMMON_H_
