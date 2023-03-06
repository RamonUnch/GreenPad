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
//	�����񏈗��{���Q
//
//	Wide�����Ŋ֐������O��
//@}
//=========================================================================
//#undef lstrcpy
//#undef lstrlen
//#undef lstrcmp


//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	�����񏈗�
//
//	���Ȃ�MFC��CString���p�N���Ă܂��B�Ƃ肠���� operator= �ɂ��
//	�P������ɂ͂قƂ�ǃR�X�g��������Ȃ��悤�ɂ��܂����BSubStr()��
//	�����R�s�[���Ȃ��悤�ɂ��悤���Ƃ��v������ł����A�����܂ł�
//	�v��Ȃ����낤�Ƃ����C������̂Łc�B
//@}
//=========================================================================

class String : public Object
{
public:

	//@{ �󕶎���쐬 //@}
	String();
	~String();

	//@{ �ʂ�String�̃R�s�[ //@}
	String( const String& obj );

	//@{ �ʂ̕����z��̃R�s�[ //@}
	String( const TCHAR* str, long siz=-1 );

	//@{ ���\�[�X����쐬 //@}
	explicit String( UINT rsrcID );

	//@{ �啶������������ʂ����r //@}
	bool operator==( LPCTSTR s ) const;
	bool operator==( const String& obj ) const;

	//@{ �啶������������ʂ��Ȃ���r //@}
	bool isSame( LPCTSTR s ) const;
	bool isSame( const String& obj ) const;

	//@{ �P����� //@}
	String& operator=( const String& obj );
	String& operator=( const TCHAR* s );
	String& operator=( const XTCHAR* s );

	//@{ ���Z��� //@}
	String& operator+=( const String& obj );
	String& operator+=( const TCHAR* s );
	String& operator+=( TCHAR c );

	//@{ ���\�[�X���[�h //@}
	String& Load( UINT rsrcID );

	//@{ �E����� //@}
	void TrimRight( size_t siz );

	//@{ int���當����֕ϊ� //@}
	String& SetInt( int n );

	//@{ �����񂩂�int�֕ϊ� //@}
	int GetInt();

public:

	//@{ ������o�b�t�@��Ԃ� //@}
	const TCHAR* c_str() const;

	//@{ ���� //@}
	size_t len() const;

	//@{ �v�f //@}
	const TCHAR operator[](int n) const;

	//@{ ���C�h������ɕϊ����ĕԂ� //@}
	const wchar_t* ConvToWChar() const;
	const char* ConvToChar() const;

	//@{ ConvToWChar�̕Ԓl�o�b�t�@�̉�� //@}
	void FreeWCMem( const wchar_t* wc ) const;
	void FreeCMem( const char* str ) const;

public:

	//@{ ���̈ꕶ�� //@}
	static TCHAR*       next( TCHAR* p );
	static const TCHAR* next( const TCHAR* p );

	//@{ �Q�o�C�g�����̐擪���ǂ����H //@}
	static bool isLB( TCHAR c );

	//@{ �����񂩂�int�֕ϊ� //@}
	static int GetInt( const TCHAR* p );

protected:

	// �������݉\�ȃo�b�t�@���A�I�[�܂߂čŒ�ł�minimum�������p�ӂ���
	TCHAR* AllocMem( size_t minimum );
	TCHAR* ReallocMem( size_t minimum );

	// �������ݏI����A�������Đݒ�
	void UnlockMem( long siz=-1 );

private:

	struct StringData
	{
		long  ref;         // �Q�ƃJ�E���^
		size_t len;        // �I�['\0'���܂߂钷��
		size_t alen;       // ���蓖�Ă��Ă��郁�����̃T�C�Y
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

// ������
inline String::String()
	{ SetData( null() ); }
// ������
inline String::String( UINT rsrcID )
	{ SetData( null() ), Load( rsrcID ); }
// ������
inline String::String( const String& obj )
	{ SetData( obj.data() ); }

// �|�C���^�v�Z�T�|�[�g
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

// �����������m��
inline TCHAR* String::AllocMem( size_t minimum )
	{ return AllocMemHelper( minimum, TEXT(""), 1 ); }
// �����������Œ�
inline void String::UnlockMem( long siz )
	{ data_->len = 1 + (siz==-1 ? my_lstrlen(c_str()) : siz); }

// �O�����f�[�^
inline String::StringData* String::null()
	{ return nullData_; }
// �����f�[�^�\��
inline String::StringData* String::data() const
	{ return data_; }
// ������
inline void String::SetData( String::StringData* d )
	{ data_=d, data_->ref++; }

// ����
inline const TCHAR* String::c_str() const
	{ return data_->buf(); }
// ����
inline size_t String::len() const
	{ return data_->len-1; }
// �v�f
inline const TCHAR String::operator[](int n) const
	{ return data_->buf()[n]; }

// ��r
inline bool String::operator==( LPCTSTR s ) const
	{ return 0==my_lstrcmp( c_str(), s ); }
// ��r
inline bool String::operator==( const String& obj ) const
	{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }
// ��r
inline bool String::isSame( LPCTSTR s ) const
{ return 0==::lstrcmpi( c_str(), s ); }
// ��r
inline bool String::isSame( const String& obj ) const
	{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }

// �v�R�s�[���
inline String& String::operator = ( const TCHAR* s )
	{ return SetString( s, my_lstrlen(s) ); }
// ����
inline String& String::operator += ( const String& obj )
	{ return CatString( obj.c_str(), obj.len() ); }
// ����
inline String& String::operator += ( const TCHAR* s )
	{ return CatString( s, my_lstrlen(s) ); }
// ����
inline String& String::operator += ( TCHAR c )
	{ return CatString( &c, 1 ); }

// �ϊ�
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

// ConvToWChar�̕Ԓl�o�b�t�@�̉��
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
//	�����񏈗��{��
//
//	String�N���X���̃o�b�t�@�m�ۊ֐����Ăׂ�悤�ɂ�����String�ł��B
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
