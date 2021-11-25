#ifndef _KILIB_REGISTRY_H_
#define _KILIB_REGISTRY_H_
#include "types.h"
#include "memory.h"
#include "path.h"
#ifndef __ccdoc__
namespace ki {
#endif


	
//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	INIファイル読み込み
//
//@}
//=========================================================================

class IniFile : public Object
{
public:

	//@{ コンストラクタ //@}
	IniFile( const TCHAR* ini=NULL, bool exeppath=true );

	//@{ iniファイル名を設定 //@}
	void SetFileName( const TCHAR* ini=NULL, bool exepath=true );

	//@{ セクション名を設定 //@}
	void SetSection( const TCHAR* section );

	//@{ セクション名をユーザー名に設定 //@}
	void SetSectionAsUserName();

	//@{ ある特定の名前のセクションがあるかどうか？ //@}
	bool HasSectionEnabled( const TCHAR* section ) const;

	//@{ 整数値読み込み //@}
	int    GetInt ( const TCHAR* key, int   defval ) const;
	//@{ 真偽値読み込み //@}
	bool   GetBool( const TCHAR* key, bool  defval ) const;
	//@{ 文字列読み込み //@}
	String GetStr ( const TCHAR* key, const String& defval ) const;
	//@{ パス文字列読み込み //@}
	Path  GetPath ( const TCHAR* key, const Path& defval ) const;

	//@{ 整数値書き込み //@}
	bool PutInt ( const TCHAR* key, int val );
	//@{ 真偽値書き込み //@}
	bool PutBool( const TCHAR* key, bool val );
	//@{ 文字列書き込み //@}
	bool PutStr ( const TCHAR* key, const TCHAR* val );
	//@{ パス書き込み //@}
	bool PutPath( const TCHAR* key, const Path& val );

private:

	Path   iniName_;
	String section_;
	char m_StrBuf[256];
};



//-------------------------------------------------------------------------

inline IniFile::IniFile( const TCHAR* ini, bool exepath )
	{ SetFileName( ini, exepath ); }

inline void IniFile::SetSection( const TCHAR* section )
	{ section_ = section; }




//=========================================================================

}      // namespace ki
#endif // _KILIB_REGISTRY_H_
