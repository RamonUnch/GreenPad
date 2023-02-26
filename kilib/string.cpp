#include "stdafx.h"
#include "app.h"
#include "memory.h"
#include "kstring.h"
using namespace ki;

static wchar_t SingleCharUpperW_nonNT(wchar_t c)
{
	char buf[4];
	BOOL defcharused=FALSE;
	if( ::WideCharToMultiByte( CP_ACP, 0, &c, 1, buf, 4, NULL, &defcharused ) != 1 || defcharused )
		return c; // Cannot convert to a single char!

	char uc = (char)(LONG_PTR)::CharUpperA( (char *)(LONG_PTR)((uchar)buf[0]) ) ;
	unicode uuc;
	if( ::MultiByteToWideChar( CP_ACP, 0, &uc, 1, &uuc, 1 ))
		return uuc;
	return c; // Convertion failed!
}
wchar_t * WINAPI my_CharUpperWW(wchar_t *s)
{
	if( !( (LONG_PTR)s &0xffff0000 ) )
	{
		wchar_t c = (wchar_t)(LONG_PTR)s;
		return (wchar_t *)(LONG_PTR)SingleCharUpperW_nonNT( c );
	}
	else
	{
		wchar_t *os = s;
		while( (*s = SingleCharUpperW_nonNT( *s )) )
			s++;
		return os;
	}
}

static wchar_t SingleCharLowerW_nonNT(wchar_t c)
{
	char buf[4];
	BOOL defcharused=FALSE;
	if( ::WideCharToMultiByte( CP_ACP, 0, &c, 1, buf, 4, NULL, &defcharused ) != 1 || defcharused )
		return c; // Cannot convert to a single char!

	char uc = (char)(LONG_PTR)::CharLowerA( (char *)(LONG_PTR)((uchar)buf[0]) ) ;
	unicode uuc;
	if( ::MultiByteToWideChar( CP_ACP, 0, &uc, 1, &uuc, 1 ))
		return uuc;
	return c; // Convertion failed!
}
wchar_t * WINAPI my_CharLowerWW(wchar_t *s)
{
	if( !( (LONG_PTR)s &0xffff0000 ) )
	{
		wchar_t c = (wchar_t)(LONG_PTR)s;
		return (wchar_t *)(LONG_PTR)SingleCharLowerW_nonNT( c );
	}
	else
	{
		wchar_t *os = s;
		while( (*s = SingleCharLowerW_nonNT( *s )) )
			s++;
		return os;
	}
}
wchar_t my_CharUpperSingleW(wchar_t c)
{
#if defined(UNICOWS) || !defined(_UNICODE)
	if( app().isNT() )
		return (wchar_t)(LONG_PTR)::CharUpperW( (wchar_t *)(LONG_PTR)c );
	else
		return SingleCharUpperW_nonNT(c);
#else
	return (wchar_t)(LONG_PTR)::CharUpperW( (wchar_t *)(LONG_PTR)c );
#endif
}
wchar_t my_CharLowerSingleW(wchar_t c)
{
#if defined(UNICOWS) || !defined(_UNICODE)
	if( app().isNT() )
		return (wchar_t)(LONG_PTR)::CharLowerW( (wchar_t *)(LONG_PTR)c );
	else
		return SingleCharLowerW_nonNT(c);
#else
	return (wchar_t)(LONG_PTR)::CharLowerW( (wchar_t *)(LONG_PTR)c );
#endif
}
static BOOL my_IsCharLowerW_nonNT(wchar_t c)
{
	char buf[4];
	BOOL defcharused=FALSE;
	if( ::WideCharToMultiByte( CP_ACP, 0, &c, 1, buf, 4, NULL, &defcharused ) != 1 || defcharused )
		return FALSE; // Cannot convert to a single char!

	return ::IsCharLowerA( buf[0] );
}
BOOL my_IsCharLowerW(wchar_t c)
{
#if defined(UNICOWS) || !defined(_UNICODE)
	if( app().isNT() )
		return ::IsCharLowerW( c );
	else
		return my_IsCharLowerW_nonNT(c);
#else
	return ::IsCharLowerW( c );
#endif

}

#ifdef OLDWIN32S
#undef WideCharToMultiByte
#undef MultiByteToWideChar
int WINAPI SimpleWC2MB_1st(UINT cp, DWORD flg, LPCWSTR s, int sl, LPSTR d, int dl, LPCSTR defc, LPBOOL useddef);
int (WINAPI *SimpleWC2MB)(UINT cp, DWORD flg, LPCWSTR s, int sl, LPSTR d, int dl, LPCSTR defc, LPBOOL useddef) = SimpleWC2MB_1st;
int WINAPI SimpleWC2MB_fb(UINT cp, DWORD flg, LPCWSTR s, int sl, LPSTR d, int dl, LPCSTR defc, LPBOOL useddef);
int WINAPI SimpleWC2MB_1st(UINT cp, DWORD flg, LPCWSTR s, int sl, LPSTR d, int dl, LPCSTR defc, LPBOOL useddef)
{
	// Try with a simple test buffer to see if the native function works.
	char mb[2]; mb[0] = '\0';
	int ret = ::WideCharToMultiByte(CP_ACP, 0, L"ts",2 , mb, countof(mb), NULL, NULL);
	if( ret && mb[0] == 't' && mb[1] == 's' )
		SimpleWC2MB = ::WideCharToMultiByte;
	else
		SimpleWC2MB = SimpleWC2MB_fb;

	return SimpleWC2MB(cp, flg, s, sl, d, dl, defc, useddef);
}
int WINAPI SimpleWC2MB_fb(UINT cp, DWORD flg, LPCWSTR s, int sl, LPSTR d, int dl, LPCSTR defc, LPBOOL useddef)
{
	if( d == NULL || dl == 0 ) // return required length.
		return sl==-1? my_lstrlenW(s): sl;

	int i;
	if( sl == -1 )
	{ // Copy until NULL
		for( i=0; i < dl && s[i]; i++)
			d[i] = (char)s[i];
		d[i] = '\0';
		return i == dl? 0: i;
	}

	if( dl <= sl )
		return 0;

	for( i=0; i < sl; i++)
		d[i] = (char)s[i];

	d[i] = '\0';
	return i;
}
int WINAPI SimpleMB2WC_1st(UINT cp, DWORD flg, LPCSTR s, int sl, LPWSTR d, int dl);
int(WINAPI*SimpleMB2WC)   (UINT cp, DWORD flg, LPCSTR s, int sl, LPWSTR d, int dl) = SimpleMB2WC_1st;
int WINAPI SimpleMB2WC_fb (UINT cp, DWORD flg, LPCSTR s, int sl, LPWSTR d, int dl);
int WINAPI SimpleMB2WC_1st(UINT cp, DWORD flg, LPCSTR s, int sl, LPWSTR d, int dl)
{
	// Try with a simple test buffer to see if the native function works.
	wchar_t wc[2]; wc[0] = '\0';
	int ret = ::MultiByteToWideChar(CP_ACP, 0, "ts",2 , wc,countof(wc));
	if( ret && wc[0] == L't' && wc[1] == L's' )
		SimpleMB2WC = ::MultiByteToWideChar;
	else
		SimpleMB2WC = SimpleMB2WC_fb;

	return SimpleMB2WC(cp, flg, s, sl, d, dl);
}
int WINAPI SimpleMB2WC_fb(UINT cp, DWORD flg, LPCSTR s, int sl, LPWSTR d, int dl)
{
	if( d == NULL || dl == 0 )
		return sl==-1? my_lstrlenA(s): sl;

	int i;
	if( sl == -1 )
	{ // Copy until NULL
		for( i=0; i < dl && s[i]; i++)
			d[i] = (wchar_t)s[i];
		d[i] = L'\0';
		return i == dl? 0: i;
	}

	if( dl <= sl )
		return 0;

	for( i=0; i < sl; i++)
		d[i] = (char)s[i];
	d[i] = L'\0';
	return i;
}
#define WideCharToMultiByte SimpleWC2MB
#define MultiByteToWideChar SimpleMB2WC
#endif// OLDWIN32S

//=========================================================================
String::StringData* String::nullData_;
#if !defined(_UNICODE) && defined(_MBCS)
char                String::lb_[256];
#endif

void String::LibInit()
{
	static size_t nullstr_image[4];
	nullstr_image[0] = 1;
	nullstr_image[1] = 1;
	nullstr_image[2] = 4;
	nullData_ = reinterpret_cast<StringData*>(nullstr_image);

#if !defined(_UNICODE) && defined(_MBCS)
	for( int c=0; c<256; ++c )
		lb_[c] = (::IsDBCSLeadByte(c) ? 2 : 1);
#endif
}



//-------------------------------------------------------------------------

String::String( const TCHAR* s, long len )
{
	// 長さ指定が無い場合は計算
	if( len==-1 )
		len = my_lstrlen(s);

	if( len==0 )
	{
		// 0文字用の特殊バッファ
		SetData( null() );
	}
	else
	{
		// 新規バッファ作成
		data_ = static_cast<StringData*>
		    (mem().Alloc( sizeof(StringData)+(len+1)*sizeof(TCHAR) ));
		data_->ref  = 1;
		data_->len  = len+1;
		data_->alen = len+1;
		memmove( data_+1, s, (len+1)*sizeof(TCHAR) );
	}
}

inline void String::ReleaseData()
{
	if( --data_->ref <= 0 )
		mem().DeAlloc(
			data_, sizeof(StringData)+sizeof(TCHAR)*data_->alen );
}

String::~String()
{
	ReleaseData();
}

TCHAR* String::ReallocMem( size_t minimum=0 )
{
	return AllocMemHelper( minimum, c_str(), len()+1 );
}

String& String::SetString( const TCHAR* str, size_t siz )
{
	TCHAR* buf = AllocMem( siz+1 );

	memmove( buf, str, siz*sizeof(TCHAR) );
	buf[siz] = TEXT('\0');

	UnlockMem( siz );
	return *this;
}

String& String::CatString( const TCHAR* str, size_t siz )
{
	const int plen = len();
	TCHAR* buf = ReallocMem( plen + siz + 1 );

	memmove( buf+plen, str, siz*sizeof(TCHAR) );
	buf[plen+siz] = TEXT('\0');

	UnlockMem( plen+siz );
	return *this;
}

TCHAR* String::AllocMemHelper( size_t minimum, const TCHAR* str, size_t siz )
{
	if( data_->ref > 1 || data_->alen < minimum )
	{
		minimum = Max( minimum, data_->alen );

		StringData* pNew = static_cast<StringData*>
			(mem().Alloc( sizeof(StringData)+minimum*sizeof(TCHAR) ));
		pNew->ref  = 1;
		pNew->alen = minimum;
		pNew->len  = siz;
		memmove( pNew->buf(), str, siz*sizeof(TCHAR) );

		ReleaseData();
		data_ = pNew;
	}

	return data_->buf();
}

String& String::operator = ( const String& obj )
{
	if( data() != obj.data() )
	{
		ReleaseData();
		SetData( obj.data() );
	}
	return *this;
}

#ifdef _UNICODE
String& String::operator = ( const char* s )
{
	long len = ::MultiByteToWideChar( CP_ACP, 0, s, -1, NULL, 0 );
	::MultiByteToWideChar( CP_ACP, 0, s, -1, AllocMem(len+1), len+1 );
#else
String& String::operator = ( const wchar_t* s )
{
	long len = ::WideCharToMultiByte(CP_ACP,0,s,-1,NULL,0,NULL,NULL);
	::WideCharToMultiByte(CP_ACP,0,s,-1,AllocMem(len+1),len+1,NULL,NULL);
#endif
	UnlockMem( len );
	return *this;
}

String& String::Load( UINT rsrcID )
{
	const int step=256;

	// 256バイトの固定長バッファへまず読んでみる
	TCHAR tmp[step], *buf;
	int red = app().LoadString( rsrcID, tmp, countof(tmp) );
	if( countof(tmp) - red > 2 )
		return (*this = tmp);

	// 少しずつ増やして対応してみる
	size_t siz = step;
	do
	{
		siz+= step;
		buf = AllocMem( siz );
		red = app().LoadString( rsrcID, buf, siz );
	} while( siz - red <= 2 );

	buf[red] = TEXT('\0');
	UnlockMem( red );
	return *this;
}

void String::TrimRight( size_t siz )
{
	if( siz >= len() )
	{
		ReleaseData();
		SetData( null() );
	}
	else
	{
		// 文字列バッファの参照カウントを確実に１にする
		ReallocMem();

		// 指定文字数分削る
		data_->len -= siz;
		data_->buf()[data_->len-1] = TEXT('\0');
	}
}

int String::GetInt( const TCHAR* x )
{
	int n=0;
	bool minus = (*x==TEXT('-'));
	for( const TCHAR* p=(minus?x+1:x); *p!=TEXT('\0'); p=next(p) )
	{
		if( *p<TEXT('0') || TEXT('9')<*p )
			return 0;
		n = (10*n) + (*p-TEXT('0'));
	}
	return minus ? -n : n;
}

String& String::SetInt( int n )
{
	if( n==0 )
	{
		*this = TEXT("0");
	}
	else
	{
		bool minus = (n<0);
		if( minus )
			n= -n;

		TCHAR tmp[20];
		tmp[19] = TEXT('\0');
		int i;

		for( i=18; i>=0; --i )
		{
			tmp[i] = TEXT('0') + n%10;
			n /= 10;
			if( n==0 )
				break;
		}

		if( minus )
			tmp[--i] = TEXT('-');

		*this = tmp+i;
	}
	return *this;
}

const wchar_t* String::ConvToWChar() const
{
#ifdef _UNICODE
	return c_str();
#else
	int ln = ::MultiByteToWideChar( CP_ACP,  0, c_str(), -1 , 0, 0 );
	wchar_t* p = new wchar_t[ln+1];
	::MultiByteToWideChar( CP_ACP,  0, c_str(), -1 , p, ln+1 );
	return p;
#endif
}

const char* String::ConvToChar() const
{
#ifdef _UNICODE
	int ln = ::WideCharToMultiByte( CP_ACP, 0, c_str(), -1, NULL, 0, NULL, NULL );
	char* p = new char[ln+32];
	::WideCharToMultiByte( CP_ACP,  0, c_str(), -1 , p, ln+1, NULL, NULL );
//	MessageBoxA(NULL, p, NULL, 0);
	return p;
#else
	return c_str();
#endif
}
