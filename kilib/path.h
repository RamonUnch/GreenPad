#ifndef _KILIB_PATH_H_
#define _KILIB_PATH_H_
#include "types.h"
#include "kstring.h"

DWORD GetFileAttributesUNC(LPCTSTR fname);

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

class Path A_FINAL: public String
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
	const TCHAR *CompactIfPossible(TCHAR *buf, unsigned Mx);

	//@{ ディレクトリ情報以外 //@}
	const TCHAR* name() const;

	//@{ 最後の拡張子 //@}
	const TCHAR* ext() const;

	//@{ 最初の.以降全部と見なした拡張子 //@}
	const TCHAR* ext_all() const;

	//@{ ディレクトリ情報と最後の拡張子を除いた名前部分 //@}
	Path body() const;

	//@{ ディレクトリ情報と最初の.以降全部を除いた名前部分 //@}
	Path body_all() const;

public:

	//@{ ファイルかどうか //@}
	bool isFile() const;

	//@{ ディレクトリかどうか //@}
	bool isDirectory() const;
	static bool isDirectory( const TCHAR *fn );

	//@{ 存在するかどうか。isFile() || isDirectory() //@}
	bool exist() const;
	static bool exist( const TCHAR *fn );

	//@{ 読み取り専用かどうか //@}
	bool isReadOnly() const;
	static bool isReadOnly( const TCHAR *fn );

public:

	static const TCHAR* name( const TCHAR* str );
	static const TCHAR* ext( const TCHAR* str );
	static const TCHAR* ext_all( const TCHAR* str );
};



//-------------------------------------------------------------------------

inline bool Path::isFile() const
	{ return c_str()[len()-1] != TEXT('\\') && c_str()[len()-1] != TEXT('/')
	      && 0==(GetFileAttributesUNC(c_str())&FILE_ATTRIBUTE_DIRECTORY); }

inline bool Path::isDirectory() const
	{ return isDirectory( c_str() ); }

inline bool Path::isDirectory( const TCHAR *fn ) // static
	{ DWORD x=GetFileAttributesUNC( fn );
	  return x!=0xffffffff && (x&FILE_ATTRIBUTE_DIRECTORY)!=0; }

inline bool Path::exist() const
	{ return exist( c_str() ); }

inline bool Path::exist( const TCHAR *fn ) // static
	{ return 0xffffffff != GetFileAttributesUNC(fn); }

inline bool Path::isReadOnly() const
	{ return isReadOnly( c_str() ); }

inline bool Path::isReadOnly( const TCHAR *fn ) // Static
	{ DWORD x = GetFileAttributesUNC( fn );
	  return x!=0xffffffff && (x&FILE_ATTRIBUTE_READONLY)!=0; }

inline const TCHAR* Path::name() const
	{ return name(c_str()); }

inline const TCHAR* Path::ext() const
	{ return ext(c_str()); }

inline const TCHAR* Path::ext_all() const
	{ return ext_all(c_str()); }



//=========================================================================

}      // namespace ki
#endif // _KILIB_PATH_H_
