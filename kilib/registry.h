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

class A_WUNUSED IniFile
{
public:

	//@{ コンストラクタ //@}
	IniFile( /*const TCHAR* ini=NULL, bool exeppath=true */ );

	//@{ iniファイル名を設定 //@}
	void SetFileName( /*const TCHAR* ini=NULL, bool exepath=true*/ );

	//@{ セクション名を設定 //@}
	void SetSection( const TCHAR* section );

	//@{ Cache section //@}
	void CacheSection();

	//@{ セクション名をユーザー名に設定 //@}
	void SetSectionAsUserName();
	bool SetSectionAsUserNameIfNotShared( const TCHAR *section);

	//@{ ある特定の名前のセクションがあるかどうか？ //@}
	bool HasSectionEnabled( const TCHAR* section ) const;

	//@{ 整数値読み込み //@}
	int    GetInt ( const TCHAR* key, int   defval ) const;
	//@{ 真偽値読み込み //@}
	bool   GetBool( const TCHAR* key, bool  defval ) const;
	//@{ Get rect from ini file//@}
	void GetRect ( const TCHAR* key, RECT *rc, const RECT *defrc) const;
	//@{ 文字列読み込み //@}
	String GetStr ( const TCHAR* key, const TCHAR *defval ) const;
	//@{ パス文字列読み込み //@}
	Path  GetPath ( const TCHAR* key, const TCHAR *defval ) const;
	//@{ Get String in section //@}
	String GetStrinSect( const TCHAR* key, const TCHAR* sect, const TCHAR *defval ) const;
	TCHAR *GetSStrHere(const TCHAR* key, const TCHAR* sect, const TCHAR *defval, TCHAR buf[MAX_PATH]) const;

	//@{ 整数値書き込み //@}
	bool PutInt ( const TCHAR* key, int val );
	//@{ 真偽値書き込み //@}
	bool PutBool( const TCHAR* key, bool val );
	//@{ Save rect to ini file //@}
	bool PutRect( const TCHAR* key, const RECT *rc);
	//@{ 文字列書き込み //@}
	bool PutStr ( const TCHAR* key, const TCHAR* val );
	//@{ パス書き込み //@}
	bool PutPath( const TCHAR* key, const Path& val );
	//@{ Put string in section //@}
	bool PutStrinSect( const TCHAR* key, const TCHAR *sect, const TCHAR* val );

	const TCHAR *getName() const { return iniName_; };

private:

	TCHAR  iniName_[MAX_PATH];
	TCHAR  section_[64];
};



//-------------------------------------------------------------------------

inline IniFile::IniFile( /*const TCHAR* ini, bool exepath*/ )
	{ SetFileName( /*ini, exepath*/ ); }

inline void IniFile::SetSection( const TCHAR* section )
	{ my_lstrcpyn( section_, section, countof(section_) ); }




//=========================================================================

}      // namespace ki
#endif // _KILIB_REGISTRY_H_
