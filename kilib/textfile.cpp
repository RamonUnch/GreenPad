#include "stdafx.h"
#include "app.h"
#include "textfile.h"
#include "ktlarray.h"
using namespace ki;

#ifndef NO_CHARDET

#define CHARDET_RESULT_OK		    0
#define CHARDET_RESULT_NOMEMORY		    (-1)
#define CHARDET_RESULT_INVALID_DETECTOR	    (-2)

typedef void* chardet_t;

#endif //NO_CHARDET

//=========================================================================
// テキストファイル読み出し共通インターフェイス
//=========================================================================

struct ki::TextFileRPimpl : public Object
{
	inline TextFileRPimpl()
		: state(EOL) {}

	virtual size_t ReadLine( unicode* buf, ulong siz )
		= 0;

	enum { EOF=0, EOL=1, EOB=2 } state;
};



//-------------------------------------------------------------------------
// Unicode系用のベースクラス
//	UTF-8以外はそんなに出会わないだろうから遅くてもよしとする。
//-------------------------------------------------------------------------

struct rBasicUTF : public ki::TextFileRPimpl
{
	inline rBasicUTF() : BOF(true) {}
	virtual unicode PeekC() = 0;
	virtual unicode GetC() {unicode ch=PeekC(); Skip(); return ch;}
	virtual void Skip() = 0;
	virtual bool Eof() = 0;

	bool BOF;

	size_t ReadLine( unicode* buf, ulong siz )
	{
		state = EOF;

		// 改行が出るまで読む
		unicode *w=buf, *e=buf+siz;
		while( !Eof() )
		{
			*w = GetC();
			if(BOF && *w!=0xfeff) BOF = false;
			if( *w==L'\r' || *w==L'\n' )
			{
				state = EOL;
				break;
			}
			else if( !BOF && ++w==e )
			{
				state = EOB;
				break;
			}
			if(BOF) BOF = false;
		}

		// 改行コードスキップ処理
		if( state == EOL )
			if( *w==L'\r' && !Eof() && PeekC()==L'\n' )
				Skip();

		if(BOF) BOF = false;
		// 読んだ文字数
		return w-buf;
	}
};



//-------------------------------------------------------------------------
// UCS2ベタ/UCS4ベタ。それぞれUTF16, UTF32の代わりとして使う。
// ついでに同じtemplateで、ISO-8859-1も処理してしまう。^^;
//-------------------------------------------------------------------------

template<typename T>
struct rUCS : public rBasicUTF
{
	rUCS( const uchar* b, ulong s, bool bigendian )
		: fb( reinterpret_cast<const T*>(b) )
		, fe( reinterpret_cast<const T*>(b+(s/sizeof(T))*sizeof(T)) )
		, be( bigendian ) {}

	const T *fb, *fe;
	const bool be;

	// エンディアン変換
	inline  byte swap(  byte val ) { return val; }
	inline dbyte swap( dbyte val ) { return (val<<8) |(val>>8); }
	inline qbyte swap( qbyte val ) { return ((val>>24)&0xff |
	                                         (val>>8)&0xff00 |
											 (val<<8)&0xff0000|
											 (val<<24)); }

	virtual void Skip() { ++fb; }
	virtual bool Eof() { return fb==fe; }
	virtual unicode PeekC() { return (unicode)(be ? swap(*fb) : *fb); }
};

typedef rUCS< byte> rWest;
typedef rUCS<dbyte> rUtf16;

// UTF-32読み込み
struct rUtf32 : public rUCS<qbyte>
{
	rUtf32( const uchar* b, ulong s, bool bigendian )
		: rUCS<qbyte>(b,s,bigendian)
		, state(0) {}

	int state;
	qbyte curChar() { return be ? swap(*fb) : *fb; }
	bool inBMP(qbyte c) { return c<0x10000; }

	virtual unicode PeekC()
	{
		qbyte c = curChar();
		if( inBMP(c) )
			return (unicode)c;
		return (unicode)(state==0 ? 0xD800 + (((c-0x10000) >> 10)&0x3ff)
			                      : 0xDC00 + ( (c-0x10000)       &0x3ff));
	}

	virtual void Skip()
	{
		if( inBMP(curChar()) )
			++fb;
		else if( state==0 )
			state=1;
		else
			++fb, state=0;
	}
};


//-------------------------------------------------------------------------
// UTF-1
//-------------------------------------------------------------------------
struct rUtf1 : public rBasicUTF
{
	rUtf1( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, SurrogateLow( 0 ) {}

	const uchar *fb, *fe;
	qbyte SurrogateLow;

	inline byte conv( uchar x )
	{
		if( x<=0x20 )      return x + 0xBE;
		else if( x<=0x7E ) return x - 0x21;
		else if( x<=0x9F ) return x + 0x60;
		else               return x - 0x42;
	}

	bool Eof() { return SurrogateLow ? false : fb==fe; }
	void Skip()
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		if( *fb >= 0xFC && *fb <= 0xFF )      { fb+=5; }
		else if( *fb >= 0xF6 && *fb <= 0xFB ) { fb+=3; }
		else if( *fb >= 0xA0 && *fb <= 0xF5 ) { fb+=2; }
		else /*if( *fb <= 0x9F )*/            {  ++fb; }
	}
	unicode PeekC()
	{
		qbyte ch;

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}

		if( *fb <= 0x9F )                         { ch = (*fb); }
		else if( *fb == 0xA0 )                    { ch = (*(fb+1)); }
		else if( *fb >= 0xA1 && *fb <= 0xF5 )     { ch = ((*fb-0xA1) * 0xBE + conv(*(fb+1)) + 0x100); }
		else if( *fb >= 0xF6 && *fb <= 0xFB )     { ch = ((*fb-0xF6) * 0x8D04 + conv(*(fb+1)) * 0xBE + conv(*(fb+2)) + 0x4016); }
		else /*if( *fb >= 0xFC && *fb <= 0xFF )*/ { ch = ((*fb-0xFC) * 0x4DAD6810 + conv(*(fb+1)) * 0x68A8F8 + conv(*(fb+2)) * 0x8D04 + conv(*(fb+3)) * 0xBE + conv(*(fb+4)) + 0x38E2E); }

		if( ch > 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}
};

//-------------------------------------------------------------------------
// UTF-9 (draft-abela-utf9-00)
//-------------------------------------------------------------------------
struct rUtf9 : public rBasicUTF
{
	rUtf9( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, SurrogateLow( 0 ) {}

	const uchar *fb, *fe;
	qbyte SurrogateLow;

	bool Eof() { return SurrogateLow ? false : fb==fe; }
	void Skip()
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		if( *fb >= 0x98 && *fb <= 0x9F )      { fb+=5; }
		else if( *fb >= 0x94 && *fb <= 0x97 ) { fb+=4; }
		else if( *fb >= 0x90 && *fb <= 0x93 ) { fb+=3; }
		else if( *fb >= 0x80 && *fb <= 0x8F ) { fb+=2; }
		else /* 0~0x7F,0xA0~0xFF */           {  ++fb; }
	}
	unicode PeekC()
	{
		qbyte ch;

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}

		if( *fb >= 0x98 && *fb <= 0x9F )      { ch = (((*fb & 0x07) << 28) + ((*(fb+1) & 0x7F) << 21) + ((*(fb+2) & 0x7F) << 14) + ((*(fb+3) & 0x7F) << 7) + (*(fb+4) & 0x7F)); }
		else if( *fb >= 0x94 && *fb <= 0x97 ) { ch = (((*fb & 0x03) << 21) + ((*(fb+1) & 0x7F) << 14) + ((*(fb+2) & 0x7F) << 7) + (*(fb+3) & 0x7F)); }
		else if( *fb >= 0x90 && *fb <= 0x93 ) { ch = (((*fb & 0x03) << 14) + ((*(fb+1) & 0x7F) << 7) + (*(fb+2) & 0x7F)); }
		else if( *fb >= 0x80 && *fb <= 0x8F ) { ch = (((*fb & 0x7F) << 7) + (*(fb+1) & 0x7F)); }
		else /* 0~0x7F,0xA0~0xFF */           { ch = (*fb); }

		if( ch > 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}
};

//-------------------------------------------------------------------------
// Old FSS-UTF (/usr/ken/utf/xutf from dump of Sep 2 1992)
//-------------------------------------------------------------------------
struct rUtfOFSS : public rBasicUTF
{
	rUtfOFSS( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, SurrogateLow( 0 ) {}

	const uchar *fb, *fe;
	qbyte SurrogateLow;

	bool Eof() { return SurrogateLow ? false : fb==fe; }
	void Skip()
	{
		if( SurrogateLow ) return; // don't go further if leftover exists

		if( *fb >= 0xf0 && *fb <= 0xf8 )      { fb+=5; }
		else if( *fb >= 0xe0 && *fb <= 0xf0 ) { fb+=4; }
		else if( *fb >= 0xc0 && *fb <= 0xe0 ) { fb+=3; }
		else if( *fb >= 0x80 && *fb <= 0xc0 ) { fb+=2; }
		else /* 0~0x7F,0xA0~0xFF */           {  ++fb; }
	}
	unicode PeekC()
	{
		qbyte ch;

		if( SurrogateLow )
		{
			ch = SurrogateLow;
			SurrogateLow = 0;
			return (unicode)ch;
		}

		if( *fb >= 0xf0 && *fb <= 0xf8 )      { ch = (((*fb & 0x07) << 28) + ((*(fb+1) & 0x7F) << 21) + ((*(fb+2) & 0x7F) << 14) + ((*(fb+3) & 0x7F) << 7) + (*(fb+4) & 0x7F) + 0x2082080); }
		else if( *fb >= 0xe0 && *fb <= 0xf0 ) { ch = (((*fb & 0x0f) << 21) + ((*(fb+1) & 0x7F) << 14) + ((*(fb+2) & 0x7F) << 7) + (*(fb+3) & 0x7F) + 0x0082080); }
		else if( *fb >= 0xc0 && *fb <= 0xe0 ) { ch = (((*fb & 0x1f) << 14) + ((*(fb+1) & 0x7F) << 7) + (*(fb+2) & 0x7F) + 0x0002080); }
		else if( *fb >= 0x80 && *fb <= 0xc0 ) { ch = (((*fb & 0x3f) << 7) + (*(fb+1) & 0x7F) + 0x0000080); }
		else /* 0~0x7F,0xA0~0xFF */           { ch = (*fb); }

		if( ch > 0x10000 )
		{
			SurrogateLow = (0xDC00 + (((ch-0x10000)    )&0x3ff));
			ch = (0xD800 + (((ch-0x10000)>>10)&0x3ff));
		}
		return (unicode)ch;
	}
};

//-------------------------------------------------------------------------
// UTF-5
//     0-  F : 1bbbb
//    10- FF : 1bbbb 0bbbb
//   100-FFF : 1bbbb 0bbbb 0bbbb
// というように、16進での一桁を一文字で表していくフォーマット。
// 各 0bbbb は '0', '1', ... '9', 'A', ... 'F'
// 各 1bbbb は 'G', 'H', ... 'P', 'Q', ... 'V' の字で表現。
//-------------------------------------------------------------------------

struct rUtf5 : public rBasicUTF
{
	rUtf5( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s ) {}

	const uchar *fb, *fe;

	// 16進文字から整数値へ変換
	inline byte conv( uchar x )
	{
		if( '0'<=x && x<='9' ) return x-'0';
		else                   return x-'A'+0x0A;
	}

	void Skip() { do ++fb; while( fb<fe && *fb<'G' ); }
	bool Eof() { return fb==fe; }
	unicode PeekC()
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

struct rUtf7 : public rBasicUTF
{
	rUtf7( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, rest( -1 ) { fillbuf(); }

	const uchar *fb, *fe;
	unicode buf[3]; // b64を８文字毎に読んでバッファに溜めておく
	int rest;       // バッファの空き
	bool inB64;     // base64エリア内ならtrue

	void Skip() { if(--rest==0) fillbuf(); }
	bool Eof() { return fb==fe && rest==0; }
	unicode PeekC() { return buf[rest-1]; }

	void fillbuf()
	{
		if( fb<fe )
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
};

//-------------------------------------------------------------------------
// SCSU (UTR #6)
// Code portion is taken from:
// http://czyborra.com/scsu/scsu.c written by Roman Czyborra@dds.nl
//-------------------------------------------------------------------------
namespace 
{
	static const int SCSU_win[256]={
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
	0x3000, 0x3080, 0x3100, 0x3180, 0x3200, 0x3280, 0x3300, 0x3800,
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
struct rSCSU : public rBasicUTF
{
	rSCSU( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, active( 0 )
		, mode( 0 )
		, skip( 0 )
		, c( 0 )
		, d( 0 ) {}

	const uchar *fb, *fe;
	char active, mode;
	ulong skip;
	uchar c, d;

	void Skip() { fb+=skip; skip=0; }
	bool Eof() { return fb==fe; }
	uchar GetChar() { return fb+skip>fe ? 0 : *(fb+(skip++)); }
	unicode PeekC()
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
			else if (c >= 0xE0 && c <= 0xE7) /* UCn */
			{
				active = c - 0xE0; mode = 0;

				Skip();
				return PeekC();
			}
			else if (c >= 0xE8 && c <= 0xEF) /* UDn */
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
namespace {
	static const uchar bocu1_byte_to_trail[256] = {
//   0x00 - 0x20
    -1,   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, -1,   -1,   0x10, 0x11, 0x12, 0x13,
    -1,
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
struct rBOCU1 : public rBasicUTF
{
	rBOCU1( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, skip( 0 )
		, pc( 0x40 )
		, cp( 0 ) {}

	const uchar *fb, *fe;
	ulong skip;
	unicode cp, pc;

	void Skip() { fb+=skip; skip=0; }
	bool Eof() { return fb==fe; }
	uchar GetChar() { return (fb+skip < fe) ? *(fb+(skip++)) : 0; }
	unicode PeekC()
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
// UTF8/MBCS
//  CR,LFが１バイト文字としてきちんと出てくるので、
//  切り分けが簡単な形式をここでまとめて扱う。UTF8以外の変換は
//  Windowsに全て任せている。
//-------------------------------------------------------------------------

namespace 
{
	typedef char* (WINAPI * uNextFunc)(WORD,const char*,DWORD);

	static const byte mask[] = { 0, 0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
	static inline int GetMaskIndex(uchar n)
	{
		if( uchar(n+2) < 0xc2 ) return 1; // 00～10111111, fe, ff
		if( n          < 0xe0 ) return 2; // 110xxxxx
		if( n          < 0xf0 ) return 3; // 1110xxxx
		if( n          < 0xf8 ) return 4; // 11110xxx
		if( n          < 0xfc ) return 5; // 111110xx
		                        return 6; // 1111110x
	}

	static char* WINAPI CharNextUtf8( WORD, const char* p, DWORD )
	{
		return const_cast<char*>( p+GetMaskIndex(uchar(*p)) );
	}

	// CharNextExAはGB18030の４バイト文字列を扱えないそうだ。
	static char* WINAPI CharNextGB18030( WORD, const char* p, DWORD )
	{
		const char *q;
		if (!(*p & 0x80) || *p == 0x80 || *p == 0xFF || // ASCII, Euro sign, EOF
			!p[1] || p[1] == 0xFF || (unsigned char)p[1] < 0x30) // invalid DBCS
			q = p+1;
		else if (p[1] >= 0x30 && p[1] <= 0x39) // 4BCS leading
		{
			if (p[2] && p[3] && (p[2] & 0x80) && p[2] != 0x80 && p[2] != 0xFF
				&& p[3] >= 0x30 && p[3] <= 0x39 &&
				(((unsigned char)*p >= 0x81 && (unsigned char)*p <= 0x84) ||
				 ((unsigned char)*p >= 0x90 && (unsigned char)*p <= 0xE3)))
				q = p+4;
			else
				q = p+1;
		}
		else // DBCS
			q = p+2;
		return const_cast<char*>( q );
	}

	// IMultiLanguage2::DetectInputCodepageはGB18030のことを認識できません。
	static bool IsGB18030Like( const uchar* ptr, ulong siz, int refcs )
	{
		ulong i;
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
						if (ptr[i+2] > 0x80 && ptr[i+2] < 0xFF &&
							ptr[i+3] >= 0x30 && ptr[i+3] <= 0x39 &&
							((ptr[i] >= 0x81 && ptr[i] <= 0x84) ||
							 (ptr[i] >= 0x90 && ptr[i] <= 0xE3)))
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

	// Win95対策。
	//   http://support.microsoft.com/default.aspx?scid=%2Fisapi%2Fgomscom%2Easp%3Ftarget%3D%2Fjapan%2Fsupport%2Fkb%2Farticles%2Fjp175%2F3%2F92%2Easp&LN=JA
	// MSDNにはWin95以降でサポートと書いてあるのにCP_UTF8は
	// 使えないらしいので、自前の変換関数で。
	typedef int (WINAPI * uConvFunc)(UINT,DWORD,const char*,int,wchar_t*,int);
	static int WINAPI Utf8ToWideChar( UINT, DWORD, const char* sb, int ss, wchar_t* wb, int ws )
	{
		const uchar *p = reinterpret_cast<const uchar*>(sb);
		const uchar *e = reinterpret_cast<const uchar*>(sb+ss);
		wchar_t     *w = wb; // バッファサイズチェック無し（仕様）

		for( int t; p<e; ++w )
		{
			t  = GetMaskIndex(*p);
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

struct rMBCS : public TextFileRPimpl
{
	// ファイルポインタ＆コードページ
	const char* fb;
	const char* fe;
	const int   cp;
	uNextFunc next;
	uConvFunc conv;

	// 初期設定
	rMBCS( const uchar* b, ulong s, int c )
		: fb( reinterpret_cast<const char*>(b) )
		, fe( reinterpret_cast<const char*>(b+s) )
		, cp( c==UTF8 ? UTF8N : c )
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
		, next( cp==UTF8N ?   CharNextUtf8 : cp==GB18030 ? CharNextGB18030 :
							CharNextExA )
#endif
		, conv( cp==UTF8N && (app().isWin95()||!::IsValidCodePage(65001))
		                  ? Utf8ToWideChar : MultiByteToWideChar )
	{
		if( cp==UTF8N && fe-fb>=3
		 && b[0]==0xef && b[1]==0xbb && b[2]==0xbf )
			fb += 3; // BOMスキップ
	}

	size_t ReadLine( unicode* buf, ulong siz )
	{
		// バッファの終端か、ファイルの終端の近い方まで読み込む
		const char *p, *end = Min( fb+siz/2, fe );
		state = (end==fe ? EOF : EOB);

		// 改行が出るまで進む
		for( p=fb; p<end; )
			if( *p=='\r' || *p=='\n' )
			{
				state = EOL;
				break;
			}
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
			else if( (*p) & 0x80 && p+1<fe )
			{
				p = next(cp,p,0);
			}
#endif
			else
			{
				++p;
			}

		// Unicodeへ変換
		ulong len;
#ifndef _UNICODE
		len = conv( cp, 0, fb, p-fb, buf, siz );
#else
		if(!app().isNewShell())
		{
			len = conv( cp, 0, fb, p-fb, buf, siz );
		}
		else
		{
			len = ::MultiByteToWideChar( cp, 0, fb, int(p-fb), buf, siz );
		}
#endif
		// 改行コードスキップ処理
		if( state == EOL )
			if( *(p++)=='\r' && p<fe && *p=='\n' )
				++p;
		fb = p;

		// 終了
		return len;
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
//		GL (0x21～0xfe) GR (0xa0～0xff)
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

struct rIso2022 : public TextFileRPimpl
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
	bool      fixed; // ESCによる切り替えを行わないならtrue
	bool    mode_hz; // HZの場合。

	// 作業変数
	CodeSet *GL, *GR, G[4];
	int gWhat; // 次の字は 1:GL/GR 2:G2 3:G3 で出力
	ulong len;

	// 初期化
	rIso2022( const uchar* b, ulong s, bool f, bool hz,
		CodeSet g0, CodeSet g1, CodeSet g2=ASCII, CodeSet g3=ASCII )
		: fb( b )
		, fe( b+s )
		, fixed( f )
		, mode_hz( hz )
		, GL( &G[0] )
		, GR( &G[1] )
		, gWhat( 1 )
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
				G[ (p[0]-0x28)%4 ] = ASCII;         // 1B [28-2B] 4A
			else if( p[1]==0x49 )
				G[ (p[0]-0x28)%4 ] = KANA;          // 1B [28-2B] 49
			else if( *reinterpret_cast<const dbyte*>(p)==0x4228 )
				G[ 0 ] = ASCII;                     // 1B 28 42
			else if( *reinterpret_cast<const dbyte*>(p)==0x412E )
				G[ 2 ] = LATIN;                     // 1B 2E 41
			else if( p[0]==0x24 )
				if( p[1]==0x40 || p[1]==0x42 )
					G[ 0 ] = JIS;                   // 1B 24 [40|42]
				else if( p[1]==0x41 )
					G[ 0 ] = GB;                    // 1B 24 41
				else if( p+2 < fe )
					if( p[2]==0x41 )
						G[ ((*++p)-0x28)%4 ] = GB;  // 1B 24 [28-2B] 41
					else if( p[2]==0x43 )
						G[ ((*++p)-0x28)%4 ] = KSX; // 1B 24 [28-2B] 43
		}
		++p;
	}

	void DoOutput( unicode*& buf, const uchar*& p )
	{
		// 文字集合取り出し
		CodeSet cs =
			(gWhat==2 ? G[2] : 
			(gWhat==3 ? G[3] :
			(*p&0x80  ? *GR  : *GL)));

		char c[2];
		ulong wt=1;
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

	size_t ReadLine( unicode* buf, ulong siz )
	{
		len=0;

		// バッファの終端か、ファイルの終端の近い方まで読み込む
		const uchar *p, *end = Min( fb+siz/2, fe );
		state = (end==fe ? EOF : EOB);

		// 改行が出るまで進む
		for( p=fb; p<end; ++p )
			switch( *p )
			{
			case '\r':
			case '\n': state =   EOL; goto outofloop;
			case 0x0F:    GL = &G[0]; break;
			case 0x0E:    GL = &G[1]; break;
			case 0x8E: gWhat =     2; break;
			case 0x8F: gWhat =     3; break;
			case 0x1B:
				if( p+1<fe ) {
					++p; if( *p==0x7E )    GR = &G[1];
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

		// 改行コードスキップ処理
		if( state == EOL )
			if( *(p++)=='\r' && p<fe && *p=='\n' )
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
	: cs_( charset )
	, nolbFound_(true)
{
}

TextFileR::~TextFileR()
{
	Close();
}

size_t TextFileR::ReadLine( unicode* buf, ulong siz )
{
	return impl_->ReadLine( buf, siz );
}

int TextFileR::state() const
{
	return impl_->state;
}

void TextFileR::Close()
{
	fp_.Close();
}

bool TextFileR::Open( const TCHAR* fname )
{
	// ファイルを開く
	if( !fp_.Open(fname) )
		return false;
	const uchar* buf = fp_.base();
	const ulong  siz = fp_.size();

	// 必要なら自動判定
	cs_ = AutoDetection( cs_, buf, Min<ulong>(siz,16<<10) ); // 先頭16KB

	// 対応するデコーダを作成
	switch( cs_ )
	{
	case Western: impl_ = new rWest(buf,siz,true); break;
	case UTF16b:
	case UTF16BE: impl_ = new rUtf16(buf,siz,true); break;
	case UTF16l:
	case UTF16LE: impl_ = new rUtf16(buf,siz,false); break;
	case UTF32b:
	case UTF32BE: impl_ = new rUtf32(buf,siz,true); break;
	case UTF32l:
	case UTF32LE: impl_ = new rUtf32(buf,siz,false); break;
	case UTF1Y:
	case UTF1:    impl_ = new rUtf1(buf,siz); break;
	case UTF5:    impl_ = new rUtf5(buf,siz); break;
	case UTF7:    impl_ = new rUtf7(buf,siz); break;
	case UTF9Y:
	case UTF9:    impl_ = new rUtf9(buf,siz); break;
	case SCSU:    impl_ = new rSCSU(buf,siz); break;
	case BOCU1:   impl_ = new rBOCU1(buf,siz); break;
	case OFSSUTFY:
	case OFSSUTF: impl_ = new rUtfOFSS(buf,siz); break;
	case EucJP:   impl_ = new rIso2022(buf,siz,true,false,ASCII,JIS,KANA); break;
	case IsoJP:   impl_ = new rIso2022(buf,siz,false,false,ASCII,KANA); break;
	case IsoKR:   impl_ = new rIso2022(buf,siz,true,false,ASCII,KSX); break;
	case IsoCN:   impl_ = new rIso2022(buf,siz,true,false,ASCII,GB); break;
	case HZ:      impl_ = new rIso2022(buf,siz,true,true, ASCII,GB); break;
	default:      impl_ = new rMBCS(buf,siz,cs_); break;
	}

	return true;
}

int TextFileR::AutoDetection( int cs, const uchar* ptr, ulong siz )
{
//-- まず、文字の出現回数の統計を取る

	int  freq[256];
	bool bit8 = false;
	mem00( freq, sizeof(freq) );
	for( ulong i=0; i<siz; ++i )
	{
		if( ptr[i] >= 0x80 )
			bit8 = true;
		++freq[ ptr[i] ];
	}

//-- 改行コード決定 (UTF16/32/7のとき問題あり。UTF5に至っては判定不可…)

	     if( freq['\r'] > freq['\n']*2 ) lb_ = CR;
	else if( freq['\n'] > freq['\r']*2 ) lb_ = LF;
	else                                 lb_ = CRLF;
	nolbFound_ = freq['\r']==0 && freq['\n']==0;

//-- デフォルトコード

	int defCs = ::GetACP();

//-- 小さすぎる場合はここで終了

	if( siz < 4 )
		return cs==AutoDetect ? defCs : cs;

//-- 明示指定がある場合はここで終了

	ulong bom4 = (ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | (ptr[3]);
	ulong bom2 = (ptr[0]<<8)  | (ptr[1]);

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

	if( cs != AutoDetect )
		return cs;

//-- BOMチェック・7bitチェック

	bool Jp = ::IsValidCodePage(932)!=FALSE;

	if( (bom4>>8) == 0xefbbbf )      cs = UTF8;
	else if( (bom4>>8) == 0xf7644c ) cs = UTF1Y;
	else if( (bom4>>8) == 0x93fdff ) cs = UTF9Y;
	else if( (bom4>>8) == 0x0efeff ) cs = SCSU;
	else if( (bom4>>8) == 0xfbee28 ) cs = BOCU1;
	else if( (bom4>>8) == 0xc3bcff ) cs = OFSSUTFY;
	else if( bom4 == 0x0000feff ) cs = UTF32b;
	else if( bom4 == 0xfffe0000 ) cs = UTF32l;
	else if( bom2 == 0xfeff )     cs = UTF16b;
	else if( bom2 == 0xfffe )     cs = UTF16l;
	else if( bom4 == 0x1b242943 && ::IsValidCodePage(949) ) cs = IsoKR;
	else if( bom4 == 0x1b242941 && ::IsValidCodePage(936) ) cs = IsoCN;
	else if( Jp && !bit8 && freq[0x1b]>0 )                  cs = IsoJP;

	if( cs != AutoDetect )
		return cs;

//-- UTF-5 チェック

	ulong u5sum = 0;
	for( uchar c='0'; c<='9'; ++c ) u5sum += freq[c];
	for( uchar c='A'; c<='V'; ++c ) u5sum += freq[c];
	if( siz == u5sum )
		return UTF5;

//-- UTF-16/32 detection
	if( freq[ 0 ] ) // nulls in content?
	{ // then it may be UTF-16/32 without BOM
		if(CheckUTFConfidence(ptr,siz,sizeof(dbyte),true)) return UTF16LE;
		if(CheckUTFConfidence(ptr,siz,sizeof(dbyte),false)) return UTF16BE;
		if(CheckUTFConfidence(ptr,siz,sizeof(qbyte),true)) return UTF32LE;
		if(CheckUTFConfidence(ptr,siz,sizeof(qbyte),false)) return UTF32BE;
	}

//-- chardet and MLang detection
	if( app().isNewShell() )
	{ // chardet works better when size > 64
		if( siz > 80 )
		{
			cs = chardetAutoDetection( ptr, siz );
			if( cs ) return cs;
		}
		cs = MLangAutoDetection( ptr, siz );
		if( cs ) return cs;
	}
	else
	{ // chardet is the only auto detection method
		cs = chardetAutoDetection( ptr, siz );
		if( cs ) return cs;
	}

// last resort
//-- 暫定版 UTF-8 / 日本語EUC チェック

	// 改行コードがLFか、ある程度の大きさか、でないと
	// 無条件で ANSI-CP と見なしてしまう。
	if( bit8 && (siz>4096 || lb_==1
	 || freq[0xfd]>0 || freq[0xfe]>0 || freq[0xff]>0 || freq[0x80]>0) )
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
		{
			// UTF8として読めるかどうかチェック
			bool b8=true;
			int mi=1;
			for( ulong i=0; i<siz && b8; ++i )
				if( --mi )
				{
					if( ptr[i]<0x80 || ptr[i]>=0xc0 )
						b8 = false;
				}
				else
				{
					mi = 1;
					if( ptr[i] > 0x7f )
					{
						mi = GetMaskIndex( ptr[i] );
						if( mi == 1 )//ptr[i] >= 0xfe )
							b8 = false;
					}
				}
			if( b8 )
				return UTF8N;
		}
	}


//-- 判定結果

	return cs ? cs : defCs;
}

int TextFileR::MLangAutoDetection( const uchar* ptr, ulong siz )
{
	int cs = 0;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>310)
#ifndef NO_MLANG
	app().InitModule( App::OLE );
	IMultiLanguage2 *lang = NULL;
	if( S_OK == ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_ALL, IID_IMultiLanguage2, (LPVOID*)&lang ) )
	{
		int detectEncCount = 5;
		DetectEncodingInfo detectEnc[5];
		lang->DetectInputCodepage(MLDETECTCP_DBCS, 0, (char *)(ptr), (INT *)(&siz), detectEnc, &detectEncCount); // 2 ugly C-cast here

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
		TCHAR tmp[10];
		::wsprintf(tmp,TEXT("%d"),cs);
		::MessageBox(NULL,tmp,TEXT("MLangDetect"),0);
# endif

		if (cs == 20127) cs = 0; // 20127 == ASCII, 0 = unknown
		if (cs == 65000) cs = 0; // 65000 == UTF-7, let's disable misdetecting as UTF-7

		if (lang)
			lang->Release();
	}
#endif //NO_MLANG
#endif //TARGET_VER
	return cs;
}

int TextFileR::chardetAutoDetection( const uchar* ptr, ulong siz )
{
	int cs = 0;
#ifndef NO_CHARDET
	// function calls
	int (__cdecl*chardet_create)(chardet_t*) = 0;
	void (__cdecl*chardet_destroy)(chardet_t) = 0;
	int (__cdecl*chardet_handle_data)(chardet_t, const char*, unsigned int) = 0;
	int (__cdecl*chardet_data_end)(chardet_t) = 0;
	int (__cdecl*chardet_get_charset)(chardet_t, char*, unsigned int) = 0;
	//int (__cdecl*chardet_reset)(chardet_t) = 0;
	HINSTANCE hIL;

	chardet_t pdet = NULL;
	char charset[128];

# define STR2CP(a,b) if(0 == ::lstrcmpA(charset,a)) { \
					chardet_destroy(pdet); \
					::FreeLibrary(hIL); \
					return b; \
				}


	if(hIL = ::LoadLibrary(TEXT("chardet.dll")))
	{
		chardet_create = (int(__cdecl*)(chardet_t*))::GetProcAddress(hIL, "chardet_create");
		chardet_destroy = (void(__cdecl*)(chardet_t))::GetProcAddress(hIL, "chardet_destroy");
		chardet_handle_data = (int(__cdecl*)(chardet_t, const char*, unsigned int))::GetProcAddress(hIL, "chardet_handle_data");
		chardet_data_end = (int(__cdecl*)(chardet_t))::GetProcAddress(hIL, "chardet_data_end");
		chardet_get_charset = (int(__cdecl*)(chardet_t, char*, unsigned int))::GetProcAddress(hIL, "chardet_get_charset");
		//chardet_reset = (int(__cdecl*)(chardet_t))::GetProcAddress(hIL, "chardet_reset");

	    if( (chardet_create && chardet_destroy && chardet_handle_data && chardet_data_end && chardet_get_charset) && 0 == chardet_create(&pdet) )
	    {
			if(siz == 16384) siz-=1; // prevert off-by-one error
			if(0 == chardet_handle_data(pdet, (const char *)ptr, siz))
			{
				chardet_data_end(pdet);
				chardet_get_charset(pdet, charset, 128);

				STR2CP("Shift_JIS",SJIS)
				STR2CP("EUC-JP",EucJP)
				STR2CP("EUC-KR",UHC)
				STR2CP("x-euc-tw",CNS)
				STR2CP("Big5",Big5)
				STR2CP("gb18030",(::IsValidCodePage(GB18030) ? GB18030 : GBK))
				STR2CP("UTF-8",UTF8N)
				STR2CP("windows-1253",Greek)
				STR2CP("KOI8-R",Koi8R)
				STR2CP("windows-1251",Cyrillic)
				STR2CP("IBM866",CyrillicDOS)
				STR2CP("IBM855",CyrillicIBM)
				STR2CP("windows-1255",Hebrew)
				STR2CP("windows-1250",Central)
				STR2CP("TIS-620",Thai)

			}
		}

	}
# undef STR2CP
#endif //NO_CHARDET
	return cs;
}

// functions for detecting BOM-less UTF-16/32
bool TextFileR::IsNonUnicodeRange(qbyte u)
{ // Unicode 6.3 based
			return	(0x000860 <= u && u < 0x0008A0) ||	// U+0840-085F,08A0-08FF : Mandaic, Arabic Extended-A
					//(0x0018B0 <= u && u < 0x001900) ||	// U+18B0-18FF : Unified Canadian Aboriginal Syllabics Extended
					(0x001AB0 <= u && u < 0x001B00) ||
					//(0x001BC0 <= u && u < 0x001C00) ||	// U+1BC0-1BFF : Batak
					(0x001C80 <= u && u < 0x001CC0) ||	// U+1CC0-1CCF : Sundanese Supplement
					//(0x002C60 <= u && u < 0x002C80) ||	// U+2C60-2C7F : Latin Extended-C
					//(0x002DE0 <= u && u < 0x002E00) ||	// U+2DE0-2DFF : Cyrillic Extended-A
					(0x002FE0 <= u && u < 0x002FF0) ||
					//(0x00A4D0 <= u && u < 0x00A700) ||	// U+A4D0-A4FF,A500-A63F,A640-A69F,A6A0-A6FF : Lisu, Vai, Cyrillic Extended-B, Bamum
					//(0x00A720 <= u && u < 0x00A800) ||	// U+A720-A7FF : Latin Extended-D
					(0x00A9E0 <= u && u < 0x00AA00) ||	//
					//(0x00AAE0 <= u && u < 0x00AB00) ||	// U+AB00-AB2F,AAE0-AAFF : Ethiopic Extended-A, Meetei Mayek Extensions
					(0x00AB00 <= u && u < 0x00ABC0) ||
					//(0x00D7B0 <= u && u < 0x00D800) ||	// U+D7B0-D7FF : Hangul Jamo Extended-B
					(0x010200 <= u && u < 0x010280) ||	// U+10280-1029F,102A0-102DF : Lycian, Carian
					(0x0102E0 <= u && u < 0x010300) ||	// U+10300-1032F,10330-1034F : Old Italic, Gothic
					(0x010350 <= u && u < 0x010380) ||	// U+10380-1039F,103A0-103DF : Ugaritic, Old Persian
					(0x0103E0 <= u && u < 0x010400) ||	// U+10400-1044F,10450-1047F,10480-104AF : Deseret, Shavian, Osmanya
					(0x0104B0 <= u && u < 0x010800) ||	// U+10800-1083F,10840-1085F : Cypriot Syllabary, Imperial Aramaic
					(0x010860 <= u && u < 0x010900) ||	// U+10900-1091F,10920-1093F : Phoenician, Lydian
					(0x010940 <= u && u < 0x010980) ||	// U+10A00-10A5F,10A60-10A7F,10980-1099F,109A0-109FF : Kharoshthi, Old South Arabian, Meroitic Hieroglyphs, Meroitic Cursive
					(0x010A80 <= u && u < 0x010B00) ||	// U+10B00-10B3F,10B40-10B5F,10B60-10B7F : Avestan, Inscriptional Parthian, Inscriptional Pahlavi
					(0x010B80 <= u && u < 0x010C00) ||	// U+10C00-10C4F : Old Turkic
					(0x010C50 <= u && u < 0x010E60) ||	// U+10E60-10E7F : Rumi Numeral Symbols
					(0x010E80 <= u && u < 0x011000) ||	// U+11000-1107F,11080-110CF,110D0-110FF,11100-1114F : Brahmi, Kaithi, Sora Sompeng, Chakma
					(0x011150 <= u && u < 0x011180) ||	// U+11180-111DF : Sharada
					(0x0111E0 <= u && u < 0x011680) ||	// U+11680-116CF : Takri
					(0x0116D0 <= u && u < 0x012000) ||	// U+12000-123FF,12400-1247F : Cuneiform, Cuneiform Numbers and Punctuation
					(0x012480 <= u && u < 0x013000) ||	// U+13000-1342F : Egyption Ideographs
					(0x013430 <= u && u < 0x016800) ||	// U+16800-16A3F : Bamum Supplement
					(0x016A40 <= u && u < 0x016F00) ||	// U+16F00-16F9F : Miao
					(0x016FA0 <= u && u < 0x01B000) ||	// U+1B000-1B0FF : Kana Supplement
					(0x01B100 <= u && u < 0x01D000) ||	// U+1D000-1D0FF,1D100-1D1FF,1D200-1D24F : Byzantine Musical Symbols, Musical Symbols, Ancient Greek Musical Notation
					(0x01D250 <= u && u < 0x01D300) ||	// U+1D300-1D35F,1D360-1D37F : Tai Xuan Jing Symbols, Counting Rod Numerals
					(0x01D380 <= u && u < 0x01D400) ||	// U+1D400-1D7FF : Mathematical Alphanumeric Symbols
					(0x01D800 <= u && u < 0x01EE00) ||	// U+1EE00-1EEFF : Arabic Mathematical Alphabetic Symbols
					(0x01EF00 <= u && u < 0x01F000) ||	// U+1F000-1F02F,1F030-1F09F : Mahjong Tiles, Domino Tiles
					//(0x01F0A0 <= u && u < 0x01F100) ||	// U+1F0A0-1F0FF,1F100-1F1FF,1F200-1F2FF,1F300-1F5FF,1F600-1F64F : Playing Cards, Enclosed Alphanumeric Supplement, Enclosed Ideographic Supplement, Miscellaneous Symbols And Pictographs, Emoticons
					(0x01F650 <= u && u < 0x01F680) ||	// U+1F680-1F6FF,1F700-1F77F  : Transport And Map Symbols, Alchemical Symbols
					(0x01F780 <= u && u < 0x020000) ||	// U+20000-2A6DF : CJK Unified Ideographs Extension B
					(0x02A6E0 <= u && u < 0x02A700) ||	// U+2A700-2B73F,2B740-2B81F : CJK Unified Ideographs Extension C, CJK Unified Ideographs Extension D
					(0x02B820 <= u && u < 0x02F800) ||	// U+2F800-2FA1F : CJK Compatibility Ideographs Supplement
					(0x02FA20 <= u && u < 0x0E0000) ||	// U+E0000-E007F : Tags
					(0x0E0080 <= u && u < 0x0E0100) ||	// U+E0100-E01EF : Variation Selectors Supplement
					(0x0E01F0 <= u && u < 0x0F0000) ||	// U+F0000-FFFFD : Supplementary Private Use Area A
					(0x110000 <= u); // U+100000-10FFFD : Supplementary Private Use Area B , U+110000-FFFFFFFF non-used
}
bool TextFileR::IsAscii(uchar c) { return 0x20 <= c && c < 0x80; }
bool TextFileR::IsSurrogateLead(qbyte w) { return 0xD800 <= w && w <= 0xDBFF; }

bool TextFileR::CheckUTFConfidence(const uchar* ptr, ulong siz, unsigned int uChrSize, bool LE)
{
	qbyte uchr;
	ulong usize = siz / uChrSize, x = 0;
	ulong unconfidence = 0, confidence = 0;
	bool impossible = false, prevIsNull = false;
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
			impossible = true;
			break;
		}
		if((0x00 <= uchr && uchr < 0x80)) confidence+=4; // unicode ASCII
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
	TCHAR utfTmp[80];
	::wsprintf(utfTmp,TEXT("uChrSize=%d,LE=%d,usize=%d, confidence=%d, unconfidence=%d, result=%d"),uChrSize,LE,usize,confidence,unconfidence,(impossible?0:(confidence-unconfidence > usize)));
	::MessageBox(NULL,utfTmp,TEXT("UTFDetect"),0);
# endif

	if( impossible ) return false;
	else return (confidence-unconfidence > usize);
}

//=========================================================================
// テキストファイル出力共通インターフェイス
//=========================================================================

struct ki::TextFileWPimpl : public Object
{
	virtual void WriteLine( const unicode* buf, ulong siz )
		{ while( siz-- ) WriteChar( *buf++ ); }

	virtual void WriteLB( const unicode* buf, ulong siz )
		{ WriteLine( buf, siz ); }

	virtual void WriteChar( unicode ch )
		{}

	~TextFileWPimpl()
		{ delete [] buf_; }

protected:

	TextFileWPimpl( FileW& w )
		: fp_   (w)
		, bsiz_ (65536)
		, buf_  (new char[bsiz_]) {}

	void ReserveMoreBuffer()
		{
			char* nBuf = new char[bsiz_<<=1];
			delete [] buf_;
			buf_ = nBuf;
		}

	FileW& fp_;
	ulong  bsiz_;
	char*  buf_;
};



//-------------------------------------------------------------------------
// Unicodeテキスト
//-------------------------------------------------------------------------

struct wUtf16LE : public TextFileWPimpl
{
	wUtf16LE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom){ unicode ch=0xfeff; fp_.Write(&ch,2); } }
	void WriteLine( const unicode* buf, ulong siz ) {fp_.Write(buf,siz*2);}
};

struct wUtf16BE : public TextFileWPimpl
{
	wUtf16BE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) WriteChar(0xfeff); }
	void WriteChar( unicode ch ) { fp_.WriteC(ch>>8), fp_.WriteC(ch&0xff); }
};

struct wUtf32LE : public TextFileWPimpl
{
	wUtf32LE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) {unicode c=0xfeff; WriteLine(&c,1);} }
//	void WriteChar( unicode ch )
//		{ fp_.WriteC(ch&0xff), fp_.WriteC(ch>>8), fp_.WriteC(0), fp_.WriteC(0); }
	virtual void WriteLine( const unicode* buf, ulong siz )
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
			for(int i=0; i<=3; ++i)
				fp_.WriteC( (uchar)(cc>>(8*i)) );
		}
	}
};

struct wUtf32BE : public TextFileWPimpl
{
	wUtf32BE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) {unicode c=0xfeff; WriteLine(&c,1);} }
//	void WriteChar( unicode ch )
//		{ fp_.WriteC(0), fp_.WriteC(0), fp_.WriteC(ch>>8), fp_.WriteC(ch&0xff); }
	virtual void WriteLine( const unicode* buf, ulong siz )
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
			for(int i=3; i>=0; --i)
				fp_.WriteC( (uchar)(cc>>(8*i)) );
		}
	}
};

struct wWest : public TextFileWPimpl
{
	wWest( FileW& w ) : TextFileWPimpl(w) {}
	void WriteChar( unicode ch ) { fp_.WriteC(ch>0xff ? '?' : (uchar)ch); }
};

struct wUtf1 : public TextFileWPimpl
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
	void WriteChar( unicode ch )
	{
		qbyte c = ch;
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

		if( c <= 0x9f )
			fp_.WriteC( static_cast<uchar>(c) );
		else if( c <= 0xff )
			fp_.WriteC( 0xA0 ),
			fp_.WriteC( static_cast<uchar>(c) );
		else if( c <= 0x4015 )
			fp_.WriteC( static_cast<uchar>(0xA1 + (c - 0x100) / 0xBE) ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x100) % 0xBE)) );
		else if( c <= 0x38E2D )
			fp_.WriteC( static_cast<uchar>(0xF6 + (c - 0x4016) / (0xBE*0xBE))  ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x4016) / 0xBE % 0xBE)) ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x4016) % 0xBE)) );
		else
			fp_.WriteC( static_cast<uchar>(0xFC + (c - 0x38E2E) / (0xBE*0xBE*0xBE*0xBE))  ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x38E2E) / (0xBE*0xBE*0xBE) % 0xBE)) ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x38E2E) / (0xBE*0xBE) % 0xBE)) ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x38E2E) / 0xBE % 0xBE)) ),
			fp_.WriteC( static_cast<uchar>(conv((c - 0x38E2E) % 0xBE)) );
	}
};

struct wUtf9 : public TextFileWPimpl
{
	wUtf9( FileW& w, bool bom ) : TextFileWPimpl(w), SurrogateHi(0)
	{
		if( bom ) // BOM書き込み
			fp_.Write( "\x93\xFD\xFF", 3 );
	}

	qbyte SurrogateHi;

	void WriteChar( unicode ch )
	{
		qbyte c = ch;
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

		if( c <= 0x7F || (c >= 0xA0 && c <= 0xFF ))
			fp_.WriteC( static_cast<uchar>(c) );
		else if( c <= 0x07FF )
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7)) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else if( c <= 0xFFFF )
			fp_.WriteC( static_cast<uchar>(0x90 | (c >> 14)) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else if( c <= 0x7FFFFF )
			fp_.WriteC( static_cast<uchar>(0x94 | (c >> 21)) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else
			fp_.WriteC( static_cast<uchar>(0x98 | (c >> 28) & 0x07) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 21) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
	}
};

struct wUtfOFSS : public TextFileWPimpl
{
	wUtfOFSS( FileW& w, bool bom ) : TextFileWPimpl(w), SurrogateHi(0)
	{
		if( bom ) // BOM書き込み
			fp_.Write( "\xC3\xBC\xFF", 3 );
	}

	qbyte SurrogateHi;

	void WriteChar( unicode ch )
	{
		qbyte c = ch;
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

		if( c <= 0x7F )
			fp_.WriteC( static_cast<uchar>(c) );
		else if( c <= 0x1fff + 0x0000080 )
			c -= 0x0000080,
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7)) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else if( c <= 0x7ffff + 0x0002080 )
			c -= 0x0002080,
			fp_.WriteC( static_cast<uchar>(0xc0 | (c >> 14)) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else if( c <= 0x1ffffff + 0x0082080 )
			c -= 0x0082080,
			fp_.WriteC( static_cast<uchar>(0xe0 | (c >> 21)) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
		else
			c -= 0x2082080,
			fp_.WriteC( static_cast<uchar>(0xf0 | (c >> 28) & 0x07) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 21) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 14) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c >> 7) & 0x7F) ),
			fp_.WriteC( static_cast<uchar>(0x80 | (c & 0x7F)) );
	}
};

struct wUtf5 : public TextFileWPimpl
{
	wUtf5( FileW& w ) : TextFileWPimpl(w) {}
	void WriteChar( unicode ch )
	{
		static const char conv[] = {
			'0','1','2','3','4','5','6','7',
				'8','9','A','B','C','D','E','F' };
		if(ch<0x10)
		{
			fp_.WriteC(ch+'G');
		}
		else if(ch<0x100)
		{
			fp_.WriteC((ch>>4)+'G');
			fp_.WriteC(conv[ch&0xf]);
		}
		else if(ch<0x1000)
		{
			fp_.WriteC((ch>>8)+'G');
			fp_.WriteC(conv[(ch>>4)&0xf]);
			fp_.WriteC(conv[ch&0xf]);
		}
		else
		{
			fp_.WriteC((ch>>12)+'G');
			fp_.WriteC(conv[(ch>>8)&0xf]);
			fp_.WriteC(conv[(ch>>4)&0xf]);
			fp_.WriteC(conv[ch&0xf]);
		}
	}
};

//-------------------------------------------------------------------------
// SCSU Encoder
// Code portion from http://unicode.org/Public/PROGRAMS/SCSUMini/scsumini.c
// created by: Markus W. Scherer
//-------------------------------------------------------------------------

namespace {
	static const ulong SCSU_offsets[16]={
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
struct wSCSU : public TextFileWPimpl
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

	wSCSU( FileW& w ) : TextFileWPimpl(w), isUnicodeMode(0), win(0)
	{
		WriteChar( 0xfeff ); // Write BOM
	}

    char isUnicodeMode, win;

	char isInWindow(ulong offset, ulong c)
	{
		return (char)(offset<=c && c<=(offset+0x7f));
	}

	// get the index of the static/dynamic window that contains c; -1 if none
	int getWindow(ulong c)
	{
		int i;

		for(i=0; i<16; ++i) {
			if(isInWindow(SCSU_offsets[i], c)) {
				return i;
			}
		}
		return -1;
	}

	void WriteChar( unicode c )
	{
		char window; // dynamic window 0..7
		int w;       // result of getWindow(), -1..7

		if( c<0 || c>0x10ffff || (isUnicodeMode&~1)!=0 || (win&~7)!=0 )
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
				if( c<=0xf && ((1<<c)&0x2601)==0 ) {
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
struct wBOCU1 : public TextFileWPimpl
{
	wBOCU1( FileW& w ) : TextFileWPimpl(w), pc ( 0x40 ), cp ( 0 ), diff( 0 )
	{ // write BOM
		fp_.WriteC( static_cast<uchar>(0xfb) );
		fp_.WriteC( static_cast<uchar>(0xee) );
		fp_.WriteC( static_cast<uchar>(0x28) );
	}

	unicode cp, pc;
	long diff;

	void WriteChar( unicode ch )
	{
		uchar t1,t2,t3;

		if (ch <= 0x20) {
            if(ch != 0x20) pc = 0x40;
			fp_.WriteC( static_cast<uchar>(ch) );
        } else {
            diff = ch - pc;
            if (diff < -187660) { // [...,-187660) : 21
                diff -= -14536567;
                t3 = (uchar)(diff % 243); diff/=243;
                t2 = (uchar)(diff % 243); diff/=243;
                t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteC( static_cast<uchar>(0x21) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t3 ]) );
            } else if (diff < -10513) { // [-187660,-10513) : 22-24
                diff -= -187660;
                t2 = (uchar)(diff % 243); diff/=243;
                t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteC( static_cast<uchar>(0x22 + diff) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
            } else if (diff < -64) { // [-10513,-64) : 25-4F
                diff -= -10513;
                t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteC( static_cast<uchar>(0x25 + diff) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
            } else if (diff < 64) { // [-64,63) : 50-CF
                diff -= -64;
				fp_.WriteC( static_cast<uchar>(0x50 + diff) );
            } else if (diff < 10513) { // [64,10513) : D0-FA
                diff -= 64;
                t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteC( static_cast<uchar>(0xd0 + diff) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
            } else if (diff < 187660) { // [10513,187660) : FB-FD
                diff -= 10513;
                t2 = (uchar)(diff % 243); diff/=243;
                t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteC( static_cast<uchar>(0xfb + diff) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
            } else { // [187660,...) : FE
                diff -= 187660;
                t3 = (uchar)(diff % 243); diff/=243;
                t2 = (uchar)(diff % 243); diff/=243;
                t1 = (uchar)(diff % 243); diff/=243;
				fp_.WriteC( static_cast<uchar>(0xfe) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t1 ]) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t2 ]) );
				fp_.WriteC( static_cast<uchar>(bocu1_trail_to_byte[ t3 ]) );
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

struct wUTF8 : public TextFileWPimpl
{
	wUTF8( FileW& w, int cp )
		: TextFileWPimpl(w)
	{
		if( cp == UTF8 ) // BOM書き込み
			fp_.Write( "\xEF\xBB\xBF", 3 );
	}

	void WriteLine( const unicode* str, ulong len )
	{
		//        0000-0000-0xxx-xxxx | 0xxxxxxx
		//        0000-0xxx-xxyy-yyyy | 110xxxxx 10yyyyyy
		//        xxxx-yyyy-yyzz-zzzz | 1110xxxx 10yyyyyy 10zzzzzz
		// x-xxyy-yyyy-zzzz-zzww-wwww | 11110xxx 10yyyyyy 10zzzzzz 10wwwwww
		// ...
		while( len-- )
		{
			qbyte ch = *str;
			if( (0xD800<=ch&&ch<=0xDBFF) && len )
				ch = 0x10000 + (((ch-0xD800)&0x3ff)<<10) + ((*++str-0xDC00)&0x3ff), len--;

			if( ch <= 0x7f )
				fp_.WriteC( static_cast<uchar>(ch) );
			else if( ch <= 0x7ff )
				fp_.WriteC( 0xc0 | static_cast<uchar>(ch>>6) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch<= 0xffff )
				fp_.WriteC( 0xe0 | static_cast<uchar>(ch>>12) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch<= 0x1fffff )
				fp_.WriteC( 0xf0 | static_cast<uchar>(ch>>18) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch<= 0x3ffffff )
				fp_.WriteC( 0xf8 | static_cast<uchar>(ch>>24) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>18)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else
				fp_.WriteC( 0xfc | static_cast<uchar>(ch>>30) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>24)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>18)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			++str;
		}
	}
};



struct wUTF7 : public TextFileWPimpl
{
	wUTF7( FileW& w ) : TextFileWPimpl(w) {}

	void WriteLine( const unicode* str, ulong len )
	{
		static const uchar mime[64] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'};

		// XxxxxxYyyyyyZzzz | zzWwwwwwUuuuuuVv | vvvvTtttttSsssss
		bool mode_m = false;
		while( len )
		{
			if( *str <= 0x7f )
			{
				fp_.WriteC( static_cast<uchar>(*str) );
				if( *str == L'+' )
					fp_.WriteC( '-' );
				++str, --len;
			}
			else
			{
				if(!mode_m) fp_.WriteC( '+' ), mode_m=true;
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
					fp_.WriteC( mime[ tx[0]>>10 ] );
					fp_.WriteC( mime[ (tx[0]>>4)&0x3f ] );
					fp_.WriteC( mime[ (tx[0]<<2|tx[1]>>14)&0x3f ] );
					if( n>=2 )
					{
						fp_.WriteC( mime[ (tx[1]>>8)&0x3f ] );
						fp_.WriteC( mime[ (tx[1]>>2)&0x3f ] );
						fp_.WriteC( mime[ (tx[1]<<4|tx[2]>>12)&0x3f ] );
						if( n>=3 )
						{
							fp_.WriteC( mime[ (tx[2]>>6)&0x3f ] );
							fp_.WriteC( mime[ tx[2]&0x3f ] );
						}
					}
				}
				if( len && *str<=0x7f )
					fp_.WriteC( '-' ), mode_m = false;
			}
		}
	}
};



//#endif
//-------------------------------------------------------------------------
// Windows頼りの変換
//-------------------------------------------------------------------------

struct wMBCS : public TextFileWPimpl
{
	wMBCS( FileW& w, int cp )
		: TextFileWPimpl(w), cp_(cp)
	{
		if( cp == UTF8 )
		{
			// BOM書き込み
			cp_ = UTF8N;
			fp_.Write( "\xEF\xBB\xBF", 3 );
		}
	}

	void WriteLine( const unicode* str, ulong len )
	{
		// WideCharToMultiByte API を利用した変換
		int r;
		while(
			0==(r=::WideCharToMultiByte(cp_,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		// ファイルへ書き込み
		fp_.Write( buf_, r );
	}

	int cp_;
};



//-------------------------------------------------------------------------
// ISO-2022 サブセットその１。
// ASCIIともう一つしか文字集合を使わないもの
//-------------------------------------------------------------------------

struct wIso2022 : public TextFileWPimpl
{
	wIso2022( FileW& w, int cp )
		: TextFileWPimpl(w), hz_(cp==HZ)
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

	void WriteLine( const unicode* str, ulong len )
	{
		// まず WideCharToMultiByte API を利用して変換
		int r;
		while(
			0==(r=::WideCharToMultiByte(cp_,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		bool ascii = true;
		for( int i=0; i<r; ++i )
			if( buf_[i] & 0x80 )
			{
				// 非ASCII部分は最上位ビットを落としてから出力
				if( ascii )
				{
					if( hz_ )
						fp_.WriteC( 0x7E ), fp_.WriteC( 0x7B );
					else
						fp_.WriteC( 0x0E );
					ascii = false;
				}
				fp_.WriteC( buf_[i++] & 0x7F );
				fp_.WriteC( buf_[i]   & 0x7F );
			}
			else
			{
				// ASCII部分はそのまま出力
				if( !ascii )
				{
					if( hz_ )
						fp_.WriteC( 0x7E ), fp_.WriteC( 0x7D );
					else
						fp_.WriteC( 0x0F );
					ascii = true;
				}
				fp_.WriteC( buf_[i] );

				// ただしHZの場合、0x7E は 0x7E 0x7E と表す
				if( hz_ && buf_[i]==0x7E )
					fp_.WriteC( 0x7E );
			}

		// 最後は確実にASCIIに戻す
		if( !ascii )
			if( hz_ )
				fp_.WriteC( 0x7E ), fp_.WriteC( 0x7D );
			else
				fp_.WriteC( 0x0F );
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
static const WORD IBM_sjis2kuten[] = {
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

struct wEucJp : public TextFileWPimpl
{
	wEucJp( FileW& w )
		: TextFileWPimpl(w) {}

	void WriteLine( const unicode* str, ulong len )
	{
		// まず WideCharToMultiByte API を利用して変換
		int r;
		while(
			0==(r=::WideCharToMultiByte(932,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		for( int i=0; i<r; ++i )
			if( buf_[i] & 0x80 )
			{
				if( 0xA1<=(uchar)buf_[i] && (uchar)buf_[i]<=0xDF )
				{
					// カナ
					fp_.WriteC( 0x8E );
					fp_.WriteC( buf_[i] );
				}
				else
				{
					// JIS X 0208
					sjis2jis( buf_[i], buf_[i+1], buf_+i );
					fp_.WriteC( buf_[i++] | 0x80 );
					fp_.WriteC( buf_[i]   | 0x80 );
				}
			}
			else
			{
				// ASCII部分はそのまま出力
				fp_.WriteC( buf_[i] );
			}
	}
};



//-------------------------------------------------------------------------
// ISO-2022 サブセットその３。ISO-2022-JP
//-------------------------------------------------------------------------

struct wIsoJp : public TextFileWPimpl
{
	wIsoJp( FileW& w )
		: TextFileWPimpl(w)
	{
		fp_.Write( "\x1b\x28\x42", 3 );
	}

	void WriteLine( const unicode* str, ulong len )
	{
		// まず WideCharToMultiByte API を利用して変換
		int r;
		while(
			0==(r=::WideCharToMultiByte(932,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

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
	: cs_( charset ), lb_(linebreak)
{
}

TextFileW::~TextFileW()
{
	Close();
}

void TextFileW::Close()
{
	impl_ = NULL;
	fp_.Close();
}

void TextFileW::WriteLine( const unicode* buf, ulong siz, bool lastline )
{
	impl_->WriteLine( buf, siz );
	if( !lastline )
	{
		static const ulong lbLst[] = {0x0D, 0x0A, 0x000A000D};
		static const ulong lbLen[] = {   1,    1,          2};
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
	case Western: impl_ = new wWest( fp_ ); break;
	case UTF1Y:
	case UTF1:    impl_ = new wUtf1( fp_, cs_==UTF1Y ); break;
	case UTF5:    impl_ = new wUtf5( fp_ ); break;
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
	case SCSU:    impl_ = new wSCSU( fp_ ); break;
	case BOCU1:   impl_ = new wBOCU1( fp_ ); break;
	case OFSSUTF:
	case OFSSUTFY: impl_ = new wUtfOFSS( fp_, cs_==OFSSUTFY ); break;
	case EucJP:   impl_ = new wEucJp( fp_ ); break;
	case IsoJP:   impl_ = new wIsoJp( fp_ ); break;
	case IsoKR:   impl_ = new wIso2022( fp_, cs_ ); break;
	case IsoCN:   impl_ = new wIso2022( fp_, cs_ ); break;
	case HZ:      impl_ = new wIso2022( fp_, cs_ ); break;
	case UTF8:
	case UTF8N:
	default:
#ifndef _UNICODE
		if( /*app().isWin95() &&*/ (cs_==UTF8 || cs_==UTF8N) )
			impl_ = new wUTF8( fp_, cs_ );
		else if( /*app().isWin95() &&*/ cs_==UTF7 )
			impl_ = new wUTF7( fp_ );
		else
#else
		if( (cs_==UTF8 || cs_==UTF8N) && !::IsValidCodePage(65001) )
			impl_ = new wUTF8( fp_, cs_ );
		else if( cs_==UTF7 && !::IsValidCodePage(65000) )
			impl_ = new wUTF7( fp_ );
		else
#endif
		impl_ = new wMBCS( fp_, cs_ );
		break;
	}
	return true;
}
