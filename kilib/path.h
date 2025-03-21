#ifndef _KILIB_PATH_H_
#define _KILIB_PATH_H_
#include "types.h"
#include "kstring.h"

DWORD GetFileAttributesUNC(LPCTSTR fname) A_NONNULL;
HANDLE CreateFileUNC(
	LPCTSTR fname,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
);

#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	ファイル名処理
//
//	基本的には文字列ですが、特にファイル名として扱うときに便利な
//	クラスです。名前部分のみ取り出しとか拡張子のみ取り出しとか、
//	属性を取ってくるとか色々。
//@}
//=========================================================================

class A_WUNUSED Path A_FINAL: public String
{
public:

	Path() {}

	//@{ 別のPathのコピー //@}
	Path( const Path&   s )              : String( s ){}
	Path( const String& s )              : String( s ){}
	Path( const TCHAR*  s, long siz=-1 ) : String( s, siz ){}

	//@{ 単純代入 //@}
	Path& operator=( const Path&   s )
		{ return static_cast<Path&>(String::operator=(s)); }
	Path& operator=( const String& s )
		{ return static_cast<Path&>(String::operator=(s)); }
	Path& operator=( const TCHAR*  s )
		{ return static_cast<Path&>(String::operator=(s)); }

	//@{ 加算代入 //@}
	Path& operator+=( const Path&   s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( const String& s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( const TCHAR*  s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( TCHAR c )
		{ return static_cast<Path&>(String::operator+=(c)); }

	//@{ 特殊パス取得して初期化 //@}
	explicit Path( int nPATH, bool bs=true )
		{ BeSpecialPath( nPATH, bs ); }

	//@{ 特殊パス取得 //@}
	Path& BeSpecialPath( int nPATH, bool bs=true );

	//@{ 特殊パス指定用定数 //@}
	enum { Win=0x1787, Sys, Tmp, Exe, Cur, ExeName,
			Snd=CSIDL_SENDTO, Dsk=CSIDL_DESKTOPDIRECTORY };

	//@{ 最後にバックスラッシュを入れる(true)/入れない(false) //@}
	Path& BeBackSlash( bool add );

	//@{ ドライブ名ないしルートのみ //@}
	Path& BeDriveOnly();

	//@{ ディレクトリ名のみ //@}
	Path& BeDirOnly();

	//@{ 短いパス名 //@}
	Path& BeShortStyle();

	//@{ ファイル名だけは確実に長く //@}
	Path& BeShortLongStyle();

	//@{ ...とかを入れて短く //@}
	void CompactIfPossible(TCHAR *buf, size_t Mx) A_NONNULL;

	//@{ ディレクトリ情報以外 //@}
	inline const TCHAR* name() const { return name(c_str()); }

	//@{ 最後の拡張子 //@}
	inline const TCHAR* ext() const { return ext(c_str()); }

	//@{ 最初の.以降全部と見なした拡張子 //@}
	inline const TCHAR* ext_all() const { return ext_all(c_str()); }

	//@{ ディレクトリ情報と最後の拡張子を除いた名前部分 //@}
	Path body() const;

	//@{ ディレクトリ情報と最初の.以降全部を除いた名前部分 //@}
	Path body_all() const;

public:

	//@{ ファイルかどうか //@}
	inline bool isFile() const
		{ return c_str()[len()-1] != TEXT('\\') && c_str()[len()-1] != TEXT('/')
	      && 0==(GetFileAttributesUNC(c_str())&FILE_ATTRIBUTE_DIRECTORY); }


	//@{ ディレクトリかどうか //@}
	inline bool isDirectory() const { return isDirectory( c_str() ); }
	inline static bool isDirectory( const TCHAR *fn ) A_NONNULL
		{ DWORD x=GetFileAttributesUNC( fn );
		  return x!=0xffffffff && (x&FILE_ATTRIBUTE_DIRECTORY)!=0; }


	//@{ 存在するかどうか。isFile() || isDirectory() //@}
	inline bool exist() const { return exist( c_str() ); }
	inline static bool exist( const TCHAR *fn ) A_NONNULL
		{ return 0xffffffff != GetFileAttributesUNC(fn); }

	//@{ 読み取り専用かどうか //@}
	inline bool isReadOnly() const { return isReadOnly( c_str() ); }
	inline static bool isReadOnly( const TCHAR *fn )
		{ DWORD x = GetFileAttributesUNC( fn );
		  return x!=0xffffffff && (x&FILE_ATTRIBUTE_READONLY)!=0; }

	inline FILETIME getLastWriteTime() const { return getLastWriteTime( c_str() ); }
	static FILETIME getLastWriteTime( const TCHAR *fn );

public:

	static const TCHAR* name( const TCHAR* str );
	static const TCHAR* ext( const TCHAR* str );
	static const TCHAR* ext_all( const TCHAR* str );
	static DWORD GetExeName( TCHAR buf[MAX_PATH] ) A_NONNULL;
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_PATH_H_
