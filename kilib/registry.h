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

class IniFile : public Object
{
public:

	//@{ �R���X�g���N�^ //@}
	IniFile( const TCHAR* ini=NULL, bool exeppath=true );

	//@{ ini�t�@�C������ݒ� //@}
	void SetFileName( const TCHAR* ini=NULL, bool exepath=true );

	//@{ �Z�N�V��������ݒ� //@}
	void SetSection( const TCHAR* section );

	//@{ �Z�N�V�����������[�U�[���ɐݒ� //@}
	void SetSectionAsUserName();

	//@{ �������̖��O�̃Z�N�V���������邩�ǂ����H //@}
	bool HasSectionEnabled( const TCHAR* section ) const;

	//@{ �����l�ǂݍ��� //@}
	int    GetInt ( const TCHAR* key, int   defval ) const;
	//@{ �^�U�l�ǂݍ��� //@}
	bool   GetBool( const TCHAR* key, bool  defval ) const;
	//@{ ������ǂݍ��� //@}
	String GetStr ( const TCHAR* key, const String& defval ) const;
	//@{ �p�X������ǂݍ��� //@}
	Path  GetPath ( const TCHAR* key, const Path& defval ) const;

	//@{ �����l�������� //@}
	bool PutInt ( const TCHAR* key, int val );
	//@{ �^�U�l�������� //@}
	bool PutBool( const TCHAR* key, bool val );
	//@{ �����񏑂����� //@}
	bool PutStr ( const TCHAR* key, const TCHAR* val );
	//@{ �p�X�������� //@}
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
