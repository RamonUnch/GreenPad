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
//	INI�t�@�C���ǂݍ���
//
//@}
//=========================================================================

class A_WUNUSED IniFile
{
public:

	//@{ �R���X�g���N�^ //@}
	IniFile( /*const TCHAR* ini=NULL, bool exeppath=true */ );

	//@{ ini�t�@�C������ݒ� //@}
	void SetFileName( /*const TCHAR* ini=NULL, bool exepath=true*/ );

	//@{ �Z�N�V��������ݒ� //@}
	void SetSection( const TCHAR* section );

	//@{ Cache section //@}
	void CacheSection();

	//@{ �Z�N�V�����������[�U�[���ɐݒ� //@}
	void SetSectionAsUserName();
	bool SetSectionAsUserNameIfNotShared( const TCHAR *section);

	//@{ �������̖��O�̃Z�N�V���������邩�ǂ����H //@}
	bool HasSectionEnabled( const TCHAR* section ) const;

	//@{ �����l�ǂݍ��� //@}
	int    GetInt ( const TCHAR* key, int   defval ) const;
	//@{ �^�U�l�ǂݍ��� //@}
	bool   GetBool( const TCHAR* key, bool  defval ) const;
	//@{ Get rect from ini file//@}
	void GetRect ( const TCHAR* key, RECT *rc, const RECT *defrc) const;
	//@{ ������ǂݍ��� //@}
	String GetStr ( const TCHAR* key, const TCHAR *defval ) const;
	//@{ �p�X������ǂݍ��� //@}
	Path  GetPath ( const TCHAR* key, const TCHAR *defval ) const;
	//@{ Get String in section //@}
	String GetStrinSect( const TCHAR* key, const TCHAR* sect, const TCHAR *defval ) const;
	TCHAR *GetSStrHere(const TCHAR* key, const TCHAR* sect, const TCHAR *defval, TCHAR buf[MAX_PATH]) const;

	//@{ �����l�������� //@}
	bool PutInt ( const TCHAR* key, int val );
	//@{ �^�U�l�������� //@}
	bool PutBool( const TCHAR* key, bool val );
	//@{ Save rect to ini file //@}
	bool PutRect( const TCHAR* key, const RECT *rc);
	//@{ �����񏑂����� //@}
	bool PutStr ( const TCHAR* key, const TCHAR* val );
	//@{ �p�X�������� //@}
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
