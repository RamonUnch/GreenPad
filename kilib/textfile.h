#ifndef _KILIB_TEXTFILE_H_
#define _KILIB_TEXTFILE_H_
#include "types.h"
#include "ktlaptr.h"
#include "memory.h"
#include "file.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	���p�\�R�[�h�Z�b�g
//
//	�������A�����Ń��X�g�A�b�v���ꂽ���̂̂����AWindows�ɂ�����
//	����T�|�[�g���C���X�g�[������Ă�����̂��������ۂɂ͑Ή��\�B
//	�l��-100��菬�����R�[�h�́A���̂�����ɂ���R�[�h�y�[�W�̌���
//	�T�|�[�g�𗘗p���ĕϊ����s�����߁A����Ɉˑ�����B
//@}
//=========================================================================

enum charset {
	AutoDetect = 0,    // ��������
	                   // SJIS/EucJP/IsoJP/IsoKR/IsoCN
					   // UTF5/UTF8/UTF8N/UTF16b/UTF16l/UTF32b/UTF32l
					   // �𔻒肷��B���͒m��Ȃ��B(^^;

	ASCIICP    = 20127,// Normal plain ASCII.

	WesternDOS = 850,  // ����      (CP850 != ISO-8859-1)
	Western    = 1252, // ����      (Windows1252 >> ISO-8859-1)
//	WesternOS2 = 1004  // CP1004 OS/2 encoding ~ Windows1252
	WesternMAC = 10000,// ����      (x-mac-roman != ISO-8859-1)
	WesternISO = 28605,// ����      (ISO-8859-15)
	TurkishDOS = 857,  // �g���R��   (CP857 != ISO-8859-9)
	Turkish    = 1254, // �g���R��  (Windows1254 >> ISO-8859-9)
	TurkishMAC = 10081,// �g���R��   (x-mac-turkish != ISO-8859-9)
	TurkishISO = 28599,// �g���R��   (ISO-8859-9)
	EsperantoISO = 28593,// �G�X�y�����g��(ISO-8859-3)
	HebrewDOS  = 862,  // �w�u���C��(CP862 !=> ISO-8859-8)
	Hebrew     = 1255, // �w�u���C��(Windows1255 >> ISO-8859-8)
	HebrewMAC  = 10005,// �w�u���C��(x-mac-hebrew !=> ISO-8859-8)
	ArabicIBM  = 720,  // �A���r�A��(CP720 != ISO-8859-6)
	ArabicMSDOS= 864,  // �A���r�A��(CP864 != ISO-8859-6)
	Arabic     = 1256, // �A���r�A��(Windows1256 �` ISO-8859-6)
	ArabicMAC  = 10004,// �A���r�A��(x-mac-arabic != ISO-8859-6)
	ArabicISO  = 28596,// �A���r�A��(ISO-8859-6)
	BalticIBM  = 775,  // �o���g��  (CP775 != ISO-8859-13)
	Baltic     = 1257, // �o���g��  (Windows1257 >> ISO-8859-13)
	Vietnamese = 1258, // �x�g�i����(Windows1258 != VISCII)
	CentralDOS = 852,  // ����ְۯ��(CP852 != ISO-8859-2)
	Central    = 1250, // ����ְۯ��(Windows1250 �` ISO-8859-2)
	CentralISO = 28592,// ����ְۯ��(ISO-8859-2)
	LatinMAC   = 10029,// ����ְۯ��(x-mac-ce != ISO-8859-2)
	RomaniaMAC = 10010,// ���[�}�j�A��(x-mac-romania != ISO-8859-2)
	CroatiaMAC = 10082,// �N���A�`�A��(x-mac-croatia != ISO-8859-2)
	GreekIBM   = 737,  // �M���V����(CP737 = ISO-8859-7 ?)
	GreekMSDOS = 869,  // �M���V����(CP869 != ISO-8859-7)
	Greek      = 1253, // �M���V����(Windows1253 �` ISO-8859-7)
	GreekMAC   = 10006,// �M���V����(x-mac-greek != ISO-8859-7)
	GreekISO   = 28597,// �M���V����(ISO-8859-7)
	Thai       = 874,  // �^�C��
	ThaiISO    = 28601,  // �^�C��(ISO-8859-11)
	Portuguese = 860,  // �|���g�K���� (CP860)
	Icelandic  = 861,  // �A�C�X�����h�� (CP861)
	IcelandicMAC= 10079,  // �A�C�X�����h�� (x-mac-icelandic)
	CanadianFrench= 863, // �t�����X��(�J�i�_) (CP863)
	Nordic     = 865, // MS-DOS �k�� (CP865)

	CyrillicIBM= 855,  // �L������(IBM) (CP855 = ISO-8859-5 ?)
	CyrillicDOS= 866,  // �L������(MS-DOS) (CP866 != ISO-8859-5)
	Cyrillic   = 1251, // �L������(Windows1251 != ISO-8859-5)
	CyrillicMAC= 10007, // �L������(x-mac-cyrillic != ISO-8859-5)
	CyrillicISO= 28595, // �L������(ISO-8859-5)
	Koi8R      = 20866,// �L������(KOI8-R)
	Koi8U      = 21866,// �L������(KOI8-U �E�N���C�i�n)
	MacUA      = 10017,// �L������(x-mac-ukraine �E�N���C�i�n)

	UHC        = 949,  // �؍���P (Unified Hangle Code >> EUC-KR)
	IsoKR      = -950, // �؍���Q (ISO-2022-KR)
	Johab      = 1361, // �؍���R (Johab)
	MacKR      = 10003,// �؍���S (x-mac-korean)

	GBK        = 936,  // ������P (�ȑ̎� GBK >> EUC-CN)
	IsoCN      = -936, // ������Q (�ȑ̎� ISO-2022-CN)
	HZ         = -937, // ������R (�ȑ̎� HZ-GB2312)
	Big5       = 950,  // ������S (�ɑ̎� Big5)
	CNS        = 20000,// ������T (�ɑ̎� EUC-TW/CNS)
	TCA        = 20001,// ������U (�ɑ̎� TCA)
	ETen       = 20002,// ������V (�ɑ̎� ETen)
	IBM5550    = 20003,// ������W (�ɑ̎� IBM5550)
	Teletext   = 20004,// ������X (�ɑ̎� Teletext)
	Wang       = 20005,// ������P�O (�ɑ̎� Wang)
	GB18030    = 54936,// ������P�P (�ȑ̎� GB18030 >> GBK >> EUC-CN)
	MacTW      = 10002,// ������P�Q (�ɑ̎� x-mac-taiwan �` Big5)
	MacCN      = 10008,// ������P�R (�ȑ̎� x-mac-prc �` GB2312)

	SJIS       = 932,  // ���{��P (Shift_JIS)
	EucJP      = -932, // ���{��Q (���{��EUC)
	IsoJP      = -933, // ���{��R (ISO-2022-JP)
	MacJP      = 10001,// ���{��S (x-mac-japanese)

	UTF1       = -1,   // Unicode  (UTF-1)   : BOM����
	UTF5       = -2,   // Unicode  (UTF-5)   : BOM����
	UTF7       = 65000,// Unicode  (UTF-7)   : BOM����
	UTF8       =-65001,// Unicode  (UTF-8)   : BOM�L��
	UTF8N      = 65001,// Unicode  (UTF-8N)  : BOM����
	UTF16b     = -3,   // Unicode  (UTF-16)  : BOM�L�� BE
	UTF16l     = -4,   // Unicode  (UTF-16)  : BOM�L�� LE
	UTF16BE    = -5,   // Unicode  (UTF-16BE): BOM����
	UTF16LE    = -6,   // Unicode  (UTF-16LE): BOM����
	UTF32b     = -7,   // Unicode  (UTF-32)  : BOM�L�� BE
	UTF32l     = -8,   // Unicode  (UTF-32)  : BOM�L�� LE
	UTF32BE    = -9,   // Unicode  (UTF-32BE): BOM����
	UTF32LE    = -10,  // Unicode  (UTF-32LE): BOM����
	UTF9       = -11,  // Unicode  (UTF-9)   : BOM����
	OFSSUTF    = -12,  // Unicode  (Old FSS-UTF): BOM����
	UTF1Y      =-64999,// Unicode  (UTF-1)   : BOM�L��
	UTF9Y      =-65002,// Unicode  (UTF-9)   : BOM�L��
	OFSSUTFY   = -13,  // Unicode  (Old FSS-UTF): BOM�L��

	DOSUS      = 437,  // DOSLatinUS (CP437)

	SCSU       = -60000,// Standard Compression Scheme for Unicode
	BOCU1      = -60001 // Binary Ordered Compression for Unicode-1
};

//=========================================================================
//@{
//	���s�R�[�h
//@}
//=========================================================================

enum lbcode {
	CR   = 0,
	LF   = 1,
	CRLF = 2
};

struct TextFileRPimpl;
struct TextFileWPimpl;



//=========================================================================
//@{
//	�e�L�X�g�t�@�C���Ǎ�
//
//	�t�@�C�����w�肳�ꂽ�����R�[�h�ŉ��߂��AUnicode������Ƃ���
//	��s���ɕԂ��B�����R�[�h����s�R�[�h�̎���������\�B
//@}
//=========================================================================

class TextFileR : public Object
{
public:

	//@{ �R���X�g���N�^�i�R�[�h�w��j//@}
	explicit TextFileR( int charset=AutoDetect );

	//@{ �f�X�g���N�^ //@}
	~TextFileR();

	//@{ �J�� //@}
	bool Open( const TCHAR* fname, bool always=false );

	//@{ ���� //@}
	void Close();

	//@{
	//	�ǂݍ��� (�ǂ񂾒�����Ԃ�)
	//
	//	���Ȃ��Ƃ�20���炢�̃T�C�Y���m�ۂ����o�b�t�@���w�肵�Ă��������B
	//@}
	size_t ReadBuf( unicode* buf, ulong siz );

public:

	//@{ �ǂ�ł�t�@�C���̃R�[�h�y�[�W //@}
	int codepage() const;

	//@{ ���s�R�[�h (0:CR, 1:LF, 2:CRLF) //@}
	int linebreak() const;

	//@{ �ǂݍ��ݏ� (0:EOF, 1:EOL, 2:EOB) //@}
	int state() const;

	//@{ �t�@�C���T�C�Y //@}
	ulong size() const;

	//@{ ���s�����������Ȃ������t���O //@}
	bool nolb_found() const;

	static int neededCodepage(int cs) A_XPURE;

private:

	TextFileRPimpl    *impl_;
	FileR                fp_;
	int                  cs_;
	int                  lb_;
	bool          nolbFound_;

private:

	int AutoDetection( int cs, const uchar* ptr, ulong siz );
	int MLangAutoDetection( const uchar* ptr, ulong siz );
	int chardetAutoDetection( const uchar* ptr, ulong siz );

	bool IsNonUnicodeRange(qbyte u) const A_XPURE;
	bool IsAscii(uchar c) const A_XPURE;
	bool IsSurrogateLead(qbyte w) const A_XPURE;
	bool CheckUTFConfidence(const uchar* ptr, ulong siz, unsigned int uChrSize, bool LE) const A_XPURE;
	bool IsValidUTF8( const uchar* ptr, ulong siz ) const A_XPURE;

private:

	NOCOPY(TextFileR);
};



//-------------------------------------------------------------------------

inline int TextFileR::codepage() const
	{ return cs_; }

inline int TextFileR::linebreak() const
	{ return lb_; }

inline ulong TextFileR::size() const
	{ return fp_.size(); }

inline bool TextFileR::nolb_found() const
	{ return nolbFound_; }


//=========================================================================
//@{
//	�e�L�X�g�t�@�C������
//
//	Unicode��������󂯎��A�w�肳�ꂽ�����R�[�h�ɕϊ����Ȃ���o�͂���B
//@}
//=========================================================================

class TextFileW : public Object
{
public:

	//@{ �R���X�g���N�^�i����,���s�R�[�h�w��j//@}
	TextFileW( int charset, int linebreak );
	~TextFileW();

	//@{ �J�� //@}
	bool Open( const TCHAR* fname );

	//@{ ���� //@}
	void Close();

	//@{ ��s�����o�� //@}
	void WriteLine( const unicode* buf, ulong siz, bool lastline );

private:

	TextFileWPimpl    *impl_;
	FileW                fp_;
	const int            cs_;
	const int            lb_;

private:

	NOCOPY(TextFileW);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_TEXTFILE_H_
