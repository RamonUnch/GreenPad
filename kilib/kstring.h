#ifndef _KILIB_STRING_H_
#define _KILIB_STRING_H_
#include "types.h"
#include "memory.h"
#include "ktlaptr.h"
#ifndef __ccdoc__


#ifdef UNICODE
	#define my_lstrcpy my_lstrcpyW
	#define my_lstrcpyn my_lstrcpynW
	#define my_lstrlen my_lstrlenW
	#define my_lstrcmp my_lstrcmpW
	#define my_lstrchr my_lstrchrW
	#define my_lstrcat my_lstrcatW
	#define my_lstrkpy my_lstrkpyW
#else
	#define my_lstrcpy my_lstrcpyA
	#define my_lstrcpyn my_lstrcpynA
	#define my_lstrlen my_lstrlenA
	#define my_lstrcmp my_lstrcmpA
	#define my_lstrchr my_lstrchrA
	#define my_lstrcat my_lstrcatA
	#define my_lstrkpy my_lstrkpyA
#endif

#ifdef OLDWIN32S
extern int (WINAPI *SimpleMB2WC)(UINT cp, DWORD flg, LPCSTR s, int sl, LPWSTR d, int dl);
extern int (WINAPI *SimpleWC2MB)(UINT cp, DWORD flg, LPCWSTR s, int sl, LPSTR d, int dl, LPCSTR defc, LPBOOL useddef);
#define WideCharToMultiByte SimpleWC2MB
#define MultiByteToWideChar SimpleMB2WC
#endif

wchar_t * WINAPI my_CharUpperWW(wchar_t *s);
wchar_t * WINAPI my_CharLowerWW(wchar_t *s);
#define my_CharUpperW my_CharUpperWW
#define my_CharLowerW my_CharLowerWW
wchar_t my_CharUpperSingleW(wchar_t c);
wchar_t my_CharLowerSingleW(wchar_t c);
BOOL my_IsCharLowerW(wchar_t c);
const TCHAR *Int2lStr(TCHAR str[20], int n);

inline static
const char *my_lstrcatA(char *dest, const char *src)
{
	char *orig=dest;
	for (; *dest; ++dest) ;	/* go to end of dest */
	for (; (*dest=*src); ++src,++dest) ;	/* then append from src */
	return orig;
}

inline static
const wchar_t *my_lstrcatW(wchar_t *dest, const wchar_t *src)
{
	wchar_t *orig=dest;
	for (; *dest; ++dest) ;	/* go to end of dest */
	for (; (*dest=*src); ++src,++dest) ;	/* then append from src */
	return orig;
}

inline static
char *my_lstrkpyA(char *dest, const char *src)
{
	for (; (*dest=*src); ++src,++dest) ;	/* then append from src */
	return dest;
}

inline static
wchar_t *my_lstrkpyW(wchar_t *dest, const wchar_t *src)
{
	for (; (*dest=*src); ++src,++dest) ;	/* then append from src */
	return dest;
}

inline static
const char *my_lstrchrA(const char *str, char c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}
inline static
const unicode *my_lstrchrW(const unicode *str, unicode c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}

inline static
unicode* my_lstrcpyW( unicode* const d, const unicode* s )
{
	for(unicode* n=d; (*n++ = *s++););
	return d;
}
inline static
size_t my_lstrlenW( const unicode* const d )
{
	const unicode* n=d;
	for(; *n; ++n);
	return static_cast<size_t>(n-d);
}
static inline
int my_lstrcmpW(const unicode *X, const unicode *Y)
{
	while (*X && *X == *Y) { X++; Y++; }
	return *(const unicode*)X - *(const unicode*)Y;
}
static inline
bool my_instringW(const unicode *X, const unicode *Y)
{ // return true if we find Y in the X string.
	while (*X && *X == *Y) { X++; Y++; }
	return !*Y; // Match if we reached the end of Y
}
inline static
char* my_lstrcpyA( char* const d, const char* s )
{
	for(char* n=d; (*n++ = *s++););
	return d;
}
inline static
size_t my_lstrlenA( const char* const d )
{
	const char* n;
	for(n=d; *n; ++n);
	return static_cast<size_t>(n-d);
}
static inline
int my_lstrcmpA(const char *X, const char *Y)
{
	while (*X && *X == *Y) { X++; Y++; }
	return *(const unsigned char*)X - *(const unsigned char*)Y;
}

static inline
wchar_t *my_lstrcpynW(wchar_t *out, const wchar_t *in, int outlen)
{
	int i;
	for (i=0; i<outlen && in[i]; i++)
	{
		out[i] = in[i];
	}
	out[i] = TEXT('\0');
	return out;
}
static inline
char *my_lstrcpynA(char *out, const char *in, int outlen)
{
	int i;
	for (i=0; i<outlen && in[i]; i++)
	{
		out[i] = in[i];
	}
	out[i] = TEXT('\0');
	return out;
}

namespace ki {
#endif
#ifdef _UNICODE
	#define XTCHAR char
#else
	#define XTCHAR wchar_t
#endif

//=========================================================================
//@{
//	文字列処理＋α２
//
//	Wide文字版関数を自前で
//@}
//=========================================================================
//#undef lstrcpy
//#undef lstrlen
//#undef lstrcmp


//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	文字列処理
//
//	かなりMFCのCStringをパクってます。とりあえず operator= による
//	単純代入にはほとんどコストがかからないようにしました。SubStr()の
//	時もコピーしないようにしようかとも思ったんですが、そこまでは
//	要らないだろうという気もするので…。
//@}
//=========================================================================

class String : public Object
{
public:

	//@{ 空文字列作成 //@}
	String();
	~String();

	//@{ 別のStringのコピー //@}
	String( const String& obj );

	//@{ 別の文字配列のコピー //@}
	String( const TCHAR* str, long siz=-1 );

	//@{ リソースから作成 //@}
	explicit String( UINT rsrcID );

	//@{ 大文字小文字を区別する比較 //@}
	bool operator==( LPCTSTR s ) const;
	bool operator==( const String& obj ) const;

	//@{ 大文字小文字を区別しない比較 //@}
	bool isSame( LPCTSTR s ) const;
	bool isSame( const String& obj ) const;

	//@{ 単純代入 //@}
	String& operator=( const String& obj );
	String& operator=( const TCHAR* s );
	String& operator=( const XTCHAR* s );

	//@{ 加算代入 //@}
	String& operator+=( const String& obj );
	String& operator+=( const TCHAR* s );
	String& operator+=( TCHAR c );

	//@{ リソースロード //@}
	String& Load( UINT rsrcID );

	//@{ 右を削る //@}
	void TrimRight( size_t siz );

	//@{ intから文字列へ変換 //@}
	String& SetInt( int n );

	//@{ 文字列からintへ変換 //@}
	int GetInt();

public:

	//@{ 文字列バッファを返す //@}
	const TCHAR* c_str() const;

	//@{ 長さ //@}
	size_t len() const;

	//@{ 要素 //@}
	const TCHAR operator[](int n) const;

	//@{ ワイド文字列に変換して返す //@}
	const wchar_t* ConvToWChar() const;
	const char* ConvToChar() const;

	//@{ ConvToWCharの返値バッファの解放 //@}
	void FreeWCMem( const wchar_t* wc ) const;
	void FreeCMem( const char* str ) const;

public:

	//@{ 次の一文字 //@}
	static TCHAR*       next( TCHAR* p );
	static const TCHAR* next( const TCHAR* p );

	//@{ ２バイト文字の先頭かどうか？ //@}
	static bool isLB( TCHAR c );

	//@{ 文字列からintへ変換 //@}
	static int GetInt( const TCHAR* p );

protected:

	// 書き込み可能なバッファを、終端含めて最低でもminimum文字分用意する
	TCHAR* AllocMem( size_t minimum );
	TCHAR* ReallocMem( size_t minimum );

	// 書き込み終了後、長さを再設定
	void UnlockMem( long siz=-1 );

private:

	struct StringData
	{
		long  ref;         // 参照カウンタ
		size_t len;        // 終端'\0'を含める長さ
		size_t alen;       // 割り当てられているメモリのサイズ
		TCHAR* buf() const // TCHAR buf[alen]
			{ return reinterpret_cast<TCHAR*>(
				const_cast<StringData*>(this+1)
			); }
	};

private:

	TCHAR*  AllocMemHelper( size_t minimum, const TCHAR* str, size_t siz );
	String& CatString( const TCHAR* str, size_t siz );
	String& SetString( const TCHAR* str, size_t siz );
	void    SetData( StringData* d );
	void    ReleaseData();
	static  StringData* null();
	        StringData* data() const;

private:

	StringData*        data_;
	static StringData* nullData_;
#if !defined(_UNICODE) && defined(_MBCS)
	static char        lb_[256];
#endif

private:

	static void LibInit();
	friend void APIENTRY Startup();
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

// 初期化
inline String::String()
	{ SetData( null() ); }
// 初期化
inline String::String( UINT rsrcID )
	{ SetData( null() ), Load( rsrcID ); }
// 初期化
inline String::String( const String& obj )
	{ SetData( obj.data() ); }

// ポインタ計算サポート
#if !defined(_UNICODE) && defined(_MBCS)
	inline TCHAR* String::next( TCHAR* p )
		{ return p + lb_[*(uchar*)p]; }
	inline const TCHAR* String::next( const TCHAR* p )
		{ return p + lb_[*(const uchar*)p]; }
	inline bool String::isLB( TCHAR c )
		{ return lb_[(uchar)c]==2; }
#else // _UNICODE or _SBCS
	inline TCHAR* String::next( TCHAR* p )
		{ return p + 1; }
	inline const TCHAR* String::next( const TCHAR* p )
		{ return p + 1; }
	inline bool String::isLB( TCHAR c )
		{ return false; }
#endif

// 内部メモリ確保
inline TCHAR* String::AllocMem( size_t minimum )
	{ return AllocMemHelper( minimum, TEXT(""), 1 ); }
// 内部メモリ固定
inline void String::UnlockMem( long siz )
	{ data_->len = 1 + (siz==-1 ? my_lstrlen(c_str()) : siz); }

// ０文字データ
inline String::StringData* String::null()
	{ return nullData_; }
// 内部データ構造
inline String::StringData* String::data() const
	{ return data_; }
// 初期化
inline void String::SetData( String::StringData* d )
	{ data_=d, data_->ref++; }

// 属性
inline const TCHAR* String::c_str() const
	{ return data_->buf(); }
// 属性
inline size_t String::len() const
	{ return data_->len-1; }
// 要素
inline const TCHAR String::operator[](int n) const
	{ return data_->buf()[n]; }

// 比較
inline bool String::operator==( LPCTSTR s ) const
	{ return 0==my_lstrcmp( c_str(), s ); }
// 比較
inline bool String::operator==( const String& obj ) const
	{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }
// 比較
inline bool String::isSame( LPCTSTR s ) const
{ return 0==::lstrcmpi( c_str(), s ); }
// 比較
inline bool String::isSame( const String& obj ) const
	{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }

// 要コピー代入
inline String& String::operator = ( const TCHAR* s )
	{ return SetString( s, my_lstrlen(s) ); }
// 合成
inline String& String::operator += ( const String& obj )
	{ return CatString( obj.c_str(), obj.len() ); }
// 合成
inline String& String::operator += ( const TCHAR* s )
	{ return CatString( s, my_lstrlen(s) ); }
// 合成
inline String& String::operator += ( TCHAR c )
	{ return CatString( &c, 1 ); }

// 変換
inline int String::GetInt()
	{ return GetInt( data_->buf() ); }

//@{ String + String //@}
inline const String operator+( const String& a, const String& b )
	{ return String(a) += b; }
//@{ String + TCHAR* //@}
inline const String operator+( const String& a, const TCHAR* b )
	{ return String(a) += b; }
//@{ TCHAR* + String //@}
inline const String operator+( const TCHAR* a, const String& b )
	{ return String(a) += b; }

// ConvToWCharの返値バッファの解放
inline void String::FreeWCMem( const wchar_t* wc ) const
#ifdef _UNICODE
	{}
#else // _MBCS or _SBCS
	{ delete [] const_cast<wchar_t*>(wc); }
#endif

inline void String::FreeCMem( const char* str ) const
#ifdef _UNICODE
	{ delete [] const_cast<char*>(str); }
#else // _MBCS or _SBCS
	{}
#endif



#endif // __ccdoc__
#undef XTCHAR
//=========================================================================
//@{
//	文字列処理＋α
//
//	Stringクラス内のバッファ確保関数を呼べるようにした版Stringです。
//@}
//=========================================================================

struct RawString : public String
{
	TCHAR* AllocMem( size_t m ) { return String::AllocMem(m); }
	void UnlockMem()            { String::UnlockMem(); }
};

}      // namespace ki


//=========================================================================

#endif // _KILIB_STRING_H_
