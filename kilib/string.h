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
#else
	#define my_lstrcpy my_lstrcpyA
	#define my_lstrcpyn my_lstrcpynA
	#define my_lstrlen my_lstrlenA
	#define my_lstrcmp my_lstrcmpA
#endif

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
//	Άρ{ΏQ
//
//	WideΆΕΦπ©OΕ
//@}
//=========================================================================
//#undef lstrcpy
//#undef lstrlen
//#undef lstrcmp


//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	Άρ
//
//	©ΘθMFCΜCStringπpNΑΔά·BΖθ ¦Έ operator= Ιζι
//	PγόΙΝΩΖρΗRXgͺ©©ηΘ’ζ€Ι΅ά΅½BSubStr()Μ
//	ΰRs[΅Θ’ζ€Ι΅ζ€©ΖΰvΑ½ρΕ·ͺA»±άΕΝ
//	vηΘ’Ύλ€Ζ’€Cΰ·ιΜΕcB
//@}
//=========================================================================

class String : public Object
{
public:

	//@{ σΆρμ¬ //@}
	String();
	~String();

	//@{ ΚΜStringΜRs[ //@}
	String( const String& obj );

	//@{ ΚΜΆzρΜRs[ //@}
	String( const TCHAR* str, long siz=-1 );

	//@{ \[X©ημ¬ //@}
	explicit String( UINT rsrcID );

	//@{ εΆ¬ΆπζΚ·ιδr //@}
	bool operator==( LPCTSTR s ) const;
	bool operator==( const String& obj ) const;

	//@{ εΆ¬ΆπζΚ΅Θ’δr //@}
	bool isSame( LPCTSTR s ) const;
	bool isSame( const String& obj ) const;

	//@{ Pγό //@}
	String& operator=( const String& obj );
	String& operator=( const TCHAR* s );
	String& operator=( const XTCHAR* s );

	//@{ ΑZγό //@}
	String& operator+=( const String& obj );
	String& operator+=( const TCHAR* s );
	String& operator+=( TCHAR c );

	//@{ \[X[h //@}
	String& Load( UINT rsrcID );

	//@{ Eπνι //@}
	void TrimRight( ulong siz );

	//@{ int©ηΆρΦΟ· //@}
	String& SetInt( int n );

	//@{ Άρ©ηintΦΟ· //@}
	int GetInt();

public:

	//@{ Άρobt@πΤ· //@}
	const TCHAR* c_str() const;

	//@{ ·³ //@}
	ulong len() const;

	//@{ vf //@}
	const TCHAR operator[](int n) const;

	//@{ ChΆρΙΟ·΅ΔΤ· //@}
	const wchar_t* ConvToWChar() const;

	//@{ ConvToWCharΜΤlobt@Μπϊ //@}
	void FreeWCMem( const wchar_t* wc ) const;

public:

	//@{ ΜκΆ //@}
	static TCHAR*       next( TCHAR* p );
	static const TCHAR* next( const TCHAR* p );

	//@{ QoCgΆΜζͺ©Η€©H //@}
	static bool isLB( TCHAR c );

	//@{ Άρ©ηintΦΟ· //@}
	static int GetInt( const TCHAR* p );

protected:

	// «έΒ\Θobt@πAI[άίΔΕαΕΰminimumΆͺpΣ·ι
	TCHAR* AllocMem( ulong minimum );
	TCHAR* ReallocMem( ulong minimum );

	// «έIΉγA·³πΔέθ
	void UnlockMem( long siz=-1 );

private:

	struct StringData
	{
		long  ref;         // QΖJE^
		ulong len;         // I['\0'πάίι·³
		ulong alen;        // θΔηκΔ’ιΜTCY
		TCHAR* buf() const // TCHAR buf[alen]
			{ return reinterpret_cast<TCHAR*>(
				const_cast<StringData*>(this+1)
			); }
	};

private:

	TCHAR*  AllocMemHelper( ulong minimum, const TCHAR* str, ulong siz );
	String& CatString( const TCHAR* str, ulong siz );
	String& SetString( const TCHAR* str, ulong siz );
	void    SetData( StringData* d );
	void    ReleaseData();
	static  StringData* null();
	        StringData* data() const;

private:

	StringData*        data_;
	static StringData* nullData_;
	static char        lb_[256];

private:

	static void LibInit();
	friend void APIENTRY Startup();
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

// ϊ»
inline String::String()
	{ SetData( null() ); }
// ϊ»
inline String::String( UINT rsrcID )
	{ SetData( null() ), Load( rsrcID ); }
// ϊ»
inline String::String( const String& obj )
	{ SetData( obj.data() ); }

// |C^vZT|[g
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

// ΰmΫ
inline TCHAR* String::AllocMem( ulong minimum )
	{ return AllocMemHelper( minimum, TEXT(""), 1 ); }
// ΰΕθ
inline void String::UnlockMem( long siz )
	{ data_->len = 1 + (siz==-1 ? my_lstrlen(c_str()) : siz); }

// OΆf[^
inline String::StringData* String::null()
	{ return nullData_; }
// ΰf[^\’
inline String::StringData* String::data() const
	{ return data_; }
// ϊ»
inline void String::SetData( String::StringData* d )
	{ data_=d, data_->ref++; }

// ?«
inline const TCHAR* String::c_str() const
	{ return data_->buf(); }
// ?«
inline ulong String::len() const
	{ return data_->len-1; }
// vf
inline const TCHAR String::operator[](int n) const
	{ return data_->buf()[n]; }

// δr
inline bool String::operator==( LPCTSTR s ) const
	{ return 0==my_lstrcmp( c_str(), s ); }
// δr
inline bool String::operator==( const String& obj ) const
	{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }
// δr
inline bool String::isSame( LPCTSTR s ) const
{ return 0==::lstrcmpi( c_str(), s ); }
// δr
inline bool String::isSame( const String& obj ) const
	{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }

// vRs[γό
inline String& String::operator = ( const TCHAR* s )
	{ return SetString( s, my_lstrlen(s) ); }
// ¬
inline String& String::operator += ( const String& obj )
	{ return CatString( obj.c_str(), obj.len() ); }
// ¬
inline String& String::operator += ( const TCHAR* s )
	{ return CatString( s, my_lstrlen(s) ); }
// ¬
inline String& String::operator += ( TCHAR c )
	{ return CatString( &c, 1 ); }

// Ο·
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

// ConvToWCharΜΤlobt@Μπϊ
inline void String::FreeWCMem( const wchar_t* wc ) const
#ifdef _UNICODE
	{}
#else // _MBCS or _SBCS
	{ delete [] const_cast<wchar_t*>(wc); }
#endif



#endif // __ccdoc__
#undef XTCHAR
//=========================================================================
//@{
//	Άρ{Ώ
//
//	StringNXΰΜobt@mΫΦπΔΧιζ€Ι΅½ΕStringΕ·B
//@}
//=========================================================================

struct RawString : public String
{
	TCHAR* AllocMem( ulong m ) { return String::AllocMem(m); }
	void UnlockMem()           { String::UnlockMem(); }
};

}      // namespace ki


//=========================================================================

#endif // _KILIB_STRING_H_
