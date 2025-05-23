#include "stdafx.h"
#include "app.h"
#include "textfile.h"
#include "ktlarray.h"
#include "kstring.h"
#include "path.h"
using namespace ki;
#define ISHIGHSURROGATE( ch ) ( ( 0xD800 <= (unicode)(ch) && (unicode)(ch) <= 0xDBFF ) )

//=========================================================================
// テキストファイル読み出し共通インターフェイス
//=========================================================================

struct ki::TextFileRPimpl : public TmpObject
{
//	inline TextFileRPimpl() {}

	virtual size_t ReadBuf( unicode* buf, size_t siz )
		= 0;

	virtual ~TextFileRPimpl() {}
};



//-------------------------------------------------------------------------
// Unicode系用のベースクラス
//	UTF-8以外はそんなに出会わないだろうから遅くてもよしとする。
//-------------------------------------------------------------------------

struct rBasicUTF : public ki::TextFileRPimpl
{
	inline rBasicUTF() {}
	virtual unicode PeekC() = 0;
	inline unicode GetC() {unicode ch=PeekC(); Skip(); return ch;}
	virtual void Skip() = 0;
	virtual bool Eof() = 0;
	void SkipBOMIfNeeded()
	{
		if( !Eof() )
		{
			// U+FEFF -> ZERO WIDTH NO-BREAK SPACE
			// Also used as BOM at the start of a file.
			if(PeekC() == 0xfeff) Skip();
		}
	}

	// Helper that returns 0 or the BOM length
	static int GetAfterBOM(const uchar *fb, const uchar *fe, const char *boms, int bl)
	{
		const uchar *bom = (const uchar *)boms;
		if(fb+bl <= fe)
		{
			for(int i=0; i<bl; i++)
			{
				if( fb[i] != bom[i] )
					return 0;
			}
			return bl;
		}

		return 0;
	}

	size_t ReadBuf( unicode* buf, size_t siz ) override A_FINAL
	{
		// 改行が出るまで読む
		unicode *w=buf, *e=buf+siz-1;

		while( !Eof() )
		{
			*w = GetC();

			if( ++w==e )
				break;
		}

		// If the end of the buffer contains half a DOS CRLF
		if(buf<w && *(w-1)==L'\r' && PeekC() == L'\n' )
			Skip();

		// 読んだ文字数
		return w-buf;
	}
};



//-------------------------------------------------------------------------
// UCS2ベタ/UCS4ベタ。それぞれUTF16, UTF32の代わりとして使う。
// ついでに同じtemplateで、ISO-8859-1も処理してしまう。^^;
//-------------------------------------------------------------------------


template<typename T, bool be>
struct rUCS A_FINAL : public rBasicUTF
{
	rUCS( const uchar* b, size_t s )
		: fb( reinterpret_cast<const T*>(b) )
		, fe( reinterpret_cast<const T*>(b+(s/sizeof(T))*sizeof(T)) )
		{ SkipBOMIfNeeded(); }

	const T *fb, *fe;

	// エンディアン変換
	inline  byte swap(  byte val ) { return val; }
	inline dbyte swap( dbyte val ) { return (val<<8) |(val>>8); }

	virtual void Skip() override { ++fb; }
	virtual bool Eof() override { return fb>=fe; }
	virtual unicode PeekC() override { return (unicode)(be ? swap(*fb) : *fb); }

};

typedef rUCS<byte, false> rWestISO88591;
typedef rUCS<dbyte, false> rUtf16LE;
typedef rUCS<dbyte, true> rUtf16BE;


// UTF-32読み込み
template<bool be>
struct rUtf32 A_FINAL: public rBasicUTF
{
	rUtf32( const uchar* b, size_t s )
		: fb( reinterpret_cast<const qbyte*>(b) )
		, fe( reinterpret_cast<const qbyte*>(b+(s/sizeof(qbyte))*sizeof(qbyte)) )
		, state(0)
		{ SkipBOMIfNeeded(); }

	const qbyte *fb, *fe;
	int state;

	qbyte curChar() { return be ? swap(*fb) : *fb; }
	bool inBMP(qbyte c) { return c<0x10000; }

	// エンディアン変換
	inline qbyte swap( qbyte val )
	{
		return ((val>>24)&0xff |
		        (val>>8)&0xff00 |
		        (val<<8)&0xff0000|
		        (val<<24));
	}

	virtual bool Eof() override { return fb>=fe; }
	virtual unicode PeekC() override
	{
		qbyte c = curChar();
		if( inBMP(c) )
			return (unicode)c;
		return (unicode)(state==0 ? 0xD800 + (((c-0x10000) >> 10)&0x3ff)
			                      : 0xDC00 + ( (c-0x10000)       &0x3ff));
	}

	virtual void Skip() override
	{
		if( inBMP(curChar()) )
			++fb;
		else if( state==0 )
			state=1;
		else
			++fb, state=0;
	}
};

typedef rUtf32<true> rUtf32BE;
typedef rUtf32<false> rUtf32LE;

//-------------------------------------------------------------------------
// UTF-1
// https://web.archive.org/web/20150318032101/http://kikaku.itscj.ipsj.or.jp/ISO-IR/178.pdf
//-------------------------------------------------------------------------
struct rUtf1 A_FINAL: public rBasicUTF
{
	rUtf1( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s )
		, SurrogateLow( 0 ) { fb += GetAfterBOM(fb, fe, "\xF7\x64\x4C", 3); }

	const uchar *fb, *fe;
	qbyte SurrogateLow;

	inline byte conv( uchar x )
	{
		if( x<=0x20 )      return x + 0xBE;
		else if( x<=0x7E ) return x - 0x21;
		else if( x<=0x9F ) return x + 0x60;
		else               return x - 0x42;
	}

	bool Eof() override { return SurrogateLow ? false : fb>=fe; }
	void Skip() override
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		if     ( *fb <= 0x9F ) { fb += 1; } /* 00-9F */
		else if( *fb <= 0xF5 ) { fb += 2; } /* A0-F5 */
		else if( *fb <= 0xFB ) { fb += 3; } /* F6-FB */
		else /*if(*fb<= 0xFF)*/{ fb += 5; } /* FC-FF */
	}
	unicode PeekC() override
	{
		qbyte ch;

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}

		if     ( *fb <= 0x9F ) { ch = (*fb); } /* 00-9F */
		else if( *fb == 0xA0 ) { ch = (*(fb+1)); } /* A0 */
		else if( *fb <= 0xF5 ) { ch = ((*fb-0xA1) * 0xBE + conv(*(fb+1)) + 0x100); }
		else if( *fb <= 0xFB ) { ch = ((*fb-0xF6) * 0x8D04 + conv(*(fb+1)) * 0xBE + conv(*(fb+2)) + 0x4016); }
		else/*if(*fb <= 0xFF)*/{ ch = ((*fb-0xFC) * 0x4DAD6810 + conv(*(fb+1)) * 0x68A8F8 + conv(*(fb+2)) * 0x8D04 + conv(*(fb+3)) * 0xBE + conv(*(fb+4)) + 0x38E2E); }

		if( ch >= 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}
};

//-------------------------------------------------------------------------
// UTF-9 (draft-abela-utf9-00), J. Abela, 23 December 1997
// https://www.ietf.org/archive/id/draft-abela-utf9-00.txt
//-------------------------------------------------------------------------
//    UCS-4 range (hex)     UTF-9 octet sequence (binary)
//    0000 0000-0000 007F   0xxxxxxx
//    0000 00A0-0000 00BF   101xxxxx
//    0000 00C0-0000 00FF   11xxxxxx
//    0000 0100-0000 07FF   1000xxxx 1xxxxxxx
//    0000 0800-0000 FFFF   100100xx 1xxxxxxx 1xxxxxxx
//    0001 0000-007F FFFF   100101xx 1xxxxxxx 1xxxxxxx 1xxxxxxx
//    0080 0000-7FFF FFFF   10011xxx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx
struct rUtf9 A_FINAL: public rBasicUTF
{
	rUtf9( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s )
		, SurrogateLow( 0 ) { fb += GetAfterBOM(fb, fe, "\x93\xFD\xFF", 3); }

	const uchar *fb, *fe;
	qbyte SurrogateLow;

	bool Eof() override { return SurrogateLow ? false : fb>=fe; }
	void Skip() override
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		if     ( *fb<=0x7F ) fb+=1; // 00-7F
		else if( *fb>=0xA0 ) fb+=1; // A0-FF
		else if( *fb<=0x8F ) fb+=2; // 80-8F
		else if( *fb<=0x93 ) fb+=3; // 90-93
		else if( *fb<=0x97 ) fb+=4; // 94-97
		else                 fb+=5; // 97-9F
	}
	unicode PeekC() override
	{
		qbyte ch;

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}
		// Trailing byte helper
		#define T(x) (*(fb+x) & 0x7F)
		if     ( *fb<=0x7F ) ch = *fb; // 00-7F
		else if( *fb>=0xA0 ) ch = *fb; // A0-FF
		else if( *fb<=0x8F ) ch = ((*fb & 0x7F) << 7 ) + T(1); // 80-8F
		else if( *fb<=0x93 ) ch = ((*fb & 0x03) << 14) + (T(1) << 7)  + T(2); // 90-94
		else if( *fb<=0x97 ) ch = ((*fb & 0x03) << 21) + (T(1) << 14) + (T(2) << 7)  + T(3); // 94-97
		else                 ch = ((*fb & 0x07) << 28) + (T(1) << 21) + (T(2) << 14) + (T(3) << 7) + T(4); // 94-9F
		#undef T

		if( ch >= 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}
};

//-------------------------------------------------------------------------
// Old FSS-UTF (/usr/ken/utf/xutf from dump of Sep 2 1992)
//    Bits  Hex Min Hex Max  Byte Sequence in Binary
// 1    7  00000000-0000007f 0zzzzzzz
// 2   13  00000080-0000207f 10zzzzzz 1yyyyyyy
// 3   19  00002080-0008207f 110zzzzz 1yyyyyyy 1xxxxxxx
// 4   25  00082080-0208207f 1110zzzz 1yyyyyyy 1xxxxxxx 1wwwwwww
// 5   31  02082080-7fffffff 11110zzz 1yyyyyyy 1xxxxxxx 1wwwwwww 1vvvvvvv
//-------------------------------------------------------------------------
struct rUtfOFSS A_FINAL: public rBasicUTF
{
	rUtfOFSS( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s )
		, SurrogateLow( 0 ) { fb += GetAfterBOM(fb, fe, "\xC3\xBC\xFF",3); }

	const uchar *fb, *fe;
	qbyte SurrogateLow;

	bool Eof() override { return SurrogateLow ? false : fb>=fe; }
	void Skip() override
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		if      ( *fb < 0x80 ) fb+=1; // 00-7F
		else if ( *fb < 0xC0 ) fb+=2; // 80-BF
		else if ( *fb < 0xE0 ) fb+=3; // C0-DF
		else if ( *fb < 0xF0 ) fb+=4; // E0-EF
		else if ( *fb < 0xF8 ) fb+=5; // F0-F7
		else /* fb <= 0xFF*/   fb+=1; // F8-FF BAD! skip.
	}
	unicode PeekC() override
	{
		qbyte ch;

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}

		#define T(x) (*(fb+x) & 0x7F)
		if      ( *fb < 0x80 ) ch = *fb; // 00-7F
		else if ( *fb < 0xC0 ) ch = ((*fb & 0x3f) << 7)  + T(1) + 0x0000080;
		else if ( *fb < 0xE0 ) ch = ((*fb & 0x1f) << 14) + (T(1) << 7) + T(2) + 0x0002080;
		else if ( *fb < 0xF0 ) ch = ((*fb & 0x0f) << 21) + (T(1) << 14) + (T(2) << 7) + T(3) + 0x0082080;
		else if ( *fb < 0xF8 ) ch = ((*fb & 0x07) << 28) + (T(1) << 21) + (T(2) << 14) + (T(3) << 7) + T(4) + 0x2082080;
		else /* fb <= 0xFF*/   ch = *fb; // F8-FF Invalid sequence, just copy...
		#undef T

		if( ch >= 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}
};

//-------------------------------------------------------------------------
// UTF-5
// P.-C. Wur, Computer Standards & Interfaces, 23 2001 457-466
// DOI: 10.1016/S0920-5489(01)00086-1
// https://archive.ph/20120721050018/http://tools.ietf.org/html/draft-jseng-utf5
//     0-  F : 1bbbb
//    10- FF : 1bbbb 0bbbb
//   100-FFF : 1bbbb 0bbbb 0bbbb
// というように、16進での一桁を一文字で表していくフォーマット。
// 各 0bbbb は '0', '1', ... '9', 'A', ... 'F'
// 各 1bbbb は 'G', 'H', ... 'P', 'Q', ... 'V' の字で表現。
//-------------------------------------------------------------------------

struct rUtf5 A_FINAL: public rBasicUTF
{
	rUtf5( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s ) { fb += GetAfterBOM(fb, fe, "\x56\x45\x46\x46", 4); }

	const uchar *fb, *fe;

	// 16進文字から整数値へ変換
	inline byte conv( uchar x )
	{
		if(/*'0'<=x &&*/x<='9' ) return x-'0';
		else                   return x-'A'+0x0A;
	}

	void Skip() override { do ++fb; while( fb<fe && *fb<'G' ); }
	bool Eof() override { return fb>=fe; }
	unicode PeekC() override
	{
		unicode ch = (*fb-'G');
		for( const uchar* p=fb+1; p<fe && *p<'G'; ++p )
			ch = (ch<<4)|conv(*p);
		return ch;
	}
};



//-------------------------------------------------------------------------
// UTF-7
//   ASCII範囲の字はそのまま。それ以外はUTF-16の値をbase64エンコード
//   して出力。エンコードされた部分は + と - で挟まれる。また '+' と
//   いう字自体を表現するために "+-" という形式を用いる。
//-------------------------------------------------------------------------

namespace
{
	static const uchar u7c[128]={
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3e,0xff,0xff,0xff,0x3f,
	0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
	0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0xff,0xff,0xff,0xff,0xff,
	0xff,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
	0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0xff,0xff,0xff,0xff,0xff };
}

struct rUtf7 A_FINAL: public rBasicUTF
{
	rUtf7( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s )
		, rest( -1 )
		, inB64( false )
		{ fillbuf();  /*SkipBOMIfNeeded();*/ }

	const uchar *fb, *fe;
	unicode buf[3]; // b64を８文字毎に読んでバッファに溜めておく
	int rest;       // バッファの空き
	bool inB64;     // base64エリア内ならtrue

	void Skip() override { if(--rest==0) fillbuf(); }
	bool Eof() override { return fb>=fe && rest==0; }
	unicode PeekC() override { return buf[rest-1]; }

	void fillbuf()
	{
		if( fb<fe )
		{
			if( !inB64 )
				if( *fb=='+' )
					if( fb+1<fe && fb[1]=='-' )
						rest=1, buf[0]=L'+', fb+=2;  // +-
					else
						++fb, inB64=true, fillbuf(); // 単独 +
				else
					rest=1, buf[0]=*fb++;            // 普通の字
			else
			{
				// 何文字デコードできるか数える
				int N=0, E=Max(int(fb-fe),8);
				while( N<E && fb[N]<0x80 && u7c[fb[N]]!=0xff )
					++N;

				// デコード
				buf[0]=buf[1]=buf[2]=0;
				switch( N )
				{
				case 8: buf[2]|= u7c[fb[7]];
				case 7: buf[2]|=(u7c[fb[6]]<< 6);
				case 6: buf[2]|=(u7c[fb[5]]<<12), buf[1]|=(u7c[fb[5]]>>4);
				case 5: buf[1]|=(u7c[fb[4]]<< 2);
				case 4: buf[1]|=(u7c[fb[3]]<< 8);
				case 3: buf[1]|=(u7c[fb[2]]<<14), buf[0]|=(u7c[fb[2]]>>2);
				case 2: buf[0]|=(u7c[fb[1]]<< 4);
				case 1: buf[0]|=(u7c[fb[0]]<<10);
					unicode t;
					rest = 1;
					if( N==8 )
						rest=3, t=buf[0], buf[0]=buf[2], buf[2]=t;
					else if( N>=6 )
						rest=2, t=buf[0], buf[0]=buf[1], buf[1]=t;
				}

				// 使った分進む
				if( N<E )
				{
					inB64=false;
					if( fb[N]=='-' )
						++fb;
				}
				fb += N;
				if( N==0 )
					fillbuf();
			}
		}
	}
};

//-------------------------------------------------------------------------
// SCSU (UTR #6)
// Code portion is taken from:
// http://czyborra.com/scsu/scsu.c written by Roman Czyborra@dds.nl
//-------------------------------------------------------------------------
namespace
{
	static const WORD SCSU_win[256]={
	0x0000, 0x0080, 0x0100, 0x0180, 0x0200, 0x0280, 0x0300, 0x0380,
	0x0400, 0x0480, 0x0500, 0x0580, 0x0600, 0x0680, 0x0700, 0x0780,
	0x0800, 0x0880, 0x0900, 0x0980, 0x0A00, 0x0A80, 0x0B00, 0x0B80,
	0x0C00, 0x0C80, 0x0D00, 0x0D80, 0x0E00, 0x0E80, 0x0F00, 0x0F80,
	0x1000, 0x1080, 0x1100, 0x1180, 0x1200, 0x1280, 0x1300, 0x1380,
	0x1400, 0x1480, 0x1500, 0x1580, 0x1600, 0x1680, 0x1700, 0x1780,
	0x1800, 0x1880, 0x1900, 0x1980, 0x1A00, 0x1A80, 0x1B00, 0x1B80,
	0x1C00, 0x1C80, 0x1D00, 0x1D80, 0x1E00, 0x1E80, 0x1F00, 0x1F80,
	0x2000, 0x2080, 0x2100, 0x2180, 0x2200, 0x2280, 0x2300, 0x2380,
	0x2400, 0x2480, 0x2500, 0x2580, 0x2600, 0x2680, 0x2700, 0x2780,
	0x2800, 0x2880, 0x2900, 0x2980, 0x2A00, 0x2A80, 0x2B00, 0x2B80,
	0x2C00, 0x2C80, 0x2D00, 0x2D80, 0x2E00, 0x2E80, 0x2F00, 0x2F80,
	0x3000, 0x3080, 0x3100, 0x3180, 0x3200, 0x3280, 0x3300, 0x3380,
	0xE000, 0xE080, 0xE100, 0xE180, 0xE200, 0xE280, 0xE300, 0xE380,
	0xE400, 0xE480, 0xE500, 0xE580, 0xE600, 0xE680, 0xE700, 0xE780,
	0xE800, 0xE880, 0xE900, 0xE980, 0xEA00, 0xEA80, 0xEB00, 0xEB80,
	0xEC00, 0xEC80, 0xED00, 0xED80, 0xEE00, 0xEE80, 0xEF00, 0xEF80,
	0xF000, 0xF080, 0xF100, 0xF180, 0xF200, 0xF280, 0xF300, 0xF380,
	0xF400, 0xF480, 0xF500, 0xF580, 0xF600, 0xF680, 0xF700, 0xF780,
	0xF800, 0xF880, 0xF900, 0xF980, 0xFA00, 0xFA80, 0xFB00, 0xFB80,
	0xFC00, 0xFC80, 0xFD00, 0xFD80, 0xFE00, 0xFE80, 0xFF00, 0xFF80,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x00C0, 0x0250, 0x0370, 0x0530, 0x3040, 0x30A0, 0xFF60};

	static int SCSU_start[8]={0x0000,0x0080,0x0100,0x0300,0x2000,0x2080,0x2100,0x3000},
	           SCSU_slide[8]={0x0080,0x00C0,0x0400,0x0600,0x0900,0x3040,0x30A0,0xFF00};
}
struct rSCSU A_FINAL: public rBasicUTF
{
	rSCSU( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s )
		, skip( 0 )
		, active( 0 )
		, mode( 0 )
		, c( 0 )
		, d( 0 )
		{
			// Skip 0E FE FF BOM if needed!
			fb += GetAfterBOM(fb, fe, "\x0e\xfe\xff", 3);
		}

	const uchar *fb, *fe;
	size_t skip;
	uchar active, mode;
	uchar c, d;

	void Skip() override { fb+=skip; skip=0; }
	bool Eof() override { return fb>=fe; }
	uchar GetChar() { return fb+skip>fe ? 0 : *(fb+(skip++)); }
	unicode PeekC() override
	{
		c = GetChar();
		if (!mode && c >= 0x80)
		{
			return (c - 0x80 + SCSU_slide[active]);
		}
		else if (!mode && c >= 0x20 && c <= 0x7F)
		{
			return c;
		}
		else if (!mode && c == 0x0 || c == 0x9 || c == 0xA || c == 0xC || c == 0xD)
		{
			return c;
		}
		else if (!mode && c >= 0x1 && c <= 0x8) /* SQn */
		{ // single quote
			d = GetChar();
			return (d < 0x80 ? d + SCSU_start[c - 0x1] : d - 0x80 + SCSU_slide[c - 0x1]);
		}
		else if (!mode && c >= 0x10 && c <= 0x17) /* SCn */
		{ // change window
			active = c - 0x10;

			Skip();
			return PeekC();
		}
		else if (!mode && c >= 0x18 && c <= 0x1F) /* SDn */
		{ // define window
			active = c - 0x18;
			SCSU_slide[active] = SCSU_win[GetChar()];

			Skip();
			return PeekC();
		}
		else if (!mode && c == 0xB) /* SDX */
		{
			c = GetChar(); d = GetChar();
			SCSU_slide[active = c>>5] = 0x10000 + (((c & 0x1F) << 8 | d) << 7);

			Skip();
			return PeekC();
		}
		else if (!mode && c == 0xE) /* SQU */
		{
			c = GetChar();
			return (c << 8 | GetChar());
		}
		else if (mode || c == 0xF) /* SCU */
		{ // change to Unicode mode

			if(!mode) c = GetChar();
			mode = 1;

			if (c <= 0xDF || c >= 0xF3)
			{
				return (c << 8 | GetChar());
			}
			else if (c == 0xF0) /* UQU */
			{
				c = GetChar();
				return (c << 8 | GetChar());
			}
			else if (/*c >= 0xE0 &&*/ c <= 0xE7) /* UCn */
			{
				active = c - 0xE0; mode = 0;

				Skip();
				return PeekC();
			}
			else if (/*c >= 0xE8 &&*/ c <= 0xEF) /* UDn */
			{
				SCSU_slide[active=c-0xE8] = SCSU_win[GetChar()]; mode = 0;

				Skip();
				return PeekC();
			}
			else if (c == 0xF1) /* UDX */
			{
				c = GetChar(); d = GetChar();
				SCSU_slide[active = c>>5] = 0x10000 + (((c & 0x1F) << 8 | d) << 7); mode = 0;

				Skip();
				return PeekC();
			}
			else
			{
				Skip();
				return PeekC();
			}
		}
		else
		{
			Skip();
			return PeekC();
		}
	}
};

//-------------------------------------------------------------------------
// BOCU-1
// code portion from BOCU1.pm by Naoya Tozuka
//-------------------------------------------------------------------------
#define m1 ((uchar)(-1))
namespace {
	static const uchar bocu1_byte_to_trail[256] = {
//   0x00 - 0x20
	m1,   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, m1,   m1,   m1,   m1,   m1,   m1,   m1,   m1,   m1,
	0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, m1,   m1,   0x10, 0x11, 0x12, 0x13,
	m1,
//   0x21 - 0xff
	      0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
	0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32,
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42,
	0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52,
	0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62,
	0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
	0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92,
	0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2,
	0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2,
	0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2,
	0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2 };
}
#undef m1
struct rBOCU1 A_FINAL: public rBasicUTF
{
	rBOCU1( const uchar* b, size_t s )
		: fb( b )
		, fe( b+s )
		, skip( 0 )
		, cp( 0 )
		, pc( 0x40 )
		{
			// Skip BOM if needed!
			fb += GetAfterBOM(fb, fe, "\xfb\xee\x28", 3);
		}

	const uchar *fb, *fe;
	size_t skip;
	unicode cp, pc;

	void Skip() override { fb+=skip; skip=0; }
	bool Eof() override { return fb>=fe; }
	uchar GetChar() { return (fb+skip < fe) ? *(fb+(skip++)) : 0; }
	unicode PeekC() override
	{
		uchar c = GetChar();
		long diff = 0;
		uchar t1,t2,t3;

		if (c <= 0x20) {
			cp = c;
		} else if (c == 0x21) { // 21 t1 t2 t3
			t1 = bocu1_byte_to_trail[ GetChar() ];
			t2 = bocu1_byte_to_trail[ GetChar() ];
			t3 = bocu1_byte_to_trail[ GetChar() ];
			//croak "illegal trail char" if t1 < 0 || t2 < 0 || t3 < 0;
			diff = 14161247 + t1 * 59049 + t2 * 243 + t3;
		} else if (c < 0x25) { // [22-24] t1 t2
			t1 = bocu1_byte_to_trail[ GetChar() ];
			t2 = bocu1_byte_to_trail[ GetChar() ];
			//croak "illegal trail char" if t1 < 0 || t2 < 0;
			diff = -2195326 + c * 59049 + t1 * 243 + t2;
		} else if (c < 0x50) { // [25-4F] t1
			t1 = bocu1_byte_to_trail[ GetChar() ];
			//croak "illegal trail char" if t1 < 0;
			diff = -19504 + c * 243 + t1;
		} else if (c < 0xd0) { // [50-CF]
			diff = c - 0x90;
		} else if (c < 0xfb) { // [D0-FA] t1
			t1 = bocu1_byte_to_trail[ GetChar() ];
			//croak "illegal trail char" if t1 < 0;
			diff = -50480 + c * 243 + t1;
		} else if (c < 0xfe) { // [FB-FD] t1 t2
			t1 = bocu1_byte_to_trail[ GetChar() ];
			t2 = bocu1_byte_to_trail[ GetChar() ];
			//croak "illegal trail char" if t1 < 0 || t2 < 0;
			diff = -14810786 + c * 59049 + t1 * 243 + t2;
		} else if (c == 0xfe) { // FE t1 t2 t3
			t1 = bocu1_byte_to_trail[ GetChar() ];
			t2 = bocu1_byte_to_trail[ GetChar() ];
			t3 = bocu1_byte_to_trail[ GetChar() ];
			//croak "illegal trail char" if t1 < 0 || t2 < 0 || t3 < 0;
			diff = 187660 + t1 * 59049 + t2 * 243 + t3;
		} else if (c == 0xff) {
			// reset
			cp = 0;
			diff = 0;
		}

		// codepoint, next pc
		if (c <= 0x20) {
			if (c < 0x20) pc = 0x40;
			//push(@codepoints,c);
			return c;
		} else if (c < 0xff) {
			cp = (unicode)(pc + diff);
			if (pc + diff < 0) cp = 0;
			//push(@codepoints,cp);
			if (cp < 0x20) {
				pc = 0x40;
			} else if (cp == 0x20) {
				// keep pc
			} else if (0x3040 <= cp && cp <= 0x309f) {
				pc = 0x3070;
			} else if (0x4e00 <= cp && cp <= 0x9fa5) {
				pc = 0x7711;
			} else if (0xac00 <= cp && cp <= 0xd7a3) {
				pc = 0xc1d1;
			} else {
				pc = (cp & ~0x7f) + 0x40;
			}
			return cp;
		} else { // 0xff : reset
			pc = 0x40;

			Skip();
			return PeekC();
		}
	}
};

//-------------------------------------------------------------------------
// UTF-EBCDIC
// Source: https://www.unicode.org/reports/tr16/tr16-8.html
//-------------------------------------------------------------------------
// Step 1: Apply the ubcd_to_i8[] table
// Encoding is based on CP1047 (Western), similar to CP37
// Step2: Decode UTF-8-Mod I8
// 0-7F         00000000000000xxxxxxx 0xxxxxxx
// 80-9F        0000000000000100xxxxx 100xxxxx
// A0-3FF       00000000000yyyyyxxxxx 110yyyyy 101xxxxx
// 400-3FFF     0000000zzzzyyyyyxxxxx 1110zzzz 101yyyyy 101xxxxx
// 4000-3FFFF   000wwwzzzzzyyyyyxxxxx 11110www 101zzzzz 101yyyyy 101xxxxx
// 40000-10FFFF rwwwwwzzzzzyyyyyxxxxx 1111100r 101wwwww 101zzzzz 101yyyyy 101xxxxx

namespace
{

static const uchar ubcd_to_i8[256] = {
	 0x00,0x01,0x02,0x03,0x9c,0x09,0x86,0x7f,0x97,0x8d,0x8e,0x0b,0x0c,0x0d,0x0e,0x0f
	,0x10,0x11,0x12,0x13,0x9d,0x0a,0x08,0x87,0x18,0x19,0x92,0x8f,0x1c,0x1d,0x1e,0x1f
	,0x80,0x81,0x82,0x83,0x84,0x85,0x17,0x1b,0x88,0x89,0x8a,0x8b,0x8c,0x05,0x06,0x07
	,0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9a,0x9b,0x14,0x15,0x9e,0x1a
	,0x20,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0x2e,0x3c,0x28,0x2b,0x7c
	,0x26,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0x21,0x24,0x2a,0x29,0x3b,0x5e
	,0x2d,0x2f,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0x2c,0x25,0x5f,0x3e,0x3f
	,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0x60,0x3a,0x23,0x40,0x27,0x3d,0x22
	,0xc5,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xc6,0xc7,0xc8,0xc9,0xca,0xcb
	,0xcc,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0xcd,0xce,0xcf,0xd0,0xd1,0xd2
	,0xd3,0x7e,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0xd4,0xd5,0xd6,0x5b,0xd7,0xd8
	,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0x5d,0xe6,0xe7
	,0x7b,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xe8,0xe9,0xea,0xeb,0xec,0xed
	,0x7d,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0xee,0xef,0xf0,0xf1,0xf2,0xf3
	,0x5c,0xf4,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
	,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xfb,0xfc,0xfd,0xfe,0xff,0x9f
};

}
struct rUtfEBCDIC A_FINAL: public rBasicUTF
{
	rUtfEBCDIC( const uchar* b, size_t s )
	: fb( b )
	, fe( b+s )
	, SurrogateLow( 0 )
	{
		fb += GetAfterBOM(fb, fe, "\xdd\x73\x66\x73", 4);
	}

	const uchar *fb, *fe;
	qbyte SurrogateLow;
	bool Eof() override { return SurrogateLow ? false : fb>=fe; }

	void Skip() override
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		uchar c = ubcd_to_i8[ *fb ];
		if     ( c <= 0x9F ) fb+=1; // 00000000-10011111
		else if( c <= 0xDF ) fb+=2; // 10100000-11011111
		else if( c <= 0xEF ) fb+=3; // 11100000-11101111
		else if( c <= 0xF7 ) fb+=4; // 11110000-11110111
		else if( c <= 0xFB ) fb+=5; // 11111000-11111011 / Last needed
		else if( c <= 0xFD ) fb+=6; // 11111100-11111101 * (FC,FD)
		else/*if(c <= 0XFF)*/fb+=7; // 11111110-11111111 * (FE,FF)
	}
	unicode PeekC() override
	{
		qbyte ch;
		uchar c = ubcd_to_i8[ *fb ];

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}

		// Get trailing bits (5bis at a time to be shifted).
		#define T(i) ( ubcd_to_i8[fb[i]]&0x1f )

		if     ( c <= 0x9F ) ch = c;
		else if( c <= 0xDF ) ch = (c&0x1f)<<5  | T(1);
		else if( c <= 0xEF ) ch = (c&0x0f)<<10 | T(1)<<5 | T(2);
		else if( c <= 0xF7 ) ch = (c&0x07)<<15 | T(1)<<10| T(2)<<5  | T(3);
		else if( c <= 0xF9 ) ch = (c&0x03)<<20 | T(1)<<15| T(2)<<10 | T(3)<<5  | T(4);
		else if( c <= 0xFD ) ch = (c&0x01)<<25 | T(1)<<20| T(2)<<15 | T(3)<<10 | T(4)<<5  | T(5);
		else/*if(c <= 0xFF)*/ch = (c&0x01)<<30 | T(1)<<25| T(2)<<20 | T(3)<<15 | T(4)<<10 | T(5)<<5 | T(6);
		#undef T

		if( ch >= 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}

};

//-------------------------------------------------------------------------
// UTF8/MBCS
//  CR,LFが１バイト文字としてきちんと出てくるので、
//  切り分けが簡単な形式をここでまとめて扱う。UTF8以外の変換は
//  Windowsに全て任せている。
//-------------------------------------------------------------------------

namespace
{
	typedef char* (WINAPI * uNextFunc)(WORD,const char*,DWORD);

	// CharNextExAはGB18030の４バイト文字列を扱えないそうだ。
	static char* WINAPI CharNextGB18030( WORD, const char* sp, DWORD )
	{
		const unsigned char *q;
		const unsigned char *p = reinterpret_cast<const unsigned char *>(sp);
		if (!(*p & 0x80) || *p == 0x80 || *p == 0xFF || // ASCII, Euro sign, EOF
			!p[1] || p[1] == 0xFF || p[1] < 0x30) // invalid DBCS
		{
			q = p+1;
		}
		else if (/*p[1] >= 0x30 &&*/ p[1] <= 0x39) // 4BCS leading
		{
			if (p[2] && p[3] && (p[2] & 0x80) && p[2] != 0x80 && p[2] != 0xFF
				&& p[3] >= 0x30 && p[3] <= 0x39 &&
				(( *p >= 0x81 && *p <= 0x84) ||
				 ( *p >= 0x90 && *p <= 0xE3)))
				q = p+4;
			else
				q = p+1;
		}
		else // DBCS
		{
			q = p+2;
		}
		return (char *)( q );
	}

	// IMultiLanguage2::DetectInputCodepageはGB18030のことを認識できません。
	static bool IsGB18030Like( const uchar* ptr, size_t siz, int refcs )
	{
		size_t i;
		int qbcscnt = 0; // valid Quad Byte Char Seq count
		int dbcscnt = 0; // valid Double Byte Char Seq count
		int invcnt = 0;  // invalid char seq count
		for (i = 0; i < siz;)
		{
			if (!ptr[i]) // NULL
				invcnt++;
			if (ptr[i] <= 0x80 || ptr[i] == 0xFF)
				i++;
			else if (i < siz-1)
			{
				if (ptr[i+1] < 0x40) // non-GBK
				{
					if (ptr[i+1] < 0x30) // non-GB18030
					{
						invcnt++;
						i++;
					}
					else if (i < siz-3)
					{
						if( ptr[i+2] > 0x80 && ptr[i+2] < 0xFF
						&&  ptr[i+3] >= 0x30 && ptr[i+3] <= 0x39
						&&((/*ptr[i] >= 0x81 &&*/ ptr[i] <= 0x84)
							|| (ptr[i] >= 0x90 && ptr[i] <= 0xE3)) )
						{
							qbcscnt++;
							i += 4;
						}
						else // invalid 4BCS
						{
							invcnt++;
							i++;
						}
					}
					else // incomplete, ignore
						i++;
				}
				else if (ptr[i+1] != 0x7F && ptr[i+1] != 0xFF)
				{
					dbcscnt++;
					i += 2;
				}
				else // invalid DBCS
				{
					invcnt++;
					i++;
				}
			}
			else // incomplete, ignore
				i++;
		}
		if (qbcscnt)
			return !invcnt || (invcnt < (((qbcscnt << 1) + dbcscnt) >> 3));
		return dbcscnt && (refcs>950) && (!invcnt || (invcnt < (dbcscnt >> 5)));
	}

	// UTF-8 byte sequence length
	static inline int GetMaskIndex(uchar n)
	{
		if( uchar(n+2) < 0xc2 ) return 1; // 00〜10111111, fe, ff
		if( n          < 0xe0 ) return 2; // 110xxxxx
		if( n          < 0xf0 ) return 3; // 1110xxxx
		if( n          < 0xf8 ) return 4; // 11110xxx
		if( n          < 0xfc ) return 5; // 111110xx
		return 6; // 1111110x
	}

	// Win95対策。
	//   http://support.microsoft.com/default.aspx?scid=%2Fisapi%2Fgomscom%2Easp%3Ftarget%3D%2Fjapan%2Fsupport%2Fkb%2Farticles%2Fjp175%2F3%2F92%2Easp&LN=JA
	// MSDNにはWin95以降でサポートと書いてあるのにCP_UTF8は
	// 使えないらしいので、自前の変換関数で。
	static uchar next_LUT[256]; // Filled by GetCharNextExA()
	typedef int (WINAPI * uConvFunc)(UINT,DWORD,const char*,int,wchar_t*,int);
	static int WINAPI Utf8ToWideChar( UINT, DWORD, const char* sb, int ss, wchar_t* wb, int ws )
	{
		static const byte mask[] = { 0, 0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

		const uchar *p = reinterpret_cast<const uchar*>(sb);
		const uchar *e = reinterpret_cast<const uchar*>(sb+ss);
		wchar_t     *w = wb; // バッファサイズチェック無し（仕様）

		for( int t; p<e; ++w )
		{
			t = next_LUT[(uchar)*p];
			qbyte qch = (*p++ & mask[t]);
			while( p<e && --t )
				qch<<=6, qch|=(*p++)&0x3f;
			if(qch<0x10000)
				*w = (wchar_t)qch;
			else
				*w++ = (wchar_t)(0xD800 + (((qch-0x10000)>>10)&0x3ff)),
				*w   = (wchar_t)(0xDC00 + (((qch-0x10000)    )&0x3ff));
		}
		return int(w-wb);
	}
}
struct rMBCS A_FINAL: public TextFileRPimpl
{
	// ファイルポインタ＆コードページ, File Pointer & Code Page
	const char* fb;
	const char* fe;
	const int   cp;
	const uNextFunc next;
	const uConvFunc conv;
	enum { ANSIMODE=0, UTF8MODE=1, DBCSMODE=2 };

	// 初期設定, Initialization
	rMBCS( const uchar* b, size_t s, int c )
		: fb( reinterpret_cast<const char*>(b) )
		, fe( reinterpret_cast<const char*>(b+s) )
		, cp( c==UTF8 ? UTF8N : c==GB18030Y ? GB18030 : c )
		, next( GetCharNextExA( cp ) )
		, conv( cp==UTF8N && (app().isWin95()||!::IsValidCodePage(65001))
		                  ? Utf8ToWideChar : MultiByteToWideChar )
	{
		if( cp==UTF8N ) // BOMスキップ
			fb += rBasicUTF::GetAfterBOM((uchar*)fb, (uchar*)fe, "\xEF\xBB\xBF", 3);
		if( cp==GB18030 )
			fb += rBasicUTF::GetAfterBOM((uchar*)fb, (uchar*)fe, "\x84\x31\x95\x33", 4);
	}

	size_t ReadBuf( unicode* buf, size_t siz ) override
	{
		// バッファの終端か、ファイルの終端の近い方まで読み込む
		// Read to the end of the buffer or near the end of the file
		const char *p, *end = Min( fb+siz/2-2, fe );

		// 改行が出るまで進む
		p=fb;
		switch( (UINT_PTR)next )
		{
		case ANSIMODE:
			// Simple ANSI mode, no need to pre-parse
			// because we only have 1byte per character.
			p = end;
			break;

		case UTF8MODE:
			// In UTF-8 mode, we can skip to almost the end of the
			// Buffer because it is self synchronising.
			// 6 should be enough but I put 8 for safety.
			p = Max(end-8, fb);
			// Fall through!
		case DBCSMODE:
			// We can use the internal next_LUT[]
			for( ; p<end; )
				p += next_LUT[(uchar)(*p)];
			break;

		default:
			// Complex MultyByte encoding.
			for( ; p<end; )
			{
				if( (*p) & 0x80 && p+1<fe )
					p = next(cp,p,0);
				else
					++p;
			}
		}

		// Guard for invalid UTF8/GB18030 sequences.
		// because next() can make a large step
		if( p > fe )
			p=fe;

		// If the end of the buffer contains half a DOS CRLF
		// we do not translate it.
		const char *pp = p - ( p < fe && p > fb && *(p-1)=='\r' && *(p) =='\n' );

		// Unicodeへ変換, convertion to Unicode
		size_t len = conv( cp, 0, fb, pp-fb, buf, siz );

		fb = p;

		// 終了
		return len;
	}

	uNextFunc GetCharNextExA(WORD cp)
	{
		if( cp==GB18030 ) return CharNextGB18030;
		if( cp==UTF8N )
		{
			for( int i=0; i<= 0xff; i++ )
				next_LUT[i] = GetMaskIndex( i );
			return (uNextFunc)UTF8MODE; // UTF-8 mode
		}

		CPINFO cpnfo;
		if( GetCPInfo(cp, &cpnfo) )
		{
			//MessageBox(NULL, SInt2Str(cpnfo.MaxCharSize).c_str(), SInt2Str(cpnfo.LeadByte[0]).c_str(),0 );
			if( cpnfo.MaxCharSize == 1 && cpnfo.LeadByte[0] == 0 )
			{
				// Simple ANSI codepage, no double byte sequences.
				return (uNextFunc)ANSIMODE;
			}

			// Double byte character set
			if (cpnfo.MaxCharSize == 2 && cpnfo.LeadByte[0] != 0 )
			{
				int i;
				for( i=0; i <= 0xff; ++i )
					next_LUT[i] = 1;
				i=0;
				// Fill next_LUT from the double NULL terminated ranges.
				// s1 e1 s2 e2 s3 e3... 0 0 (Maximum is 5 ranges + 0 0)
				while( i < MAX_LEADBYTES && cpnfo.LeadByte[i])
				{
					uchar s = cpnfo.LeadByte[i++];
					uchar e = cpnfo.LeadByte[i++];
					//String str = SInt2Str(s).c_str(); str += TEXT(" - "); str += SInt2Str(e).c_str();
					//MessageBox(NULL, str.c_str(), TEXT("LeadByte range"), 0 );
					for( int j=s; j <= e; ++j )
						next_LUT[j] = 2;
				}
				return (uNextFunc)DBCSMODE; // Use the LUT
			}
		}

		// CharNextExA is not here on NT3.1 and is a stub on NT3.5
		// According to the MSDN DOC CharNextExA is available since NT4/95
		// It seems that it works fine on NT3.51 build 1057
		// It seems CharNextExA can crash
		// so we exclude NT versions between 3.51.1057 and  and 4.00.1381
		uNextFunc Window_CharNextExA = (uNextFunc)NULL ;
		if(!app().isNT()
		||  app().isNTOSVerEqual( MKVER(3,51,1057) )
		||  app().isNTOSVerLarger( MKVER(4,00,1381) ) )
		{
			Window_CharNextExA = (uNextFunc)GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")), "CharNextExA");
			if (Window_CharNextExA)
			{ // We got the function!
			  // Double check that it actually works, Just in case
				const char *test = "ts";
				const char *t = Window_CharNextExA(CP_ACP, test, 0);
				if( t == &test[1] ) // Olny valid answer for any CP
				{ // CharNextExA Works!
					//MessageBox(NULL, TEXT("CharNextExA Works!"), NULL, 0);
					return Window_CharNextExA;
				}
			}
		}
		// Fallback to ANSI mode if nothing could be done...
		return (uNextFunc)ANSIMODE;
	}
};



//-------------------------------------------------------------------------
// ISO-2022 の適当な実装
//
//	コードページとして、G0, G1, G2, G3 の四面を持つ。
//	それぞれ決まったエスケープシーケンスによって、
//	さまざまな文字集合を各面に呼び出すことが出来る。
//	とりあえず現在のバージョンで対応しているふりなのは
//	次の通り。()内に、そのいい加減っぷりを示す。
//
//		<<いつでも>>
//		1B 28 42    : G0へ ASCII
//		1B 28 4A    : G0へ JIS X 0201 ローマ字 (のかわりにASCII)
//		1B 29 4A    : G1へ JIS X 0201 ローマ字 (のかわりにASCII)
//		1B 2A 4A    : G2へ JIS X 0201 ローマ字 (のかわりにASCII)
//		1B 3B 4A    : G3へ JIS X 0201 ローマ字 (のかわりにASCII)
//		1B 2E 41    : G2へ ISO-8859-1
//		<<CP932が有効な場合>>
//		1B 28 49    : G0へ JIS X 0201 カナ
//		1B 29 49    : G1へ JIS X 0201 カナ
//		1B 2A 49    : G2へ JIS X 0201 カナ
//		1B 2B 49    : G3へ JIS X 0201 カナ
//		1B 24 40    : G0へ JIS X 0208(1978)
//		1B 24 42    : G0へ JIS X 0208(1983)    (年度は区別しない)
//		<<CP936が有効な場合>>
//		1B 24 41    : G0へ GB 2312
//		1B 24 29 41 : G1へ GB 2312
//		1B 24 2A 41 : G2へ GB 2312
//		1B 24 2B 41 : G3へ GB 2312
//		<<CP949が有効な場合>>
//		1B 24 28 43 : G0へ KS X 1001
//		1B 24 29 43 : G1へ KS X 1001
//		1B 24 2A 43 : G2へ KS X 1001
//		1B 24 2B 43 : G3へ KS X 1001
//
//	各面に呼び出した文字集合は、
//		GL (0x21〜0xfe) GR (0xa0〜0xff)
//	のどちらかへマップすることで、実際のバイト値となる。
//	マップ命令となるバイト列は、次の通り
//
//		0F    : GL        へG0を呼び出し
//		0E    : GL        へG1を呼び出し
//		1B 7E : GR        へG1を呼び出し
//		8E    : GL/GR両方 へG2を一瞬だけ呼び出し。1B 4E も同義
//		8F    : GL/GR両方 へG3を一瞬だけ呼び出し。1B 4F も同義
//
//-------------------------------------------------------------------------

enum CodeSet { ASCII, LATIN, KANA, JIS, KSX, GB };

struct rIso2022 A_FINAL: public TextFileRPimpl
{
	// Helper: JIS X 0208 => SJIS
	void jis2sjis( uchar k, uchar t, char* s )
	{
		if(k>=0x3f)	s[0] = (char)(((k+1)>>1)+0xc0);
		else		s[0] = (char)(((k+1)>>1)+0x80);
		if( k&1 )	s[1] = (char)((t>>6) ? t+0x40 : t+0x3f);
		else		s[1] = (char)(t+0x9e);
	}
	// ファイルポインタ
	const uchar* fb;
	const uchar* fe;
	const bool fixed; // ESCによる切り替えを行わないならtrue
	const bool mode_hz; // HZの場合。

	// 作業変数
	const CodeSet *GL;
	CodeSet      G[4];
	int gWhat; // 次の字は 1:GL/GR 2:G2 3:G3 で出力
	size_t len;

	// 初期化
	rIso2022( const uchar* b, size_t s, bool f, bool hz,
		CodeSet g0, CodeSet g1, CodeSet g2=ASCII, CodeSet g3=ASCII )
		: fb( b )
		, fe( b+s )
		, fixed( f )
		, mode_hz( hz )
		, GL( &G[0] )
		, gWhat( 1 )
		, len( 0 )
	{
		G[0]=g0, G[1]=g1, G[2]=g2, G[3]=g3;
	}

	void DoSwitching( const uchar*& p )
	{
		if( fixed )
		{
			if( p[0]==0x24 && p[1]!=0x40 && p[1]!=0x41 && p[1]!=0x42
			 && p+2 < fe && (p[2]==0x41 || p[2]==0x43) )
				++p;
		}
		else
		{
			if( p[1]==0x4A )
				G[ (p[0]-0x28)&3 ] = ASCII;         // 1B [28-2B] 4A
			else if( p[1]==0x49 )
				G[ (p[0]-0x28)&3 ] = KANA;          // 1B [28-2B] 49
			else if( *reinterpret_cast<const dbyte*>(p)==0x4228 )
				G[ 0 ] = ASCII;                     // 1B 28 42
			else if( *reinterpret_cast<const dbyte*>(p)==0x412E )
				G[ 2 ] = LATIN;                     // 1B 2E 41
			else if( p[0]==0x24 ) {
				if( p[1]==0x40 || p[1]==0x42 )
					G[ 0 ] = JIS;                   // 1B 24 [40|42]
				else if( p[1]==0x41 )
					G[ 0 ] = GB;                    // 1B 24 41
				else if( p+2 < fe )
				{
					if( p[2]==0x41 )
						G[ ((*++p)-0x28)&3 ] = GB;  // 1B 24 [28-2B] 41
					else if( p[2]==0x43 )
						G[ ((*++p)-0x28)&3 ] = KSX; // 1B 24 [28-2B] 43
				}
			}
		}
		++p;
	}

	void DoOutput( unicode*& buf, const uchar*& p )
	{
		// 文字集合取り出し
		CodeSet cs =
			(gWhat==2 ? G[2] :
			(gWhat==3 ? G[3] :
			(*p&0x80  ? G[1] : *GL)));

		char c[2];
		size_t wt=1;
		switch( cs )
		{
		case ASCII:
			*buf = (*p)&0x7f;
			break;
		case LATIN:
			*buf = (*p)|0x80;
			break;
		case KANA:
			c[0] = (*p)|0x80;
			wt = ::MultiByteToWideChar(
				932, MB_PRECOMPOSED, c, 1, buf, 2 );
			break;
		case GB:
		case KSX:
			c[0] = (*  p)|0x80;
			c[1] = (*++p)|0x80;
			wt = ::MultiByteToWideChar(
				(cs==GB?936:949), MB_PRECOMPOSED, c, 2, buf, 2 );
			break;
		case JIS:
			jis2sjis( (p[0]&0x7f)-0x20, (p[1]&0x7f)-0x20, c );
			++p;
			wt = ::MultiByteToWideChar(
				932, MB_PRECOMPOSED, c, 2, buf, 2 );
			break;
		}
		buf+=wt;
		len+=wt;
	}

	size_t ReadBuf( unicode* buf, size_t siz ) override
	{
		len=0;

		// バッファの終端か、ファイルの終端の近い方まで読み込む
		const uchar *p, *end = Min( fb+siz/2-2, fe );

		// 改行が出るまで進む
		for( p=fb; p<end; ++p )
			switch( *p )
			{
			case 0x0F:    GL = &G[0]; break;
			case 0x0E:    GL = &G[1]; break;
			case 0x8E: gWhat =     2; break;
			case 0x8F: gWhat =     3; break;
			case 0x1B:
				if( p+1<fe ) {
					++p; if( *p==0x7E ) ; // Nothing to do.
					else if( *p==0x4E ) gWhat =  2;
					else if( *p==0x4F ) gWhat =  3;
					else if( p+1<fe )   DoSwitching(p);
				}break;
			case 0x7E: if( mode_hz && p+1<fe ) {
					++p; if( *p==0x7D ){ GL = &G[0]; break; }
					else if( *p==0x7B ){ GL = &G[1]; break; }
				} // fall through...
			default:
				if( p+1>=end ) goto outofloop;
				DoOutput( buf, p ); gWhat=1; break;
			}
		outofloop:

		// If the end of the buffer contains half a DOS CRLF
		if( p > fb && *(p-1)=='\r' && *p=='\n' )
			++p;
		fb = p;

		// 終了
		return len;
	}
};



//-------------------------------------------------------------------------
// 自動判定などなど
//-------------------------------------------------------------------------

TextFileR::TextFileR( int charset )
	: impl_ (NULL),  cs_( charset )
	, lb_( CRLF ), nolbFound_(true)
{
}

TextFileR::~TextFileR()
{
	Close();
	if( impl_ )
		delete impl_;
}

size_t TextFileR::ReadBuf( unicode* buf, size_t siz )
{
	return impl_->ReadBuf( buf, siz );
}

void TextFileR::Close()
{
	fp_.Close();
}

// cs should be below -100;
int TextFileR::neededCodepage(int cs)
{
	switch(cs)
	{
	// Whitelist...
	// for positives values and values < -100;
	case UTF1Y: // -64999
	case UTF9Y: // -65002
	case UTF7:  // +65000
	case UTF8:  // -65001
	case UTF8N: // +65001
	case SCSUY: // -60000
	case SCSU:  // -60002
	case BOCU1Y:// -60001
	case BOCU1: // -60003
	case 28591: // ISO-8859-1
//	case WesternOS2: // TODO (1004) internally handeled.
		return 0 ; // WHITELISTED

	// TYPE -(cp+1)
	case IsoKR: // -950 => 949
	case HZ:    // -937 => 936
	case IsoJP: // -933 => 932
		return -cs - 1; // Funny cases.

//	case IsoCN:
//	case EucJP:
// and all > 0 cases.
	default:
		if(cs < -100) // values between 0 and -100 are whitelisted
			return -cs; // negative sign case.
		else
			return cs; // normal case cs == needed cp.
	}
}
bool TextFileR::Open( const TCHAR* fname, bool always )
{
	// ファイルを開く
	if( !fp_.Open(fname, always) )
		return false;
	const uchar* buf = fp_.base();
	const size_t  siz = fp_.size();

	// 必要なら自動判定
	cs_ = AutoDetection( cs_, buf, siz );
	if( cs_ )
	{
		int needed_cs = neededCodepage( cs_ );

		if( needed_cs > 0 && !::IsValidCodePage(needed_cs) )
		{
			TCHAR str[128];
			::wsprintf(str, TEXT("Codepage cp%d Is not installed!\nDefaulting to current ACP"), needed_cs);
			::MessageBox(::GetActiveWindow(), str, TEXT("Encoding"), MB_OK|MB_TASKMODAL);
			if( cs_ == Western )
			{	// Codepage 1252 is not installed,
				// default to ISO-8859-1 (CP28591)
				cs_ = 28591;
			}
			else
			{
				cs_ = ::GetACP(); // default to ACP...
			}
		}
	}
	if( !cs_ ) cs_ = ::GetACP();


	// 対応するデコーダを作成
	switch( cs_ )
	{
	case 28591:   impl_ = new rWestISO88591(buf,siz); break; // ISO-8859-1
	case UTF16b:
	case UTF16BE: impl_ = new rUtf16BE(buf,siz); break;
	case UTF16l:
	case UTF16LE: impl_ = new rUtf16LE(buf,siz); break;
	case UTF32b:
	case UTF32BE: impl_ = new rUtf32BE(buf,siz); break;
	case UTF32l:
	case UTF32LE: impl_ = new rUtf32LE(buf,siz); break;
	case UTF1Y:
	case UTF1:    impl_ = new rUtf1(buf,siz); break;
	case UTF5Y:
	case UTF5:    impl_ = new rUtf5(buf,siz); break;
	case UTF7:    impl_ = new rUtf7(buf,siz); break;
	case UTF9Y:
	case UTF9:    impl_ = new rUtf9(buf,siz); break;
	case SCSUY:
	case SCSU:    impl_ = new rSCSU(buf,siz); break;
	case BOCU1Y:
	case BOCU1:   impl_ = new rBOCU1(buf,siz); break;
	case OFSSUTFY:
	case OFSSUTF: impl_ = new rUtfOFSS(buf,siz); break;
	case UTFEBCDICY:
	case UTFEBCDIC:impl_ = new rUtfEBCDIC(buf,siz); break;
	case EucJP:   impl_ = new rIso2022(buf,siz,true,false,ASCII,JIS,KANA); break;
	case IsoJP:   impl_ = new rIso2022(buf,siz,false,false,ASCII,KANA); break;
	case IsoKR:   impl_ = new rIso2022(buf,siz,true,false,ASCII,KSX); break;
	case IsoCN:   impl_ = new rIso2022(buf,siz,true,false,ASCII,GB); break;
	case HZ:      impl_ = new rIso2022(buf,siz,true,true, ASCII,GB); break;
	default:      impl_ = new rMBCS(buf,siz,cs_); break;
	}

	return impl_ != NULL;
}

int TextFileR::AutoDetection( int cs, const uchar* ptr, size_t totsiz )
{
//-- まず、文字の出現回数の統計を取る

	size_t siz = Min( totsiz, (size_t)(16<<10) ); // 先頭16KB

	uint  freq[256];
	bool bit8 = false;
	mem00( freq, sizeof(freq) );
	for( size_t i=0; i<siz; ++i )
	{
		if( ptr[i] >= 0x80 )
			bit8 = true;
		++freq[ ptr[i] ];
	}

//-- 改行コード決定 (UTF16/32/7のとき問題あり。UTF5に至っては判定不可…)

	setLBfromFreq( freq, '\r', '\n' );

//-- デフォルトコード

	int defCs = ::GetACP();

//-- 小さすぎる場合はここで終了

	if( siz < 4 )
		return cs==AutoDetect ? defCs : cs;

//-- 明示指定がある場合はここで終了

	uint bom4 = (ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | (ptr[3]);
	uint bom2 = (ptr[0]<<8)  | (ptr[1]);

	if( cs==UTF8 || cs==UTF8N )
		cs = (bom4>>8==0xefbbbf ? UTF8 : UTF8N);
	else if( cs==UTF32b || cs==UTF32BE )
		cs = (bom4==0x0000feff ? UTF32b : UTF32BE);
	else if( cs==UTF32l || cs==UTF32LE )
		cs = (bom4==0xfffe0000 ? UTF32l : UTF32LE);
	else if( cs==UTF16b || cs==UTF16BE )
		cs = (bom2==0xfeff ? UTF16b : UTF16BE);
	else if( cs==UTF16l || cs==UTF16LE )
		cs = (bom2==0xfffe ? UTF16l : UTF16LE);
	else if( cs==UTF1 || cs==UTF1Y )
		cs = bom4>>8==0xf7644c ? UTF1Y : UTF1;
	else if( cs==UTF5 || cs==UTF5Y )
		cs = bom4==0x56454646 ?  UTF5Y : UTF5;
	else if( cs==UTF9 || cs==UTF9Y )
		cs = bom4>>8==0x93fdff ? UTF9Y : UTF9;
	else if( cs==OFSSUTF || cs==OFSSUTFY )
		cs = bom4>>8==0xc3bcff ? OFSSUTFY : OFSSUTF;
	else if( cs==GB18030 || cs==GB18030Y )
		cs = bom4 == 0x84319533 ? GB18030Y : GB18030;
	else if( cs==BOCU1 || cs==BOCU1Y )
		cs = bom4>>8==0xfbee28 ? BOCU1Y : BOCU1;
	else if( cs==SCSU || cs==SCSUY )
		cs = bom4>>8==0x0efeff ? SCSUY : SCSU;
	else if( cs==UTFEBCDIC || cs==UTFEBCDICY )
	{
		cs = bom4==0xdd736673 ? UTFEBCDICY : UTFEBCDIC;
		setLBfromFreq(freq, '\r', '\x15');//in UTF-EBCDIC LF==x15
	}
	else if( isEBCDIC(cs) )
	{
		// NON-ASCII EBCDIC mode LF encding is different
		// Depeding on EBCDIC LF can be x15 or x25.
		// Technically \x15 is NEL(U+85) not LF(U+0A).
		// However it seems to get randomly swapped for no reasons.
		// so you can have 0D 15, 0D 25, or 15 alone or 25 alone
		uchar lf = freq['\x15']>freq['\x25']? '\x15' : '\x25';
		setLBfromFreq(freq, '\r', lf);
	}


	if( cs != AutoDetect )
		return cs;

//-- BOMチェック・7bitチェック

	bool Jp = ::IsValidCodePage(932)!=FALSE;

	if( (bom4>>8) == 0xefbbbf )      cs = UTF8;
	else if( (bom4>>8) == 0xf7644c ) cs = UTF1Y;
	else if( (bom4>>8) == 0x93fdff ) cs = UTF9Y;
	else if( (bom4>>8) == 0x0efeff ) cs = SCSUY;
	else if( (bom4>>8) == 0xfbee28 ) cs = BOCU1Y;
	else if( (bom4>>8) == 0xc3bcff ) cs = OFSSUTFY;
	else if( bom4 == 0x0000feff ) cs = UTF32b;
	else if( bom4 == 0xfffe0000 ) cs = UTF32l;
	else if( bom2 == 0xfeff )     cs = UTF16b;
	else if( bom2 == 0xfffe )     cs = UTF16l;
	else if( !bit8 && !freq[0]
	    &&(  bom4 == 0x2b2f7638
	      || bom4 == 0x2b2f7639
	      || bom4 == 0x2b2f762b
	      || bom4 == 0x2b2f762f ) ) cs = UTF7;
	else if( bom4 == 0x84319533 ) cs = GB18030Y;
	else if( bom4 == 0xdd736673){ cs = UTFEBCDICY; setLBfromFreq(freq, '\r', '\x15'); }
	else if( bom4 == 0x1b242943 && ::IsValidCodePage(949) ) cs = IsoKR;
	else if( bom4 == 0x1b242941 && ::IsValidCodePage(936) ) cs = IsoCN;
	else if( Jp && !bit8 && freq[0x1b]>0 )                  cs = IsoJP;

	if( cs != AutoDetect )
		return cs;

//-- UTF-5 チェック

	uint u5sum = 0;
	for( uchar c='0'; c<='9'; ++c ) u5sum += freq[c];
	for( uchar c='A'; c<='V'; ++c ) u5sum += freq[c];
	if( siz == u5sum )
		return bom4==0x56454646 ? UTF5Y : UTF5;

//-- UTF-16/32 detection
	if( freq[ 0 ] > siz >> 11 ) // More than 1/2048 nulls in content?
	{	// then it may be UTF-16/32 without BOM We make some sanity
		// checks on NULs frequency with 1% extra NULs tolerance
		if( freq[ 0 ] <= (siz>>1) + (siz>>7) && !(totsiz&1) )
		{	// If we got less than 50% NULs it might be UTF16
			// UTF-16 byte count will always be even
			if(CheckUTFConfidence(ptr,siz,sizeof(dbyte),true)) return UTF16LE;
			if(CheckUTFConfidence(ptr,siz,sizeof(dbyte),false)) return UTF16BE;
		}
		if( freq[ 0 ] <= 3*(siz>>2) + (siz>>7) && !(totsiz&3) )
		{	// If we got less than 75% NULs it might be UTF32
			// UTF-32 byte count will always be multiple of 4
			if(CheckUTFConfidence(ptr,siz,sizeof(qbyte),true)) return UTF32LE;
			if(CheckUTFConfidence(ptr,siz,sizeof(qbyte),false)) return UTF32BE;
		}

		if ( freq[ 0 ] >= siz>>9 )
		{	// More than 1/512 NULs and not UTF16/32? BINARY file!
			// Use ISO8859-1 mode which is internally handled and very simple.
			return 28591; // Latin-1 but not cp1252
		}
	}

//-- Autodetect pure ASCII File (Fast method)
	// We can do that because we ignore UTF-7 autodection anyway.
	if( !bit8 )
	{
		// 7-bit encoded file.
		// Check if there are any [0-8] or [14-31] and 127
		for( int i = 0; i <= 8; i++)
			if( freq[ i ] )
				goto nonASCII;
		for( int i = 14; i <= 31; i++)
			if( freq[ i ] )
				goto nonASCII;
		if( freq[ 127 ] )
			goto nonASCII;

		// File is ASCII => use CP_ACP.
		return defCs;
		nonASCII:;
	}

//-- chardet and MLang detection
	// chardet works better when size > 64
	if( siz <= 80 )
	{
		cs = MLangAutoDetection( ptr, siz );
		if( cs == ASCIICP ) return defCs;
		if( cs ) return cs;
	}
	// Chardet may be the only auto detection method
	cs = chardetAutoDetection( ptr, siz );
	if( cs == ASCIICP ) return defCs;
	if( cs ) return cs;

	// Try Mlang for larger sizes if chardet failed.
	if( siz > 80 )
	{
		cs = MLangAutoDetection( ptr, siz );
		if( cs == ASCIICP ) return defCs;
		if( cs ) return cs;
	}

// last resort
//-- 暫定版 UTF-8 / 日本語EUC チェック

	// 改行コードがLFか、ある程度の大きさか、でないと
	// 無条件で ANSI-CP と見なしてしまう。
	if( bit8 && (siz>4096 || lb_==1
	 || freq[0xfd] || freq[0xfe] || freq[0xff] || freq[0x80] || freq[0xC3] ) )
	{
		// UHCやGBKはEUC-JPと非常に混同しやすいので、そっちがデフォルトの場合は
		// EUC-JP自動判定を切る
		if( Jp && defCs==SJIS )
		{
			// EUCとしておかしい値が無いかチェック
			bool be=true;
			for( int k=0x90; k<=0xa0; ++k )if( freq[k]>0 ){be=false;break;}
			for( int k=0x7f; k<=0x8d; ++k )if( freq[k]>0 ){be=false;break;}
			if( be )
				return EucJP;
		}

		if( IsValidUTF8(ptr, siz) )
			return UTF8N;
	}


//-- 判定結果

	return cs ? cs : defCs;
}

void TextFileR::setLBfromFreq(uint freq[256], uchar cr, uchar lf)
{
	     if( freq[cr] > freq[lf]*2 ) lb_ = CR;
	else if( freq[lf] > freq[cr]*2 ) lb_ = LF;
	else                             lb_ = CRLF;
	nolbFound_ = freq[cr]==0 && freq[lf]==0;
}

// Just says if cs is part of the EBCDIC encoding familly
bool TextFileR::isEBCDIC( int cs )
{
//	static const unsigned short ebcidlst[] = {
//		37, 500, 870, 875, 1026, 1047,
//		//1140--1149
//		20273, 20277, 20278, 20280, 20284, 20285,
//		20290, 20297, 20420, 20423, 20424, 20833,
//		20838, 20871, 20880, 20905, 20924, 21025,
//		50930, 50931, 50933, 50935, 50937, 50939
//	};
	if( cs == UTFEBCDIC || cs == UTFEBCDICY )
		return true;

	// Detect EBCDIC based on translation
	if( 0 < cs && cs < 65535 && ::IsValidCodePage( cs ) )
	{
		wchar_t tr[4];
		int ret = ::MultiByteToWideChar( cs, 0, "\x81\x40\xF0", 4, tr, 4 );//"a 0"
		return ret==4 && tr[0]==L'a' && tr[1]==L' ' && tr[2]==L'0';
	}

	return false;
}

bool TextFileR::IsValidUTF8( const uchar* ptr, size_t siz ) const
{
	// UTF8として読めるかどうかチェック
	int mi=1; // mask index
	for( size_t i=0; i<siz ; ++i )
		if( --mi )
		{	// remaining bytes must be 10xxxxxx
			if( ptr[i]<0x80 || ptr[i]>=0xc0 )
				return false;
		}
		else
		{
			mi = 1;
			if( ptr[i] > 0x7f )
			{
				mi = GetMaskIndex( ptr[i] );
				if( mi == 1 )//ptr[i] >= 0xfe )
					return false;
			}
		}

	return true;
}

int TextFileR::MLangAutoDetection( const uchar* ptr, size_t siz )
{
	int cs = 0;
#ifndef NO_MLANG
	if ( !app().isNewShell() )
		return 0;
	app().InitModule( App::OLE );
	IMultiLanguage2 *lang = NULL;
	static const IID myIID_IMultiLanguage2 = {0xDCCFC164, 0x2B38, 0x11d2, {0xB7, 0xEC, 0x00, 0xC0, 0x4F, 0x8F, 0x5D, 0x9A}};
	static const CLSID myCLSID_CMultiLanguage = { 0x275c23e2, 0x3747, 0x11d0, {0x9f, 0xea, 0x00,0xaa,0x00,0x3f,0x86,0x46} };
	HRESULT ret = ::MyCoCreateInstance(myCLSID_CMultiLanguage, NULL, CLSCTX_ALL, myIID_IMultiLanguage2, reinterpret_cast<LPVOID*>(&lang) );
	if( S_OK == ret && lang != NULL )
	{
		int detectEncCount = 5;
		DetectEncodingInfo detectEnc[5];
		lang->DetectInputCodepage(MLDETECTCP_DBCS, 0, (char *)(ptr), reinterpret_cast<INT*>(&siz), detectEnc, &detectEncCount); // Ugly C-cast here

	# ifdef MLANG_DEBUG
		TCHAR otmp[1024];
		TCHAR stmp[64];
		::wsprintf(otmp,TEXT("detectEncCount = %d\n"),detectEncCount);

		for(int decCnt=0;decCnt<detectEncCount;decCnt++){
			::wsprintf(stmp,TEXT("detectEnc[%d] = %d (Conf.: %d)\n"),detectEncCount, detectEnc[decCnt].nCodePage, detectEnc[decCnt].nConfidence);
			::wsprintf(otmp,TEXT("%s%s\n"),otmp,stmp);

		}
		::MessageBox(NULL,otmp,TEXT("MLangDetect"),0);
	# endif

		// MLang fine tunes
		if ( detectEncCount > 1 && detectEnc[0].nCodePage == 1252 ) // sometimes it gives multiple results with 1252 in the first
		{
			if ( detectEncCount == 2 && detectEnc[1].nCodePage == 850 ) // seems to be wrongly detected
			{
				cs = 0;
			}
			else
			{
				cs =  detectEnc[detectEncCount-1].nCodePage; // always use last one
			}
		}
		else if ( detectEncCount > 1 && detectEnc[0].nCodePage > 950 && detectEnc[0].nCodePage != UTF8N ) // non asian codepage in first
		{
			int highestConfidence = 0;
			for(int x=0;x<detectEncCount;x++)
			{
				if(highestConfidence < detectEnc[x].nConfidence)
				{
					highestConfidence = detectEnc[x].nConfidence; // use codepage with highest Confidence
					cs = detectEnc[x].nCodePage;
				}
			}
		}
		else if ( detectEnc[0].nCodePage != UTF8N && ::IsValidCodePage(GB18030) &&
				  IsGB18030Like( ptr, siz, detectEnc[0].nCodePage ) ) // Check if GB18030 is vaild system codepage
		{
			cs = GB18030;
		}
		else
		{
			cs =  detectEnc[0].nCodePage;
		}

	# ifdef MLANG_DEBUG
		::MessageBox(NULL, SInt2Str(cs),TEXT("MLangDetect"),0);
	# endif

//		if (cs == 20127) cs = 0; // 20127 == ASCII, 0 = unknown
		if (cs == 65000) cs = ASCIICP; // 65000 == UTF-7, let's disable misdetecting as UTF-7

		lang->Release();
	}
#endif //NO_MLANG
	return cs;
}

int TextFileR::chardetAutoDetection( const uchar* ptr, size_t siz )
{
	int cs = 0;
#ifndef NO_CHARDET

#define CHARDET_RESULT_OK               ( 0)
#define CHARDET_RESULT_NOMEMORY         (-1)
#define CHARDET_RESULT_INVALID_DETECTOR (-2)

	typedef void* chardet_t;

	// function calls
	int (__cdecl*chardet_create)(chardet_t*);
	void (__cdecl*chardet_destroy)(chardet_t);
	int (__cdecl*chardet_handle_data)(chardet_t, const char*, unsigned int);
	int (__cdecl*chardet_data_end)(chardet_t);
	int (__cdecl*chardet_get_charset)(chardet_t, char*, unsigned int);
	//int (__cdecl*chardet_reset)(chardet_t) = 0;
	HINSTANCE hIL;

	chardet_t pdet = NULL;
	char charset[128];

	#if defined(_M_AMD64) || defined(_M_X64)
	# define CHARDET_DLL "chardet_x64.dll"
	#elif defined(_M_IA64)
	# define CHARDET_DLL "chardet_ia64.dll"
	#elif defined(_M_ARM64)
	# define CHARDET_DLL "chardet_arm64.dll"
	#elif defined(_M_ARM)
	# define CHARDET_DLL "chardet_arm.dll"
	#elif defined(_M_ALPHA)
	# define CHARDET_DLL "cdetaxp.dll"
	#elif defined(_M_MRX000) || defined(_MIPS_)
	# define CHARDET_DLL "cdetmips.dll"
	#elif defined(_M_PPC)
	# define CHARDET_DLL "cdetppc.dll"
	#else
	# define CHARDET_DLL "chardet.dll"
	#endif

	// On Win32s we must check if CHARDET.DLL exist before trying LoadLibrary()
	// Otherwise we would get a system  message
	Path chardet_in_gp_dir = Path(Path::Exe) + TEXT(CHARDET_DLL);
	if( !chardet_in_gp_dir.exist() )
		return 0;

	// Use LoadLibrary with full pathname (safer)
	hIL = ::LoadLibrary( chardet_in_gp_dir.c_str() );
	if( !hIL )
	{
		#ifdef MLANG_DEBUG
		::MessageBox(NULL,TEXT("Cannot Load CHARDET.DLL"),NULL,0);
		#endif
		return 0;
	}

	chardet_create = (int(__cdecl*)(chardet_t*))::GetProcAddress(hIL, "chardet_create");
	chardet_destroy = (void(__cdecl*)(chardet_t))::GetProcAddress(hIL, "chardet_destroy");
	chardet_handle_data = (int(__cdecl*)(chardet_t, const char*, unsigned int))::GetProcAddress(hIL, "chardet_handle_data");
	chardet_data_end = (int(__cdecl*)(chardet_t))::GetProcAddress(hIL, "chardet_data_end");
	chardet_get_charset = (int(__cdecl*)(chardet_t, char*, unsigned int))::GetProcAddress(hIL, "chardet_get_charset");
	//chardet_reset = (int(__cdecl*)(chardet_t))::GetProcAddress(hIL, "chardet_reset");

	if( !(chardet_create && chardet_destroy && chardet_handle_data && chardet_data_end && chardet_get_charset) )
	{
		#ifdef MLANG_DEBUG
		::MessageBox(NULL,TEXT("Unable to find all procs in chardet.dll"),NULL,0);
		#endif
		goto freeandexit;
	}

    if( 0 != chardet_create(&pdet) )
	{
		#ifdef MLANG_DEBUG
		::MessageBox(NULL,TEXT("chardet_create() failed!"),NULL,0);
		#endif
		goto freeandexit;
	}

	if( 0 == chardet_handle_data(pdet, reinterpret_cast<const char *>(ptr), siz-1) )
	{
		static const struct {
			const char *str; int cs;
		} cslist[] = {
			{ "ASCII",          ASCIICP },
			{ "Shift_JIS",      SJIS },
			{ "EUC-JP",         EucJP },
			{ "EUC-KR",         UHC },
			{ "x-euc-tw",       CNS },
			{ "Big5",           Big5 },
			{ "GB18030",        GB18030 },
			{ "UTF-8",          UTF8N },
			{ "windows-1253",   Greek },
			{ "ISO-8859-7",     GreekISO },
			{ "KOI8-R",         Koi8R },
			{ "windows-1251",   Cyrillic },
			{ "IBM866",         CyrillicDOS },
			{ "IBM855",         CyrillicIBM },
			{ "x-mac-cyrillic", CyrillicMAC },
			{ "MAC-CYRILLIC",   CyrillicMAC },
			{ "ISO-8859-5",     CyrillicISO },
			{ "windows-1255",   Hebrew },
			{ "windows-1250",   Central },
			{ "ISO-8859-2",     CentralISO },
			{ "TIS-620",        Thai },
			{ "ISO-8859-11",    ThaiISO },
			{ "windows-1252",   Western },
			{ "ISO-8859-1",     Western },
			{ "ISO-8859-15",    WesternISO },
			{ "ISO-8859-3",     EsperantoISO },
			{ "ISO-8859-9",     TurkishISO },
			{ "ISO-8859-6",     ArabicISO },
			{ "windows-1256",   Arabic },
			{ "windows-1258",   Vietnamese }
		};

		chardet_data_end(pdet);
		chardet_get_charset(pdet, charset, 128);

		for (size_t i =0; i < countof(cslist); i++)
		{
			// Pure ASCII case insensitive comparison.
			if(0 == my_lstrcmpiAsciiA(charset,cslist[i].str))
			{
				cs = cslist[i].cs;
				break;
			}
		}
		chardet_destroy(pdet);
	}

	// Spetial GB18030 check!
	if( cs == GB18030 && !::IsValidCodePage(GB18030) )
		cs = GBK;

	freeandexit:
	::FreeLibrary(hIL);

	#ifdef MLANG_DEBUG
	::MessageBox(NULL, SInt2Str(cs).c_str(), TEXT("CHARDET"),0);
	#endif

#endif //NO_CHARDET
	return cs;
}

// functions for detecting BOM-less UTF-16/32
bool TextFileR::IsNonUnicodeRange(qbyte u) const
{ // Unicode 14.0 based, Updated to Unicode 15.0
	if( u < 0x012550 ) // Quick most likely check.
		return (0x002FE0 <= u && u < 0x002FF0); // Small exclusion range in BMP

	if( u > 0x1100000 ) // Beyond max possible codepoint
		return true;

	return // All ranges (ranges smaller than 1000 are commented out)
		//	U+0000..U+007F	Basic Latin
		//	U+0080..U+00FF	Latin-1 Supplement
		//	U+0100..U+017F	Latin Extended-A
		//	U+0180..U+024F	Latin Extended-B
		//	U+0250..U+02AF	IPA Extensions
		//	U+02B0..U+02FF	Spacing Modifier Letters
		//	U+0300..U+036F	Combining Diacritical Marks
		//	U+0370..U+03FF	Greek and Coptic
		//	U+0400..U+04FF	Cyrillic
		//	U+0500..U+052F	Cyrillic Supplement
		//	U+0530..U+058F	Armenian
		//	U+0590..U+05FF	Hebrew
		//	U+0600..U+06FF	Arabic
		//	U+0700..U+074F	Syriac
		//	U+0750..U+077F	Arabic Supplement
		//	U+0780..U+07BF	Thaana
		//	U+07C0..U+07FF	NKo
		//	U+0800..U+083F	Samaritan
		//	U+0840..U+085F	Mandaic
		//	U+0860..U+086F	Syriac Supplement
		//	U+0870..U+089F	Arabic Extended-B
		//	U+08A0..U+08FF	Arabic Extended-A
		//	U+0900..U+097F	Devanagari
		//	U+0980..U+09FF	Bengali
		//	U+0A00..U+0A7F	Gurmukhi
		//	U+0A80..U+0AFF	Gujarati
		//	U+0B00..U+0B7F	Oriya
		//	U+0B80..U+0BFF	Tamil
		//	U+0C00..U+0C7F	Telugu
		//	U+0C80..U+0CFF	Kannada
		//	U+0D00..U+0D7F	Malayalam
		//	U+0D80..U+0DFF	Sinhala
		//	U+0E00..U+0E7F	Thai
		//	U+0E80..U+0EFF	Lao
		//	U+0F00..U+0FFF	Tibetan
		//	U+1000..U+109F	Myanmar
		//	U+10A0..U+10FF	Georgian
		//	U+1100..U+11FF	Hangul Jamo
		//	U+1200..U+137F	Ethiopic
		//	U+1380..U+139F	Ethiopic Supplement
		//	U+13A0..U+13FF	Cherokee
		//	U+1400..U+167F	Unified Canadian Aboriginal Syllabics
		//	U+1680..U+169F	Ogham
		//	U+16A0..U+16FF	Runic
		//	U+1700..U+171F	Tagalog
		//	U+1720..U+173F	Hanunoo
		//	U+1740..U+175F	Buhid
		//	U+1760..U+177F	Tagbanwa
		//	U+1780..U+17FF	Khmer
		//	U+1800..U+18AF	Mongolian
		//	U+18B0..U+18FF	Unified Canadian Aboriginal Syllabics Extended
		//	U+1900..U+194F	Limbu
		//	U+1950..U+197F	Tai Le
		//	U+1980..U+19DF	New Tai Lue
		//	U+19E0..U+19FF	Khmer Symbols
		//	U+1A00..U+1A1F	Buginese
		//	U+1A20..U+1AAF	Tai Tham
		//	U+1AB0..U+1AFF	Combining Diacritical Marks Extended
		//	U+1B00..U+1B7F	Balinese
		//	U+1B80..U+1BBF	Sundanese
		//	U+1BC0..U+1BFF	Batak
		//	U+1C00..U+1C4F	Lepcha
		//	U+1C50..U+1C7F	Ol Chiki
		//	U+1C80..U+1C8F	Cyrillic Extended-C
		//	U+1C90..U+1CBF	Georgian Extended
		//	U+1CC0..U+1CCF	Sundanese Supplement
		//	U+1CD0..U+1CFF	Vedic Extensions
		//	U+1D00..U+1D7F	Phonetic Extensions
		//	U+1D80..U+1DBF	Phonetic Extensions Supplement
		//	U+1DC0..U+1DFF	Combining Diacritical Marks Supplement
		//	U+1E00..U+1EFF	Latin Extended Additional
		//	U+1F00..U+1FFF	Greek Extended
		//	U+2000..U+206F	General Punctuation
		//	U+2070..U+209F	Superscripts and Subscripts
		//	U+20A0..U+20CF	Currency Symbols
		//	U+20D0..U+20FF	Combining Diacritical Marks for Symbols
		//	U+2100..U+214F	Letterlike Symbols
		//	U+2150..U+218F	Number Forms
		//	U+2190..U+21FF	Arrows
		//	U+2200..U+22FF	Mathematical Operators
		//	U+2300..U+23FF	Miscellaneous Technical
		//	U+2400..U+243F	Control Pictures
		//	U+2440..U+245F	Optical Character Recognition
		//	U+2460..U+24FF	Enclosed Alphanumerics
		//	U+2500..U+257F	Box Drawing
		//	U+2580..U+259F	Block Elements
		//	U+25A0..U+25FF	Geometric Shapes
		//	U+2600..U+26FF	Miscellaneous Symbols
		//	U+2700..U+27BF	Dingbats
		//	U+27C0..U+27EF	Miscellaneous Mathematical Symbols-A
		//	U+27F0..U+27FF	Supplemental Arrows-A
		//	U+2800..U+28FF	Braille Patterns
		//	U+2900..U+297F	Supplemental Arrows-B
		//	U+2980..U+29FF	Miscellaneous Mathematical Symbols-B
		//	U+2A00..U+2AFF	Supplemental Mathematical Operators
		//	U+2B00..U+2BFF	Miscellaneous Symbols and Arrows
		//	U+2C00..U+2C5F	Glagolitic
		//	U+2C60..U+2C7F	Latin Extended-C
		//	U+2C80..U+2CFF	Coptic
		//	U+2D00..U+2D2F	Georgian Supplement
		//	U+2D30..U+2D7F	Tifinagh
		//	U+2D80..U+2DDF	Ethiopic Extended
		//	U+2DE0..U+2DFF	Cyrillic Extended-A
		//	U+2E00..U+2E7F	Supplemental Punctuation
		//	U+2E80..U+2EFF	CJK Radicals Supplement
		//	U+2F00..U+2FDF	Kangxi Radicals
//		(0x002FE0 <= u && u < 0x002FF0) || // 16
		//	U+2FF0..U+2FFF	Ideographic Description Characters
		//	U+3000..U+303F	CJK Symbols and Punctuation
		//	U+3040..U+309F	Hiragana
		//	U+30A0..U+30FF	Katakana
		//	U+3100..U+312F	Bopomofo
		//	U+3130..U+318F	Hangul Compatibility Jamo
		//	U+3190..U+319F	Kanbun
		//	U+31A0..U+31BF	Bopomofo Extended
		//	U+31C0..U+31EF	CJK Strokes
		//	U+31F0..U+31FF	Katakana Phonetic Extensions
		//	U+3200..U+32FF	Enclosed CJK Letters and Months
		//	U+3300..U+33FF	CJK Compatibility
		//	U+3400..U+4DBF	CJK Unified Ideographs Extension A
		//	U+4DC0..U+4DFF	Yijing Hexagram Symbols
		//	U+4E00..U+9FFF	CJK Unified Ideographs
		//	U+A000..U+A48F	Yi Syllables
		//	U+A490..U+A4CF	Yi Radicals
		//	U+A4D0..U+A4FF	Lisu
		//	U+A500..U+A63F	Vai
		//	U+A640..U+A69F	Cyrillic Extended-B
		//	U+A6A0..U+A6FF	Bamum
		//	U+A700..U+A71F	Modifier Tone Letters
		//	U+A720..U+A7FF	Latin Extended-D
		//	U+A800..U+A82F	Syloti Nagri
		//	U+A830..U+A83F	Common Indic Number Forms
		//	U+A840..U+A87F	Phags-pa
		//	U+A880..U+A8DF	Saurashtra
		//	U+A8E0..U+A8FF	Devanagari Extended
		//	U+A900..U+A92F	Kayah Li
		//	U+A930..U+A95F	Rejang
		//	U+A960..U+A97F	Hangul Jamo Extended-A
		//	U+A980..U+A9DF	Javanese
		//	U+A9E0..U+A9FF	Myanmar Extended-B
		//	U+AA00..U+AA5F	Cham
		//	U+AA60..U+AA7F	Myanmar Extended-A
		//	U+AA80..U+AADF	Tai Viet
		//	U+AAE0..U+AAFF	Meetei Mayek Extensions
		//	U+AB00..U+AB2F	Ethiopic Extended-A
		//	U+AB30..U+AB6F	Latin Extended-E
		//	U+AB70..U+ABBF	Cherokee Supplement
		//	U+ABC0..U+ABFF	Meetei Mayek
		//	U+AC00..U+D7AF	Hangul Syllables
		//	U+D7B0..U+D7FF	Hangul Jamo Extended-B
		//	U+D800..U+DB7F	High Surrogates
		//	U+DB80..U+DBFF	High Private Use Surrogates
		//	U+DC00..U+DFFF	Low Surrogates
		//	U+E000..U+F8FF	Private Use Area
		//	U+F900..U+FAFF	CJK Compatibility Ideographs
		//	U+FB00..U+FB4F	Alphabetic Presentation Forms
		//	U+FB50..U+FDFF	Arabic Presentation Forms-A
		//	U+FE00..U+FE0F	Variation Selectors
		//	U+FE10..U+FE1F	Vertical Forms
		//	U+FE20..U+FE2F	Combining Half Marks
		//	U+FE30..U+FE4F	CJK Compatibility Forms
		//	U+FE50..U+FE6F	Small Form Variants
		//	U+FE70..U+FEFF	Arabic Presentation Forms-B
		//	U+FF00..U+FFEF	Halfwidth and Fullwidth Forms
		//	U+FFF0..U+FFFF	Specials
		//	U+10000..U+1007F	Linear B Syllabary
		//	U+10080..U+100FF	Linear B Ideograms
		//	U+10100..U+1013F	Aegean Numbers
		//	U+10140..U+1018F	Ancient Greek Numbers
		//	U+10190..U+101CF	Ancient Symbols
		//	U+101D0..U+101FF	Phaistos Disc
//		(0x010200 <= u && u < 0x010280) || //128
		//	U+10280..U+1029F	Lycian
		//	U+102A0..U+102DF	Carian
		//	U+102E0..U+102FF	Coptic Epact Numbers
		//	U+10300..U+1032F	Old Italic
		//	U+10330..U+1034F	Gothic
		//	U+10350..U+1037F	Old Permic
		//	U+10380..U+1039F	Ugaritic
		//	U+103A0..U+103DF	Old Persian
//		(0x0103E0 <= u && u < 0x010400) || // 32
		//	U+10400..U+1044F	Deseret
		//	U+10450..U+1047F	Shavian
		//	U+10480..U+104AF	Osmanya
		//	U+104B0..U+104FF	Osage
		//	U+10500..U+1052F	Elbasan
		//	U+10530..U+1056F	Caucasian Albanian
		//	U+10570..U+105BF	Vithkuqi
//		(0x0105C0 <= u && u < 0x010600) || //64
		//	U+10600..U+1077F	Linear A
		//	U+10780..U+107BF	Latin Extended-F
		//	U+10800..U+1083F	Cypriot Syllabary
		//	U+10840..U+1085F	Imperial Aramaic
		//	U+10860..U+1087F	Palmyrene
		//	U+10880..U+108AF	Nabataean
//		(0x0108B0 <= u && u < 0x0108E0) || //48
		//	U+108E0..U+108FF	Hatran
		//	U+10900..U+1091F	Phoenician
		//	U+10920..U+1093F	Lydian
//		(0x010940 <= u && u < 0x010980) || //64
		//	U+10980..U+1099F	Meroitic Hieroglyphs
		//	U+109A0..U+109FF	Meroitic Cursive
		//	U+10A00..U+10A5F	Kharoshthi
		//	U+10A60..U+10A7F	Old South Arabian
		//	U+10A80..U+10A9F	Old North Arabian
//		(0x010AA0 <= u && u < 0x010AC0) || // 32
		//	U+10AC0..U+10AFF	Manichaean
		//	U+10B00..U+10B3F	Avestan
		//	U+10B40..U+10B5F	Inscriptional Parthian
		//	U+10B60..U+10B7F	Inscriptional Pahlavi
		//	U+10B80..U+10BAF	Psalter Pahlavi
//		(0x010BB0 <= u && u < 0x010C00) || // 80
		//	U+10C00..U+10C4F	Old Turkic
//		(0x010C50 <= u && u < 0x010C80) || // 48
		//	U+10C80..U+10CFF	Old Hungarian
		//	U+10D00..U+10D3F	Hanifi Rohingya
//		(0x010D40 <= u && u < 0x010E60) || // 288
		//	U+10E60..U+10E7F	Rumi Numeral Symbols
		//	U+10E80..U+10EBF	Yezidi
		//	U+10EC0..U+10EFF	Arabic Extended-C (Uni 15.0)
		//	U+10F00..U+10F2F	Old Sogdian
		//	U+10F30..U+10F6F	Sogdian
		//	U+10F70..U+10FAF	Old Uyghur
		//	U+10FB0..U+10FDF	Chorasmian
		//	U+10FE0..U+10FFF	Elymaic
		//	U+11000..U+1107F	Brahmi
		//	U+11080..U+110CF	Kaithi
		//	U+110D0..U+110FF	Sora Sompeng
		//	U+11100..U+1114F	Chakma
		//	U+11150..U+1117F	Mahajani
		//	U+11180..U+111DF	Sharada
		//	U+111E0..U+111FF	Sinhala Archaic Numbers
		//	U+11200..U+1124F	Khojki
//		(0x011250 <= u && u < 0x011280) || // 48
		//	U+11280..U+112AF	Multani
		//	U+112B0..U+112FF	Khudawadi
		//	U+11300..U+1137F	Grantha
//		(0x011380 <= u && u < 0x011400) || //128
		//	U+11400..U+1147F	Newa
		//	U+11480..U+114DF	Tirhuta
//		(0x0114E0 <= u && u < 0x011580) || //160
		//	U+11580..U+115FF	Siddham
		//	U+11600..U+1165F	Modi
		//	U+11660..U+1167F	Mongolian Supplement
		//	U+11680..U+116CF	Takri
//		(0x0116D0 <= u && u < 0x011700) || //48
		//	U+11700..U+1174F	Ahom
//		(0x011750 <= u && u < 0x011800) || // 176
		//	U+11800..U+1184F	Dogra
//		(0x011850 <= u && u < 0x0118A0) || // 80
		//	U+118A0..U+118FF	Warang Citi
		//	U+11900..U+1195F	Dives Akuru
//		(0x011960 <= u && u < 0x0119A0) || // 64
		//	U+119A0..U+119FF	Nandinagari
		//	U+11A00..U+11A4F	Zanabazar Square
		//	U+11A50..U+11AAF	Soyombo
		//	U+11AB0..U+11ABF	Unified Canadian Aboriginal Syllabics Extended-A
		//	U+11AC0..U+11AFF	Pau Cin Hau
		//	U+11B00..U+11B5F	Devanagari Extended-A (Uni 15.0)
//		(0x011B60 <= u && u < 0x011C00) || // 160
		//	U+11C00..U+11C6F	Bhaiksuki
		//	U+11C70..U+11CBF	Marchen
//		(0x011CC0 <= u && u < 0x011D00) || // 64
		//	U+11D00..U+11D5F	Masaram Gondi
		//	U+11D60..U+11DAF	Gunjala Gondi
//		(0x011DB0 <= u && u < 0x011EE0) || // 304
		//	U+11EE0..U+11EFF	Makasar
		//	U+11F00..U+11F5F	Kawi (Uni 15.0)
//		(0x011F60 <= u && u < 0x011FB0) || // 80
		//	U+11FB0..U+11FBF	Lisu Supplement
		//	U+11FC0..U+11FFF	Tamil Supplement
		//	U+12000..U+123FF	Cuneiform
		//	U+12400..U+1247F	Cuneiform Numbers and Punctuation
		//	U+12480..U+1254F	Early Dynastic Cuneiform
		(/*0x012550 <= u &&*/ u < 0x012F90) || // 2624
		//	U+12F90..U+12FFF	Cypro-Minoan
		//	U+13000..U+1342F	Egyptian Hieroglyphs
		//	U+13430..U+1343F	Egyptian Hieroglyph Format Controls
		(0x013440 <= u && u < 0x014400) || // 4032
		//	U+14400..U+1467F	Anatolian Hieroglyphs
		(0x014680 <= u && u < 0x016800) || // 8576
		//	U+16800..U+16A3F	Bamum Supplement
		//	U+16A40..U+16A6F	Mro
		//	U+16A70..U+16ACF	Tangsa
		//	U+16AD0..U+16AFF	Bassa Vah
		//	U+16B00..U+16B8F	Pahawh Hmong
//		(0x016B90 <= u && u < 0x016E40) || // 688
		//	U+16E40..U+16E9F	Medefaidrin
//		(0x016EA0 <= u && u < 0x016F00) || // 96
		//	U+16F00..U+16F9F	Miao
//		(0x016FA0 <= u && u < 0x016FE0) || // 64
		//	U+16FE0..U+16FFF	Ideographic Symbols and Punctuation
		//	U+17000..U+187FF	Tangut
		//	U+18800..U+18AFF	Tangut Components
		//	U+18B00..U+18CFF	Khitan Small Script
		//	U+18D00..U+18D7F	Tangut Supplement
		(0x018D80 <= u && u < 0x01AFF0) || // 8816
		//	U+1AFF0..U+1AFFF	Kana Extended-B
		//	U+1B000..U+1B0FF	Kana Supplement
		//	U+1B100..U+1B12F	Kana Extended-A
		//	U+1B130..U+1B16F	Small Kana Extension
		//	U+1B170..U+1B2FF	Nushu
		(0x01B300 <= u && u < 0x01BC00) || // 2304
		//	U+1BC00..U+1BC9F	Duployan
		//	U+1BCA0..U+1BCAF	Shorthand Format Controls
		(0x01BCB0 <= u && u < 0x01CF00) || // 4688
		//	U+1CF00..U+1CFCF	Znamenny Musical Notation
//		(0x01CFD0 <= u && u < 0x01D000) || // 48
		//	U+1D000..U+1D0FF	Byzantine Musical Symbols
		//	U+1D100..U+1D1FF	Musical Symbols
		//	U+1D200..U+1D24F	Ancient Greek Musical Notation
//		(0x01D250 <= u && u < 0x01D2C0) || // 112
		//	U+1D2C0..U+1D2DF	Kaktovik Numerals (Uni 15.0)
		//	U+1D2E0..U+1D2FF	Mayan Numerals
		//	U+1D300..U+1D35F	Tai Xuan Jing Symbols
		//	U+1D360..U+1D37F	Counting Rod Numerals
//		(0x01D380 <= u && u < 0x01D400) || // 128
		//	U+1D400..U+1D7FF	Mathematical Alphanumeric Symbols
		//	U+1D800..U+1DAAF	Sutton SignWriting
		(0x01DAB0 <= u && u < 0x01DF00) || // 1104
		//	U+1DF00..U+1DFFF	Latin Extended-G
		//	U+1E000..U+1E02F	Glagolitic Supplement
		//	U+1E030..U+1E08F	Cyrillic Extended-D (Uni 15.0)
//		(0x01E090 <= u && u < 0x01E100) || // 112
		//	U+1E100..U+1E14F	Nyiakeng Puachue Hmong
//		(0x01E150 <= u && u < 0x01E290) || // 320
		//	U+1E290..U+1E2BF	Toto
		//	U+1E2C0..U+1E2FF	Wancho
//		(0x01E300 <= u && u < 0x01E4D0) || // 464
		//	U+1E4D0..U+1E4FF	Nag Mundari (Uni 15.0)
//		(0x01E500 <= u && u < 0x01E7E0) || // 736
		//	U+1E7E0..U+1E7FF	Ethiopic Extended-B
		//	U+1E800..U+1E8DF	Mende Kikakui
//		(0x01E8E0 <= u && u < 0x01E900) || // 32
		//	U+1E900..U+1E95F	Adlam
//		(0x01E960 <= u && u < 0x01EC70) || // 784
		//	U+1EC70..U+1ECBF	Indic Siyaq Numbers
//		(0x01ECC0 <= u && u < 0x01ED00) || // 64
		//	U+1ED00..U+1ED4F	Ottoman Siyaq Numbers
//		(0x01ED50 <= u && u < 0x01EE00) || // 176
		//	U+1EE00..U+1EEFF	Arabic Mathematical Alphabetic Symbols
//		(0x01EF00 <= u && u < 0x01F000) || // 256
		//	U+1F000..U+1F02F	Mahjong Tiles
		//	U+1F030..U+1F09F	Domino Tiles
		//	U+1F0A0..U+1F0FF	Playing Cards
		//	U+1F100..U+1F1FF	Enclosed Alphanumeric Supplement
		//	U+1F200..U+1F2FF	Enclosed Ideographic Supplement
		//	U+1F300..U+1F5FF	Miscellaneous Symbols and Pictographs
		//	U+1F600..U+1F64F	Emoticons
		//	U+1F650..U+1F67F	Ornamental Dingbats
		//	U+1F680..U+1F6FF	Transport and Map Symbols
		//	U+1F700..U+1F77F	Alchemical Symbols
		//	U+1F780..U+1F7FF	Geometric Shapes Extended
		//	U+1F800..U+1F8FF	Supplemental Arrows-C
		//	U+1F900..U+1F9FF	Supplemental Symbols and Pictographs
		//	U+1FA00..U+1FA6F	Chess Symbols
		//	U+1FA70..U+1FAFF	Symbols and Pictographs Extended-A
		//	U+1FB00..U+1FBFF	Symbols for Legacy Computing
		(0x01FC00 <= u && u < 0x020000) || // 1024
		//	U+20000..U+2A6DF	CJK Unified Ideographs Extension B
		//	U+2A700..U+2B73F	CJK Unified Ideographs Extension C
		//	U+2B740..U+2B81F	CJK Unified Ideographs Extension D
		//	U+2B820..U+2CEAF	CJK Unified Ideographs Extension E
		//	U+2CEB0..U+2EBEF	CJK Unified Ideographs Extension F
		(0x02EBF0 <= u && u < 0x02F800) || // 3088
		//	U+2F800..U+2FA1F	CJK Compatibility Ideographs Supplement
		(0x02FA20 <= u && u < 0x030000) || // 1504
		//	U+30000..U+3134F	CJK Unified Ideographs Extension G
		//	U+31350..U+323AF	CJK Unified Ideographs Extension H (Uni 15.0)
		(0x0323B0 <= u && u < 0x0E0000) || // 711760
		//	U+E0000..U+E007F	Tags
//		(0x0E0080 <= u && u < 0x0E0100) || // 128
		//	U+E0100..U+E01EF	Variation Selectors Supplement
		(0x0E01F0 <= u && u < 0x0F0000);   // 65040
		//	U+F0000..U+FFFFF	Supplementary Private Use Area-A
		//	U+100000..U+10FFFF	Supplementary Private Use Area-B
//		(0x110000 <= u); // 4293853183
}
bool TextFileR::IsAscii(uchar c) const { return 0x20 <= c && c < 0x80; }
bool TextFileR::IsSurrogateLead(qbyte w) const { return 0xD800 <= w && w <= 0xDBFF; }

bool TextFileR::CheckUTFConfidence(const uchar* ptr, size_t siz, unsigned char uChrSize, bool LE) const
{
	qbyte uchr = '\0';
	const size_t usize = siz / NZero(uChrSize);
	size_t unconfidence = 0, confidence = 0, x;
	size_t impossible = 0;
	bool prevIsNull = false;
	for( x=0; x < usize; x++ )
	{
		if(uChrSize == 2 && LE == true)
			uchr = ptr[x*2] | ptr[x*2+1]<<8;
		else if(uChrSize == 2 && LE == false)
			uchr = ptr[x*2+1] | ptr[x*2]<<8;
		else if(uChrSize == 4 && LE == true)
			uchr = ptr[x*4] | ptr[x*4+1]<<8 | ptr[x*4+2]<<16 | ptr[x*4+3]<<24;
		else if(uChrSize == 4 && LE == false)
			uchr = ptr[x*4+3] | ptr[x*4+2]<<8 | ptr[x*4+1]<<16 | ptr[x*4]<<24;

		if( IsNonUnicodeRange(uchr) || (uChrSize==2 && uchr==0) ) // \0\0 maybe a part of UTF-32
		{
			impossible++; // tolerate 0.1% impossible sequence
			if( impossible > usize>>10 )
				break;
		}
		if( uchr < 0x80 ) confidence+=4; // unicode ASCII
		else if(uChrSize==2 && IsAscii(ptr[x*2]) && IsAscii(ptr[x*2+1])) // both char are ASCII
		{
			confidence+=2;
			++unconfidence;
		}
		else if(uChrSize==2 && IsSurrogateLead(uchr)) ++unconfidence; // Surrogate pairs are less-used
		else ++confidence; // other Unicode chars
		if(uChrSize == 2 && prevIsNull && (LE ? !ptr[x*2] : !ptr[x*2+1])) ++unconfidence; // assume successive U+xx00 is less chance appeared

		if(uChrSize == 2)
			prevIsNull = LE ? !ptr[x*2] : !ptr[x*2+1];
	}
# ifdef UTF_DEBUG
	TCHAR utfTmp[128];
	::wsprintf(utfTmp,
		TEXT("uChrSize=%d, LE=%d, usize=%d\n"
		     "confidence=%d, unconfidence=%d\n"
		     "impossible=%d\n"
		     "result=%d" )
		, uChrSize, LE, usize
		, confidence,unconfidence
		, impossible
		, impossible <= usize>>10 && confidence-unconfidence > usize
	);
	::MessageBox(GetActiveWindow(), utfTmp, TEXT("UTFDetect"), 0);
# endif

	return impossible <= usize>>10 && confidence-unconfidence > usize;
}

//=========================================================================
// テキストファイル出力共通インターフェイス
//=========================================================================
struct ki::TextFileWPimpl : public TmpObject
{
	virtual void WriteLine( const unicode* buf, size_t siz )
		{ while( siz-- ) WriteChar( *buf++ ); }

	virtual void WriteLB( const unicode* buf, size_t siz )
		{ WriteLine( buf, siz ); }

	virtual void WriteChar( unicode ch )
		{}

	virtual ~TextFileWPimpl() {}

protected:

	explicit TextFileWPimpl( FileW& w )
		: fp_   (w) {}

	FileW& fp_;
private:
	NOCOPY(TextFileWPimpl);
};

struct TextFileWPimplWithBuf: public ki::TextFileWPimpl
{
	enum { bsiz_ = 8192, bstep_ = bsiz_ / 4 };
	explicit TextFileWPimplWithBuf( FileW& w )
		: ki::TextFileWPimpl( w )
		{}

//	~TextFileWPimplWithBuf( )
//		{};

	// This function should assume it will have enough room in buf_
	virtual void WriteBuf( const unicode* str, size_t len ) = 0;

	void WriteLine( const unicode* str, size_t len ) override A_FINAL
	{
		// Write by blocks,
		// in case the line is larger than the temporary buffer.
		while( len > bstep_ )
		{
			// If we meet a High surrogate we must make a 1 unicode char smaller step.
			// So that we do not split surrogate pairs in their middle before decoding.
			size_t step = bstep_ - ISHIGHSURROGATE( str[bstep_-1] );

			// Actually write buffer to the file!
			WriteBuf( str, step );
			len -= step;
			str += step;
		}
		// Remaining
		if( len )
			WriteBuf( str, len );
	}

protected:
	char  buf_[bsiz_];
};
//-------------------------------------------------------------------------
// Unicodeテキスト
//-------------------------------------------------------------------------

struct wUtf16LE A_FINAL: public TextFileWPimpl
{
	explicit wUtf16LE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom){ unicode ch=0xfeff; fp_.Write(&ch,2); } }
	void WriteLine( const unicode* buf, size_t siz ) override
		{ fp_.Write(buf, siz*2); }
};

struct wUtf16BE A_FINAL: public TextFileWPimpl
{
	explicit wUtf16BE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) WriteChar(0xfeff); }
	void WriteChar( unicode ch ) override
		{ fp_.NeedSpace(2), fp_.WriteCN(ch>>8), fp_.WriteCN(ch&0xff); }
	void WriteLine( const unicode* buf, size_t siz ) override
	{
		while( siz >= 2 )
		{
			siz-=2;
			fp_.NeedSpace(4);
			fp_.WriteCN( (*buf)>>8 ), fp_.WriteCN( (*buf)&0xff ), buf++;
			fp_.WriteCN( (*buf)>>8 ), fp_.WriteCN( (*buf)&0xff ), buf++;
		}
		if( siz ) // Last char
			fp_.NeedSpace(2), fp_.WriteCN( (*buf)>>8 ), fp_.WriteCN( (*buf)&0xff  );
	}
};

struct wUtf32LE A_FINAL: public TextFileWPimpl
{
	explicit wUtf32LE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) {unicode c=0xfeff; WriteLine(&c,1);} }
//	void WriteChar( unicode ch )
//		{ fp_.WriteC(ch&0xff), fp_.WriteC(ch>>8), fp_.WriteC(0), fp_.WriteC(0); }
	virtual void WriteLine( const unicode* buf, size_t siz ) override
	{
		while( siz-- )
		{
			unicode c = *buf++;
			qbyte  cc = c;
			if( (0xD800<=c && c<=0xDBFF) && siz>0 ) // trail char が正しいかどうかはチェックする気がない
			{
				unicode c2 = *buf++; siz--;
				cc = 0x10000 + (((c-0xD800)&0x3ff)<<10) + ((c2-0xDC00)&0x3ff);
			}
			fp_.NeedSpace(4);
			fp_.WriteCN( (uchar)(cc>>(8*0)) );
			fp_.WriteCN( (uchar)(cc>>(8*1)) );
			fp_.WriteCN( (uchar)(cc>>(8*2)) );
			fp_.WriteCN( (uchar)(cc>>(8*3)) );
		}
	}
};

struct wUtf32BE A_FINAL: public TextFileWPimpl
{
	explicit wUtf32BE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) {unicode c=0xfeff; WriteLine(&c,1);} }
//	void WriteChar( unicode ch )
//		{ fp_.WriteC(0), fp_.WriteC(0), fp_.WriteC(ch>>8), fp_.WriteC(ch&0xff); }
	virtual void WriteLine( const unicode* buf, size_t siz ) override
	{
		while( siz-- )
		{
			unicode c = *buf++;
			qbyte  cc = c;
			if( (0xD800<=c && c<=0xDBFF) && siz>0 ) // trail char が正しいかどうかはチェックする気がない
			{
				unicode c2 = *buf++; siz--;
				cc = 0x10000 + (((c-0xD800)&0x3ff)<<10) + ((c2-0xDC00)&0x3ff);
			}
			fp_.NeedSpace(4);
			fp_.WriteCN( (uchar)(cc>>(8*3)) );
			fp_.WriteCN( (uchar)(cc>>(8*2)) );
			fp_.WriteCN( (uchar)(cc>>(8*1)) );
			fp_.WriteCN( (uchar)(cc>>(8*0)) );
		}
	}
};

struct wWestISO88591 A_FINAL: public TextFileWPimpl
{
	explicit wWestISO88591( FileW& w ) : TextFileWPimpl(w) {}
	void WriteChar( unicode ch ) override { fp_.WriteC(ch>0xff ? '?' : (uchar)ch); }
	virtual void WriteLine( const unicode* buf, size_t siz ) override
	{
		#define SHORTEN(ch) (ch)>0xff ? '?' : (uchar)(ch)
		while( siz >= 4 )
		{
			siz-=4;
			fp_.NeedSpace(4);
			fp_.WriteCN(SHORTEN(*buf)), buf++;
			fp_.WriteCN(SHORTEN(*buf)), buf++;
			fp_.WriteCN(SHORTEN(*buf)), buf++;
			fp_.WriteCN(SHORTEN(*buf)), buf++;
		}
		fp_.NeedSpace(3);
		while( siz-- )
			fp_.WriteCN(SHORTEN(*buf)), buf++;
	}
};

struct wUtf1 A_FINAL: public TextFileWPimpl
{
	wUtf1( FileW& w, bool bom ) : TextFileWPimpl(w), SurrogateHi(0)
	{
		if( bom ) // BOM書き込み
			fp_.Write( "\xF7\x64\x4C", 3 );
	}

	qbyte SurrogateHi;

	inline qbyte conv ( qbyte x )
	{
		if( x<=0x5D )      return x + 0x21;
		else if( x<=0xBD ) return x + 0x42;
		else if( x<=0xDE ) return x - 0xBE;
		else               return x - 0x60;
	}
	void WriteChar( unicode ch ) override
	{
		qbyte c = ch;
		fp_.NeedSpace(5);
		if( c <= 0x9f )
			fp_.WriteCN( static_cast<uchar>(c) );
		else if( c <= 0xff )
			fp_.WriteCN( 0xA0 ),
			fp_.WriteCN( static_cast<uchar>(c) );
		else if( c <= 0x4015 )
			fp_.WriteCN( static_cast<uchar>(0xA1 + (c - 0x100) / 0xBE) ),
			fp_.WriteCN( static_cast<uchar>(conv((c - 0x100) % 0xBE)) );
		else if( c < 0xD800 )
			fp_.WriteCN( static_cast<uchar>(0xF6 + (c - 0x4016) / (0xBE*0xBE))  ),
			fp_.WriteCN( static_cast<uchar>(conv((c - 0x4016) / 0xBE % 0xBE)) ),
			fp_.WriteCN( static_cast<uchar>(conv((c - 0x4016) % 0xBE)) );
		else // c >= 0xD800
		{
			if( ch<=0xDBFF )
			{
				SurrogateHi = c; return;
			}
			else if( ch<=0xDFFF )
				if( SurrogateHi )
					c = 0x10000 + (((SurrogateHi-0xD800)&0x3ff)<<10) + ((c-0xDC00)&0x3ff), SurrogateHi = 0;
				else return; // find Surrogate Low part only, discard it
			else // find Surrogate Hi part only, discard it
				SurrogateHi = 0;

			if( c <= 0x38E2D )
				fp_.WriteCN( static_cast<uchar>(0xF6 + (c - 0x4016) / (0xBE*0xBE))  ),
				fp_.WriteCN( static_cast<uchar>(conv((c - 0x4016) / 0xBE % 0xBE)) ),
				fp_.WriteCN( static_cast<uchar>(conv((c - 0x4016) % 0xBE)) );
			else
				fp_.WriteCN( static_cast<uchar>(0xFC + (c - 0x38E2E) / (0xBE*0xBE*0xBE*0xBE))  ),
				fp_.WriteCN( static_cast<uchar>(conv((c - 0x38E2E) / (0xBE*0xBE*0xBE) % 0xBE)) ),
				fp_.WriteCN( static_cast<uchar>(conv((c - 0x38E2E) / (0xBE*0xBE) % 0xBE)) ),
				fp_.WriteCN( static_cast<uchar>(conv((c - 0x38E2E) / 0xBE % 0xBE)) ),
				fp_.WriteCN( static_cast<uchar>(conv((c - 0x38E2E) % 0xBE)) );
		}
	}
};

struct wUtf9 A_FINAL: public TextFileWPimpl
{
	wUtf9( FileW& w, bool bom ) : TextFileWPimpl(w), SurrogateHi(0)
	{
		if( bom ) // BOM書き込み
			fp_.Write( "\x93\xFD\xFF", 3 );
	}

	qbyte SurrogateHi;

	void WriteChar( unicode ch ) override
	{
		qbyte c = ch;
		fp_.NeedSpace(4);
		if( c <= 0x7F || (c >= 0xA0 && c <= 0xFF ))
			fp_.WriteCN( static_cast<uchar>(c) );
		else if( c <= 0x07FF )
			fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7)        ) ),
			fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)      ) );
		else if( c < 0xD800 ) // Before high surrogate
			fp_.WriteCN( static_cast<uchar>(0x90 | (c >> 14)       ) ),
			fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F ) ),
			fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)      ) );
		else // ch >= 0xD800 (High surrog or higher)
		{
			if( ch<=0xDBFF )
			{
				SurrogateHi = c; return;
			}
			else if( ch<=0xDFFF )
				if( SurrogateHi )
					c = 0x10000 + (((SurrogateHi-0xD800)&0x3ff)<<10) + ((c-0xDC00)&0x3ff), SurrogateHi = 0;
				else return; // find Surrogate Low part only, discard it
			else // find Surrogate Hi part only, discard it
				SurrogateHi = 0;

			if( c <= 0xFFFF )
				fp_.WriteCN( static_cast<uchar>(0x90 | (c >> 14)       ) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F ) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)      ) );
			else // if( c <= 0x7FFFFF )
				fp_.WriteCN( static_cast<uchar>(0x94 | (c >> 21)       ) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F ) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)      ) );
//			else // Unrecheabe because we start from UTF-16
//				fp_.WriteCN( static_cast<uchar>(0x98 | (c >> 28) & 0x07) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 21) & 0x7F) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F ) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)      ) );
		}
	}
};

struct wUtfOFSS A_FINAL: public TextFileWPimpl
{
	wUtfOFSS( FileW& w, bool bom ) : TextFileWPimpl(w), SurrogateHi(0)
	{
		if( bom ) // BOM書き込み
			fp_.Write( "\xC3\xBC\xFF", 3 );
	}

	qbyte SurrogateHi;

	void WriteChar( unicode ch ) override
	{
		qbyte c = ch;

		fp_.NeedSpace(4);
		if( c <= 0x7F )
			fp_.WriteCN( static_cast<uchar>(c) );
		else if( c <= 0x1fff + 0x0000080 )
			c -= 0x0000080,
			fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7)) ),
			fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else
		{
			if( 0xD800<=ch&&ch<=0xDBFF )
			{
				SurrogateHi = c; return;
			}
			else if( 0xDC00<=ch&&ch<=0xDFFF )
				if( SurrogateHi )
					c = 0x10000 + (((SurrogateHi-0xD800)&0x3ff)<<10) + ((c-0xDC00)&0x3ff), SurrogateHi = 0;
				else return; // find Surrogate Low part only, discard it
			else // find Surrogate Hi part only, discard it
				SurrogateHi = 0;

			if( c <= 0x7ffff + 0x0002080 )
				c -= 0x0002080,
				fp_.WriteCN( static_cast<uchar>(0xc0 | (c >> 14)) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)) );
			else // if( c <= 0x1ffffff + 0x0082080 )
				c -= 0x0082080,
				fp_.WriteCN( static_cast<uchar>(0xe0 | (c >> 21)) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
				fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)) );
//			else // Uncrachable if we start from UTF-16
//				c -= 0x2082080,
//				fp_.WriteCN( static_cast<uchar>(0xf0 | (c >> 28) & 0x07) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 21) & 0x7F) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
//				fp_.WriteCN( static_cast<uchar>(0x80 | (c & 0x7F)) );
		}
	}
};

struct wUtf5 A_FINAL: public TextFileWPimpl
{
	explicit wUtf5( FileW& w, bool bom ) : TextFileWPimpl(w)
	{
		if( bom )
			fp_.Write("\x56\x45\x46\x46", 4);
	}
	void WriteChar( unicode ch ) override
	{
		fp_.NeedSpace(4);
		static const char conv[] = {
			'0','1','2','3','4','5','6','7',
				'8','9','A','B','C','D','E','F' };
		if(ch<0x10)
		{
			fp_.WriteCN(ch+'G');
		}
		else if(ch<0x100)
		{
			fp_.WriteCN((ch>>4)+'G');
			fp_.WriteCN(conv[ch&0xf]);
		}
		else if(ch<0x1000)
		{
			fp_.WriteCN((ch>>8)+'G');
			fp_.WriteCN(conv[(ch>>4)&0xf]);
			fp_.WriteCN(conv[ch&0xf]);
		}
		else
		{
			fp_.WriteCN((ch>>12)+'G');
			fp_.WriteCN(conv[(ch>>8)&0xf]);
			fp_.WriteCN(conv[(ch>>4)&0xf]);
			fp_.WriteCN(conv[ch&0xf]);
		}
	}
};

//-------------------------------------------------------------------------
// SCSU Encoder
// Code portion from http://unicode.org/Public/PROGRAMS/SCSUMini/scsumini.c
// created by: Markus W. Scherer
//-------------------------------------------------------------------------

namespace {
	static const WORD SCSU_offsets[16]={
		/* initial offsets for the 8 dynamic (sliding) windows */
		0x0080, /* Latin-1 */
		0x00C0, /* Latin Extended A */
		0x0400, /* Cyrillic */
		0x0600, /* Arabic */
		0x0900, /* Devanagari */
		0x3040, /* Hiragana */
		0x30A0, /* Katakana */
		0xFF00, /* Fullwidth ASCII */

		/* offsets for the 8 static windows */
		0x0000, /* ASCII for quoted tags */
		0x0080, /* Latin - 1 Supplement (for access to punctuation) */
		0x0100, /* Latin Extended-A */
		0x0300, /* Combining Diacritical Marks */
		0x2000, /* General Punctuation */
		0x2080, /* Currency Symbols */
		0x2100, /* Letterlike Symbols and Number Forms */
		0x3000  /* CJK Symbols and punctuation */
	};
}
struct wSCSU A_FINAL: public TextFileWPimpl
{
	/* SCSU command byte values */
	enum {
		SQ0=0x01, /* Quote from window pair 0 */
		SQU=0x0E, /* Quote a single Unicode character */
		SCU=0x0F, /* Change to Unicode mode */
		SC0=0x10, /* Select window 0 */

		UC0=0xE0, /* Select window 0 */
		UQU=0xF0  /* Quote a single Unicode character */
	};

	explicit wSCSU( FileW& w, bool bom ) : TextFileWPimpl(w), isUnicodeMode(0), win(0)
	{
		if( bom ) WriteChar( 0xfeff ); // Write BOM
	}

	char isUnicodeMode, win;

	char isInWindow(uint offset, uint c)
	{
		return (char)(offset<=c && c<=(offset+0x7f));
	}

	// get the index of the static/dynamic window that contains c; -1 if none
	int getWindow(uint c)
	{
		int i;

		for(i=0; i<16; ++i) {
			if(isInWindow(SCSU_offsets[i], c)) {
				return i;
			}
		}
		return -1;
	}

	void WriteChar( unicode c ) override
	{
		uchar window; // dynamic window 0..7
		int w;       // result of getWindow(), -1..7

		if( (isUnicodeMode&~1)!=0 || (win&~7)!=0 )
		{
			return;
		}

		window=win;
		if( !isUnicodeMode )
		{
			// single-byte mode
			if(c<0x20)
			{
				/*
				* Encode C0 control code:
				* Check the code point against the bit mask 0010 0110 0000 0001
				* which contains 1-bits at the bit positions corresponding to
				* code points 0D 0A 09 00 (CR LF TAB NUL)
				* which are encoded directly.
				* All other C0 control codes are quoted with SQ0.
				*/
				if( ((1u<<c)&0x2601u) == 0 ) {
					fp_.WriteC( static_cast<uchar>(SQ0) );
				}
				fp_.WriteC( static_cast<uchar>(c) );
			}
			else if( c<=0x7f )
			{
				// encode US-ASCII directly
				fp_.WriteC( static_cast<uchar>(c) );
			}
			else if( isInWindow(SCSU_offsets[window], c) )
			{
				// use the current dynamic window
				fp_.WriteC( static_cast<uchar>(0x80+(c-SCSU_offsets[window])) );
			}
			else if( (w=getWindow(c))>=0 )
			{
				if( w<=7 )
				{
					// switch to a dynamic window
					fp_.WriteC( static_cast<uchar>(SC0+w) );
					fp_.WriteC( static_cast<uchar>(0x80+(c-SCSU_offsets[w])) );
					win=window=(char)w;
				}
				else
				{
					// quote from a static window
					fp_.WriteC( static_cast<uchar>(SQ0+(w-8)) );
					fp_.WriteC( static_cast<uchar>(c-SCSU_offsets[w]) );
				}
			}
			else if( c==0xfeff )
			{
				// encode the signature character U+FEFF with SQU
				fp_.WriteC( static_cast<uchar>(SQU) );
				fp_.WriteC( static_cast<uchar>(0xfe) );
				fp_.WriteC( static_cast<uchar>(0xff) );
			}
			else
			{
				// switch to Unicode mode
				fp_.WriteC( static_cast<uchar>(SCU) );
				isUnicodeMode=1;
				WriteChar( c );
			}
		}
		else
		{
			/* Unicode mode */
			if( c<=0x7f )
			{
				// US-ASCII: switch to single-byte mode with the previous dynamic window
				isUnicodeMode=0;
				fp_.WriteC( static_cast<uchar>(UC0+window) );
				WriteChar( c );
			}
			else if( (w=getWindow(c))>=0 && w<=7 )
			{
				// switch to single-byte mode with a matching dynamic window
				fp_.WriteC( static_cast<uchar>(UC0+w) );
				win=window=(char)w;
				isUnicodeMode=0;
				WriteChar( c );
			}
			else
			{
				if( 0xe000<=c && c<=0xf2ff )
				{
					fp_.WriteC( static_cast<uchar>(UQU) );
				}
				fp_.WriteC( static_cast<uchar>(c>>8) );
				fp_.WriteC( static_cast<uchar>(c) );
			}
		}
	}
};
//-------------------------------------------------------------------------
// BOCU-1
// code portion from BOCU1.pm by Naoya Tozuka
//-------------------------------------------------------------------------
namespace {
	static const uchar bocu1_trail_to_byte[243] = {
//   0 - 19 (0x0 - 0x13)
	      0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,             0x1c, 0x1d, 0x1e, 0x1f,
//   20 - 242 (0x14 - 0xf2)
	      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };
}
struct wBOCU1 A_FINAL: public TextFileWPimpl
{
	explicit wBOCU1( FileW& w, bool bom ) : TextFileWPimpl(w), cp ( 0 ) , pc ( 0x40 ), diff( 0 )
	{ // write BOM + Reset byte in case files are catenated...
		if( bom )
			fp_.Write( "\xFB\xEE\x28\xFF", 4 );
		else
			fp_.WriteC( '\xFF' ); // Reset byte.
	}

	unicode cp, pc;
	long diff;

	void WriteChar( unicode ch ) override
	{
		uchar t1,t2,t3;

		fp_.NeedSpace(4);
		if (ch <= 0x20) {
			if(ch != 0x20) pc = 0x40;
			fp_.WriteCN( static_cast<uchar>(ch) );
		} else {
			diff = ch - pc;
			if (diff < -187660) { // [...,-187660) : 21
				diff -= -14536567;
				t3 = (uchar)(diff % 243); diff/=243;
				t2 = (uchar)(diff % 243); diff/=243;
				t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteCN( static_cast<uchar>(0x21) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t3 ]) );
			} else if (diff < -10513) { // [-187660,-10513) : 22-24
				diff -= -187660;
				t2 = (uchar)(diff % 243); diff/=243;
				t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteCN( static_cast<uchar>(0x22 + diff) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
			} else if (diff < -64) { // [-10513,-64) : 25-4F
				diff -= -10513;
				t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteCN( static_cast<uchar>(0x25 + diff) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
			} else if (diff < 64) { // [-64,63) : 50-CF
				diff -= -64;
				fp_.WriteCN( static_cast<uchar>(0x50 + diff) );
			} else if (diff < 10513) { // [64,10513) : D0-FA
				diff -= 64;
				t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteCN( static_cast<uchar>(0xd0 + diff) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
			} else if (diff < 187660) { // [10513,187660) : FB-FD
				diff -= 10513;
				t2 = (uchar)(diff % 243); diff/=243;
				t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteCN( static_cast<uchar>(0xfb + diff) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
			} else { // [187660,...) : FE
				diff -= 187660;
				t3 = (uchar)(diff % 243); diff/=243;
				t2 = (uchar)(diff % 243); diff/=243;
				t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteCN( static_cast<uchar>(0xfe) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
				fp_.WriteCN( static_cast<uchar>(bocu1_trail_to_byte[ t3 ]) );
			}

			// next pc
			if (0x3040 <= ch && ch <= 0x309f) {
				pc = 0x3070;
			} else if (0x4e00 <= ch && ch <= 0x9fa5) {
				pc = 0x7711;
			} else if (0xac00 <= ch && ch <= 0xd7a3) {
				pc = 0xc1d1;
			} else {
				pc = ch & ~0x7f | 0x40;
			}
		}

	}
};


//-------------------------------------------------------------------------
// Win95対策の自前UTF8/UTF7処理
//-------------------------------------------------------------------------
//#ifndef _UNICODE

struct wUTF8 A_FINAL: public TextFileWPimpl
{
	explicit wUTF8( FileW& w, int cp )
		: TextFileWPimpl(w)
	{
		if( cp == UTF8 ) // BOM書き込み
			fp_.Write( "\xEF\xBB\xBF", 3 );
	}

	void WriteLine( const unicode* str, size_t len ) override
	{
		// Because we start from UTF-16 we are limited to 4 byte UTF-8 sequence
		// BOM + surrog pair = 65536-2048 + 1024*1024  = 1 112 064 codepoints
		//        0000-0000-0xxx-xxxx | 0xxxxxxx
		//        0000-0xxx-xxyy-yyyy | 110xxxxx 10yyyyyy
		//        xxxx-yyyy-yyzz-zzzz | 1110xxxx 10yyyyyy 10zzzzzz
		// x-xxyy-yyyy-zzzz-zzww-wwww | 11110xxx 10yyyyyy 10zzzzzz 10wwwwww
		// ...
		while( len-- )
		{
			qbyte ch = *str;

			fp_.NeedSpace(4);

			if( ch <= 0x7f )
				fp_.WriteCN( static_cast<uchar>(ch) );
			else if( ch <= 0x7ff )
				fp_.WriteCN( 0xc0 | static_cast<uchar>(ch>>6) ),
				fp_.WriteCN( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch < 0xD800 ) // 3 byte sequence before surrogate.
				fp_.WriteCN( 0xe0 | static_cast<uchar>(ch>>12) ),
				fp_.WriteCN( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteCN( 0x80 | static_cast<uchar>(ch&0x3f) );
			else // ( ch >= 0xD800 )
			{
				if( ch <= 0xDBFF && len )
					ch = 0x10000 + (((ch-0xD800)&0x3ff)<<10) + ((*++str-0xDC00)&0x3ff), len--;

				if( ch<= 0xffff ) // 3 byte sequence after surrogate.
					fp_.WriteCN( 0xe0 | static_cast<uchar>(ch>>12) ),
					fp_.WriteCN( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
					fp_.WriteCN( 0x80 | static_cast<uchar>(ch&0x3f) );
				else // if( ch<= 0x1fffff )
					fp_.WriteCN( 0xf0 | static_cast<uchar>(ch>>18) ),
					fp_.WriteCN( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
					fp_.WriteCN( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
					fp_.WriteCN( 0x80 | static_cast<uchar>(ch&0x3f) );
//				else if( ch<= 0x3ffffff )
//					fp_.WriteC( 0xf8 | static_cast<uchar>(ch>>24) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>18)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
//				else
//					fp_.WriteC( 0xfc | static_cast<uchar>(ch>>30) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>24)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>18)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
//					fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			}
			++str;
		}
	}
};



namespace {
	static const uchar mime[64] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'};

	static const uchar moredirect[13] = {
		'\t','\n','\r', ' ', // UTF-7 Rule 3 (RFC 2152)
		'\'', '(', ')', ',', '-', '.', '/', ':', '?'}; // Always OK
}
struct wUTF7 A_FINAL: public TextFileWPimpl
{
	bool direct[128];
	explicit wUTF7( FileW& w ) : TextFileWPimpl(w)
	{
		mem00( direct, sizeof(direct) );
		// A-Z, a-Z, 0-9, /, are directly encoded.
		for( size_t i=0; i < countof(mime) ; i++ )
			direct[mime[i]] = true;
		// More directtly encoded chars.
		for( size_t i=0; i < countof(moredirect) ; i++ )
			direct[moredirect[i]] = true;
	}

	void WriteLine( const unicode* str, size_t len ) override
	{
		// XxxxxxYyyyyyZzzz | zzWwwwwwUuuuuuVv | vvvvTtttttSsssss
		bool mode_m = false;
		while( len )
		{
			if( *str <= 0x7f && direct[*str] )
			{
				fp_.WriteC( static_cast<uchar>(*str) );
				if( *str == L'+' )
					fp_.WriteC( '-' );
				++str, --len;
			}
			else
			{
				fp_.NeedSpace(10);
				if(!mode_m) fp_.WriteCN( '+' ), mode_m=true;
				unicode tx[3] = {0,0,0};
				int n=0;
				tx[0] = *str, ++str, --len, ++n;
				if( len && *str>0x7f )
				{
					tx[1] = *str, ++str, --len, ++n;
					if( len && *str>0x7f )
						tx[2] = *str, ++str, --len, ++n;
				}
				{
					fp_.WriteCN( mime[ tx[0]>>10 ] );
					fp_.WriteCN( mime[ (tx[0]>>4)&0x3f ] );
					fp_.WriteCN( mime[ (tx[0]<<2|tx[1]>>14)&0x3f ] );
					if( n>=2 )
					{
						fp_.WriteCN( mime[ (tx[1]>>8)&0x3f ] );
						fp_.WriteCN( mime[ (tx[1]>>2)&0x3f ] );
						fp_.WriteCN( mime[ (tx[1]<<4|tx[2]>>12)&0x3f ] );
						if( n>=3 )
						{
							fp_.WriteCN( mime[ (tx[2]>>6)&0x3f ] );
							fp_.WriteCN( mime[ tx[2]&0x3f ] );
						}
					}
				}
				if( len && *str<=0x7f )
					fp_.WriteCN( '-' ), mode_m = false;
			}
		}
	}
};

namespace
{
static const uchar i8_to_ubcd[256] = {
	 0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x15,0x0B,0x0C,0x0D,0x0E,0x0F
	,0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F
	,0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61
	,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F
	,0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6
	,0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xAD,0xE0,0xBD,0x5F,0x6D
	,0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96
	,0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07
	,0x20,0x21,0x22,0x23,0x24,0x25,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B
	,0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xFF
	,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x51,0x52,0x53,0x54,0x55,0x56
	,0x57,0x58,0x59,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x70,0x71,0x72,0x73
	,0x74,0x75,0x76,0x77,0x78,0x80,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x9A,0x9B,0x9C
	,0x9D,0x9E,0x9F,0xA0,0xAA,0xAB,0xAC,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6
	,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBE,0xBF,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xDA,0xDB
	,0xDC,0xDD,0xDE,0xDF,0xE1,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xFA,0xFB,0xFC,0xFD,0xFE
};

}
struct wUtfEBCDIC A_FINAL: public TextFileWPimpl
{
	wUtfEBCDIC( FileW& w, bool bom ) :
	TextFileWPimpl( w ),
	SurrogateHi( 0 )
	{
		if( bom ) // BOM書き込み
			fp_.Write( "\xDD\x73\x66\x73", 4 );
	}

	qbyte SurrogateHi;

	void WriteChar( unicode ch ) override
	{
		qbyte c = ch;
		fp_.NeedSpace(5);
		#define WM(x) fp_.WriteCN( i8_to_ubcd[static_cast<uchar>(x)] )
		if( c <= 0x9F )
			WM( c ); // Direct mapping...
		else if( c <= 0x03FF )
			WM( 0xC0 | (c >> 5)        ), // L2 110xxxxx
			WM( 0xA0 | (c & 0x1F)      ); // T1 101yyyyy
		else if( c <= 0x3FFF ) // Before high surrogate
			WM( 0xE0 | ( c >> 10 )           ), // L3 1110xxxx
			WM( 0xA0 | (( c >> 5  ) & 0x1F)  ), // T1 101yyyyy
			WM( 0xA0 | (( c       ) & 0x1F)  ); // T2 101zzzzz
		else if( c < 0xD800 ) // Before high surrogate (4 bytes)
			WM( 0xF0 | ( c >> 15 )         ), // L4 11110xxx
			WM( 0xA0 | ((c >> 10 ) & 0x1F) ), // T1 101yyyyy
			WM( 0xA0 | ((c >>  5 ) & 0x1F) ), // T2 101zzzzz
			WM( 0xA0 | ((c       ) & 0x1F) ); // T3 101ttttt
		else // ch >= 0xD800 (High surrog or higher)
		{
			if( c<=0xDBFF )
			{
				SurrogateHi = c; return;
			}
			else if( c<=0xDFFF )
				if( SurrogateHi )
					c = 0x10000 + (((SurrogateHi-0xD800)&0x3ff)<<10) + ((c-0xDC00)&0x3ff), SurrogateHi = 0;
				else return; // find Surrogate Low part only, discard it
			else // find Surrogate Hi part only, discard it
				SurrogateHi = 0;

			if( c <= 0x3FFFF ) // 4 bytes again
				WM(   0xF0 |  (c >> 15)         ), // L4 11110xxx
				WM(   0xA0 | ((c >> 10) & 0x1F) ), // T1 101yyyyy
				WM(   0xA0 | ((c >>  5) & 0x1F) ), // T2 101zzzzz
				WM(   0xA0 | ((c >>  0) & 0x1F) ); // T3 101ttttt
			else // if( c <= 0x10FFFF ) // 1 00001 11111 11111 11111
				WM( 0xF8 | ( c >> 20)         ), // L5 1111100x
				WM( 0xA0 | ((c >> 15) & 0x1F) ), // T1 101yyyyy
				WM( 0xA0 | ((c >> 10) & 0x1F) ), // T2 101zzzzz
				WM( 0xA0 | ((c >>  5) & 0x1F) ), // T3 101ttttt
				WM( 0xA0 | ((c >>  0) & 0x1F) ); // T4 101uuuuu
		}
		#undef WM
	}
};
//#endif
//-------------------------------------------------------------------------
// Windows頼りの変換
//-------------------------------------------------------------------------

struct wMBCS A_FINAL: public TextFileWPimpl
{
	explicit wMBCS( FileW& w, int cp )
		: TextFileWPimpl(w), cp_(cp)
	{
		if( cp == UTF8 )
		{
			// BOM書き込み
			cp_ = UTF8N;
			fp_.Write( "\xEF\xBB\xBF", 3 );
		}
		if( cp == GB18030Y )
		{
			// BOM書き込み
			cp_ = GB18030;
			fp_.Write( "\x84\x31\x95\x33", 4 );
		}
	}

	// Directly write into the FileW buffer (lower mem usage).
	void WriteLine( const unicode* str, size_t len ) override
	{
		// WideCharToMultiByte API を利用した変換
		fp_.WriteInCodepageFromUnicode( cp_, str, len );
	}

private:
	int cp_;
};



//-------------------------------------------------------------------------
// ISO-2022 サブセットその１。
// ASCIIともう一つしか文字集合を使わないもの
//-------------------------------------------------------------------------

struct wIso2022 A_FINAL: public TextFileWPimplWithBuf
{
	explicit wIso2022( FileW& w, int cp )
		: TextFileWPimplWithBuf(w), hz_(cp==HZ)
	{
		switch( cp )
		{
		case IsoKR:
			fp_.Write( "\x1B\x24\x29\x43", 4 );
			cp_ = UHC;
			break;

		case IsoCN:
			fp_.Write( "\x1B\x24\x29\x41", 4 );
			// fall through...
		default:
			cp_ = GBK;
			break;
		}
	}

	void WriteBuf( const unicode* str, size_t len ) override
	{
		// まず WideCharToMultiByte API を利用して変換
		int r=::WideCharToMultiByte(cp_, 0, str, len, buf_, bsiz_,NULL,NULL);

		bool ascii = true;
		for( int i=0; i<r; ++i )
		{
			fp_.NeedSpace(4);
			if( buf_[i] & 0x80 )
			{
				// 非ASCII部分は最上位ビットを落としてから出力
				if( ascii )
				{
					if( hz_ )
						fp_.WriteCN( 0x7E ), fp_.WriteCN( 0x7B );
					else
						fp_.WriteCN( 0x0E );
					ascii = false;
				}
				fp_.WriteCN( buf_[i++] & 0x7F );
				fp_.WriteCN( buf_[i]   & 0x7F );
			}
			else
			{
				// ASCII部分はそのまま出力
				if( !ascii )
				{
					if( hz_ )
						fp_.WriteCN( 0x7E ), fp_.WriteCN( 0x7D );
					else
						fp_.WriteCN( 0x0F );
					ascii = true;
				}
				fp_.WriteCN( buf_[i] );

				// ただしHZの場合、0x7E は 0x7E 0x7E と表す
				if( hz_ && buf_[i]==0x7E )
					fp_.WriteCN( 0x7E );
			}
		}

		// 最後は確実にASCIIに戻す
		if( !ascii )
		{
			fp_.NeedSpace(2);
			if( hz_ )
				fp_.WriteCN( 0x7E ), fp_.WriteCN( 0x7D );
			else
				fp_.WriteCN( 0x0F );
		}
	}

	int  cp_;
	bool hz_;
};




//-------------------------------------------------------------------------
// ISO-2022 サブセットその２。日本語EUC
//-------------------------------------------------------------------------

// Helper: SJIS ==> JIS X 0208
static void sjis2jis( uchar s1, uchar s2, char* k )
{
	if( ((s1==0xfa || s1==0xfb) && s2>=0x40) || (s1==0xfc && (s2&0xf0)==0x40) )
	{
		// IBM外字のマッピング
static const WORD IBM_sjis2kuten[400] = {
/*fa40*/0x5c51,0x5c52,0x5c53,0x5c54,0x5c55,0x5c56,0x5c57,0x5c58,0x5c59,0x5c5a,0x0d15,0x0d16,0x0d17,0x0d18,0x0d19,0x0d1a,
/*fa50*/0x0d1b,0x0d1c,0x0d1d,0x0d1e,0x022c,0x5c5c,0x5c5d,0x5c5e,0x0d4a,0x0d42,0x0d44,0x0248,0x5901,0x5902,0x5903,0x5904,
/*fa60*/0x5905,0x5906,0x5907,0x5908,0x5909,0x590a,0x590b,0x590c,0x590d,0x590e,0x590f,0x5910,0x5911,0x5912,0x5913,0x5914,
/*fa70*/0x5915,0x5916,0x5917,0x5918,0x5919,0x591a,0x591b,0x591c,0x591d,0x591e,0x591f,0x5920,0x5921,0x5922,0x5923,/**/0x0109,
/*fa80*/0x5924,0x5925,0x5926,0x5927,0x5928,0x5929,0x592a,0x592b,0x592c,0x592d,0x592e,0x592f,0x5930,0x5931,0x5932,0x5933,
/*fa90*/0x5934,0x5935,0x5936,0x5937,0x5938,0x5939,0x593a,0x593b,0x593c,0x593d,0x593e,0x593f,0x5940,0x5941,0x5942,0x5943,
/*faa0*/0x5944,0x5945,0x5946,0x5947,0x5948,0x5949,0x594a,0x594b,0x594c,0x594d,0x594e,0x594f,0x5950,0x5951,0x5952,0x5953,
/*fab0*/0x5954,0x5955,0x5956,0x5957,0x5958,0x5959,0x595a,0x595b,0x595c,0x595d,0x595e,0x5a01,0x5a02,0x5a03,0x5a04,0x5a05,
/*fac0*/0x5a06,0x5a07,0x5a08,0x5a09,0x5a0a,0x5a0b,0x5a0c,0x5a0d,0x5a0e,0x5a0f,0x5a10,0x5a11,0x5a12,0x5a13,0x5a14,0x5a15,
/*fad0*/0x5a16,0x5a17,0x5a18,0x5a19,0x5a1a,0x5a1b,0x5a1c,0x5a1d,0x5a1e,0x5a1f,0x5a20,0x5a21,0x5a22,0x5a23,0x5a24,0x5a25,
/*fae0*/0x5a26,0x5a27,0x5a28,0x5a29,0x5a2a,0x5a2b,0x5a2c,0x5a2d,0x5a2e,0x5a2f,0x5a30,0x5a31,0x5a32,0x5a33,0x5a34,0x5a35,
/*faf0*/0x5a36,0x5a37,0x5a38,0x5a39,0x5a3a,0x5a3b,0x5a3c,0x5a3d,0x5a3e,0x5a3f,0x5a40,0x5a41,0x5a42,/**/0x0109,0x0109,0x0109,
/*fb40*/0x5a43,0x5a44,0x5a45,0x5a46,0x5a47,0x5a48,0x5a49,0x5a4a,0x5a4b,0x5a4c,0x5a4d,0x5a4e,0x5a4f,0x5a50,0x5a51,0x5a52,
/*fb50*/0x5a53,0x5a54,0x5a55,0x5a56,0x5a57,0x5a58,0x5a59,0x5a5a,0x5a5b,0x5a5c,0x5a5d,0x5a5e,0x5b01,0x5b02,0x5b03,0x5b04,
/*fb60*/0x5b05,0x5b06,0x5b07,0x5b08,0x5b09,0x5b0a,0x5b0b,0x5b0c,0x5b0d,0x5b0e,0x5b0f,0x5b10,0x5b11,0x5b12,0x5b13,0x5b14,
/*fb70*/0x5b15,0x5b16,0x5b17,0x5b18,0x5b19,0x5b1a,0x5b1b,0x5b1c,0x5b1d,0x5b1e,0x5b1f,0x5b20,0x5b21,0x5b22,0x5b23,/**/0x0109,
/*fb80*/0x5b24,0x5b25,0x5b26,0x5b27,0x5b28,0x5b29,0x5b2a,0x5b2b,0x5b2c,0x5b2d,0x5b2e,0x5b2f,0x5b30,0x5b31,0x5b32,0x5b33,
/*fb90*/0x5b34,0x5b35,0x5b36,0x5b37,0x5b38,0x5b39,0x5b3a,0x5b3b,0x5b3c,0x5b3d,0x5b3e,0x5b3f,0x5b40,0x5b41,0x5b42,0x5b43,
/*fba0*/0x5b44,0x5b45,0x5b46,0x5b47,0x5b48,0x5b49,0x5b4a,0x5b4b,0x5b4c,0x5b4d,0x5b4e,0x5b4f,0x5b50,0x5b51,0x5b52,0x5b53,
/*fbb0*/0x5b54,0x5b55,0x5b56,0x5b57,0x5b58,0x5b59,0x5b5a,0x5b5b,0x5b5c,0x5b5d,0x5b5e,0x5c01,0x5c02,0x5c03,0x5c04,0x5c05,
/*fbc0*/0x5c06,0x5c07,0x5c08,0x5c09,0x5c0a,0x5c0b,0x5c0c,0x5c0d,0x5c0e,0x5c0f,0x5c10,0x5c11,0x5c12,0x5c13,0x5c14,0x5c15,
/*fbd0*/0x5c16,0x5c17,0x5c18,0x5c19,0x5c1a,0x5c1b,0x5c1c,0x5c1d,0x5c1e,0x5c1f,0x5c20,0x5c21,0x5c22,0x5c23,0x5c24,0x5c25,
/*fbe0*/0x5c26,0x5c27,0x5c28,0x5c29,0x5c2a,0x5c2b,0x5c2c,0x5c2d,0x5c2e,0x5c2f,0x5c30,0x5c31,0x5c32,0x5c33,0x5c34,0x5c35,
/*fbf0*/0x5c36,0x5c37,0x5c38,0x5c39,0x5c3a,0x5c3b,0x5c3c,0x5c3d,0x5c3e,0x5c3f,0x5c40,0x5c41,0x5c42,/**/0x0109,0x0109,0x0109,
/*fc40*/0x5c43,0x5c44,0x5c45,0x5c46,0x5c47,0x5c48,0x5c49,0x5c4a,0x5c4b,0x5c4c,0x5c4d,0x5c4e,/**/0x0109,0x0109,0x0109,0x0109,
};
		k[0] = IBM_sjis2kuten[ (s1-0xfa)*12*16 + (s2-0x40) ]>>8;
		k[1] = IBM_sjis2kuten[ (s1-0xfa)*12*16 + (s2-0x40) ]&0xff;
	}
	else
	{
		// その他
		if( s2>=0x9f )
		{
			if( s1>=0xe0 ) k[0] = ((s1-0xc0)<<1);
			else           k[0] = ((s1-0x80)<<1);
			k[1] = s2-0x9e;
		}
		else
		{
			if( s1>=0xe0 ) k[0] = ((s1-0xc0)<<1)-1;
			else           k[0] = ((s1-0x80)<<1)-1;

			if( s2 & 0x80 )	k[1] = s2-0x40;
			else			k[1] = s2-0x3f;
		}
	}

	k[0] += 0x20;
	k[1] += 0x20;
}

struct wEucJp A_FINAL: public TextFileWPimplWithBuf
{
	explicit wEucJp( FileW& w )
		: TextFileWPimplWithBuf(w) {}

	void WriteBuf( const unicode* str, size_t len ) override
	{
		// まず WideCharToMultiByte API を利用して変換
		int r = ::WideCharToMultiByte(932,0,str,len,buf_,bsiz_,NULL,NULL);

		for( int i=0; i<r; ++i )
		{
			fp_.NeedSpace(2);
			if( buf_[i] & 0x80 )
			{
				if( 0xA1<=(uchar)buf_[i] && (uchar)buf_[i]<=0xDF )
				{
					// カナ
					fp_.WriteCN( 0x8E );
					fp_.WriteCN( buf_[i] );
				}
				else
				{
					// JIS X 0208
					sjis2jis( buf_[i], buf_[i+1], buf_+i );
					fp_.WriteCN( buf_[i++] | 0x80 );
					fp_.WriteCN( buf_[i]   | 0x80 );
				}
			}
			else
			{
				// ASCII部分はそのまま出力
				fp_.WriteCN( buf_[i] );
			}
		}
	}
};



//-------------------------------------------------------------------------
// ISO-2022 サブセットその３。ISO-2022-JP
//-------------------------------------------------------------------------

struct wIsoJp A_FINAL: public TextFileWPimplWithBuf
{
	explicit wIsoJp( FileW& w )
		: TextFileWPimplWithBuf(w)
	{
		fp_.Write( "\x1b\x28\x42", 3 );
	}

	void WriteBuf( const unicode* str, size_t len ) override
	{
		// まず WideCharToMultiByte API を利用して変換
		int r = ::WideCharToMultiByte(932,0,str,len,buf_,bsiz_,NULL,NULL);

		enum { ROMA, KANJI, KANA } state = ROMA;
		for( int i=0; i<r; ++i )
			if( buf_[i] & 0x80 )
			{
				if( 0xA1<=(uchar)buf_[i] && (uchar)buf_[i]<=0xDF )
				{
					// カナ
					if( state != KANA )
						fp_.Write( "\x1b\x28\x49", 3 ), state = KANA;
					fp_.WriteC( buf_[i] & 0x7f );
				}
				else
				{
					// JIS X 0208
					if( state != KANJI )
						fp_.Write( "\x1b\x24\x42", 3 ), state = KANJI;
					sjis2jis( buf_[i], buf_[i+1], buf_+i );
					fp_.WriteC( buf_[i++] );
					fp_.WriteC( buf_[i]   );
				}
			}
			else
			{
				// ASCII部分はそのまま出力
				if( state != ROMA )
					fp_.Write( "\x1b\x28\x42", 3 ), state = ROMA;
				fp_.WriteC( buf_[i] );
			}

		if( state != ROMA )
			fp_.Write( "\x1b\x28\x42", 3 ), state = ROMA;
	}
};




//-------------------------------------------------------------------------
// 書き込みルーチンの準備等々
//-------------------------------------------------------------------------

TextFileW::TextFileW( int charset, int linebreak )
	: impl_ (NULL), cs_( charset ), lb_(linebreak)
{
}

TextFileW::~TextFileW()
{
	if( impl_ )
		delete impl_;
	Close();
}

void TextFileW::Close()
{
	fp_.Close();
}

void TextFileW::WriteLine( const unicode* buf, size_t siz, bool lastline )
{
	impl_->WriteLine( buf, siz );
	if( !lastline )
	{
		static const DWORD lbLst[] = {0x0D, 0x0A, 0x000A000D};
		static const DWORD lbLen[] = {   1,    1,          2};
		impl_->WriteLB(
			reinterpret_cast<const unicode*>(&lbLst[lb_]), lbLen[lb_] );
	}
}

bool TextFileW::Open( const TCHAR* fname )
{
	if( !fp_.Open( fname, true ) )
		return false;

	switch( cs_ )
	{
	case 28591: impl_ = new wWestISO88591( fp_ ); break; // ISO-8859-1
	case UTF1Y:
	case UTF1:    impl_ = new wUtf1( fp_, cs_==UTF1Y ); break;
	case UTF5Y:
	case UTF5:    impl_ = new wUtf5( fp_, cs_==UTF5Y ); break;
	case UTF9:
	case UTF9Y:    impl_ = new wUtf9( fp_, cs_==UTF9Y ); break;
	case UTF16l:
	case UTF16LE: impl_ = new wUtf16LE( fp_, cs_==UTF16l ); break;
	case UTF16b:
	case UTF16BE: impl_ = new wUtf16BE( fp_, cs_==UTF16b ); break;
	case UTF32l:
	case UTF32LE: impl_ = new wUtf32LE( fp_, cs_==UTF32l ); break;
	case UTF32b:
	case UTF32BE: impl_ = new wUtf32BE( fp_, cs_==UTF32b ); break;
	case SCSUY:
	case SCSU:    impl_ = new wSCSU( fp_, cs_==SCSUY ); break;
	case BOCU1Y:
	case BOCU1:   impl_ = new wBOCU1( fp_, cs_==BOCU1Y ); break;
	case OFSSUTF:
	case OFSSUTFY: impl_ = new wUtfOFSS( fp_, cs_==OFSSUTFY ); break;
	case UTFEBCDICY:
	case UTFEBCDIC:impl_ = new wUtfEBCDIC( fp_, cs_==UTFEBCDICY ); break;
	case EucJP:   impl_ = new wEucJp( fp_ ); break;
	case IsoJP:   impl_ = new wIsoJp( fp_ ); break;
	case IsoKR:   impl_ = new wIso2022( fp_, cs_ ); break;
	case IsoCN:   impl_ = new wIso2022( fp_, cs_ ); break;
	case HZ:      impl_ = new wIso2022( fp_, cs_ ); break;
	case UTF8:
	case UTF8N:
	default:
#ifndef _UNICODE
		if( (cs_==UTF8 || cs_==UTF8N) )
			impl_ = new wUTF8( fp_, cs_ );
		else if( cs_==UTF7 )
			impl_ = new wUTF7( fp_ );
		else
#else
		if( (cs_==UTF8 || cs_==UTF8N)
		&&  (!app().isNT() || !::IsValidCodePage(65001)) )
			impl_ = new wUTF8( fp_, cs_ );
		else if( cs_==UTF7
		&&    ( !app().isNT() || !::IsValidCodePage(65000)) )
			impl_ = new wUTF7( fp_ );
		else
#endif
		impl_ = new wMBCS( fp_, cs_ );
		break;
	}
	return impl_ != NULL;
}
